#include "chamfercentralvalidation.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <utility>

namespace {

constexpr qsizetype MaximumSamples = 200000;
constexpr double PreviewPositionTolerance = 0.01; // mm; Cartesian chord criterion only.

bool positiveFinite(double value)
{
  return std::isfinite(value) && value > 0.0;
}

struct ColumnStretch
{
  double minimum = 0.0;
  double maximum = 0.0;
};

// Singular values of a 3x2 matrix from its columns, without iterative decomposition.
std::optional<ColumnStretch> columnStretch(const V3d& first, const V3d& second)
{
  const double scale = std::max(first.stableNorm(), second.stableNorm());
  if (!first.allFinite() || !second.allFinite() || !positiveFinite(scale)) return std::nullopt;
  const V3d left = first / scale;
  const V3d right = second / scale;
  const double aa = left.squaredNorm();
  const double bb = right.squaredNorm();
  const double ab = left.dot(right);
  const double maximum = std::sqrt(0.5 * (aa + bb + std::hypot(aa - bb, 2.0 * ab)));
  // Product of singular values equals the parallelogram area; avoids cancellation.
  const double minimum = left.cross(right).stableNorm() / maximum;
  if (!positiveFinite(minimum) || minimum <= GeomConst::Eps * maximum) return std::nullopt;
  const ColumnStretch result{scale * minimum, scale * maximum};
  if (!positiveFinite(result.minimum) || !positiveFinite(result.maximum)) return std::nullopt;
  return result;
}

struct MachiningDerivativeBounds
{
  double first = 0.0; // Upper norm bound, mm per radian.
  double second = 0.0; // Upper norm bound, mm per radian squared.
  double axialFirst = 0.0; // Upper axial first-derivative magnitude, mm per radian.
  double transverseFirst = 0.0; // Lower transverse first-derivative norm, mm per radian.
};

// Coefficient-based bounds derived in plans/SPATIAL_LEADS_CURVATURE_STEP1_RU.md.
// Includes the chamfer offset and the actual (rounded) ellipse projection coefficients.
std::optional<MachiningDerivativeBounds> machiningDerivativeBounds(const ChamferPath& path)
{
  const auto& parameters = path.parameters();
  const auto& edge = parameters.edge;
  const V3d& axis = path.outwardAxis();
  const V3d major = edge.majorRadius * edge.majorAxis;
  const V3d minor = edge.minorRadius * edge.minorAxis;
  const auto full = columnStretch(major, minor);
  const auto projected = columnStretch(major - major.dot(axis) * axis,
                                        minor - minor.dot(axis) * axis);
  if (!full || !projected) return std::nullopt;
  const V3d normal = edge.majorAxis.cross(edge.minorAxis).normalized();
  const double slope = normal.cross(axis).stableNorm() / std::abs(normal.dot(axis));
  const double ratio = std::max(1.0, projected->maximum / projected->minimum);
  const double ratioSquared = ratio * ratio;
  const double radialAngularSecond = ratio * (ratioSquared - 1.0);
  const double radialSecond = ratioSquared + radialAngularSecond;
  const double firstG = slope * ratio;
  const double secondG = 2.0 * firstG * firstG + slope * radialSecond;
  const double cotangent = 1.0 / std::tan(parameters.chamferAngleDegrees * GeomConst::DegToRad);
  const double firstAxialFactor = firstG * (2.0 * slope + cotangent) + 2.0 * slope * ratio;
  const double secondAxialFactor = secondG * (2.0 * slope + cotangent)
      + 4.0 * firstG * slope * ratio + 2.0 * slope * radialSecond;
  const double halfSize = 0.5 * parameters.chamferSize;
  const MachiningDerivativeBounds result{
      full->maximum + halfSize * (firstG + ratio + firstAxialFactor),
      full->maximum + halfSize * (secondG + 2.0 * firstG * ratio + radialSecond + secondAxialFactor),
      std::hypot(major.dot(axis), minor.dot(axis)) + halfSize * firstAxialFactor,
      (projected->minimum + halfSize / std::hypot(1.0, slope)) / ratio};
  if (!positiveFinite(result.first) || !positiveFinite(result.second)
      || !std::isfinite(result.axialFirst) || result.axialFirst < 0.0
      || !positiveFinite(result.transverseFirst)) return std::nullopt;
  return result;
}

std::optional<double> leadCurvatureBound(const MachiningDerivativeBounds& bounds,
                                       const ChamferLeadParameters& lead)
{
  const double span = lead.spanDegrees * GeomConst::DegToRad;
  const double transverse = bounds.transverseFirst * span;
  const double axial = bounds.axialFirst * span;
  const double clearance = lead.clearance;
  if (!positiveFinite(transverse) || !std::isfinite(axial) || axial < 0.0
      || !positiveFinite(clearance)) return std::nullopt;
  // Scale the Gram eigenvalue formula to avoid squared/fourth-power overflow.
  const double scale = std::max({transverse, axial, clearance});
  const double x = transverse / scale;
  const double y = axial / scale;
  const double z = clearance / scale;
  const double sum = x * x + y * y + z * z;
  const double discriminant = std::hypot(x - z, y) * std::hypot(x + z, y);
  const double minimum = (x * z) * std::sqrt(2.0 / (sum + discriminant));
  if (!positiveFinite(minimum) || minimum <= GeomConst::Eps * std::sqrt(sum))
    return std::nullopt;
  const double firstLower = scale * minimum;
  const double secondUpper = bounds.second * span * span + bounds.first * span + clearance;
  const double curvature = (secondUpper / firstLower) / firstLower;
  if (!positiveFinite(firstLower) || !positiveFinite(secondUpper) || !positiveFinite(curvature))
    return std::nullopt;
  return curvature;
}

std::optional<std::array<double, 3>> phaseCurvatureBounds(const ChamferPath& path)
{
  const auto bounds = machiningDerivativeBounds(path);
  if (!bounds) return std::nullopt;
  const auto leadIn = leadCurvatureBound(*bounds, path.parameters().leadIn);
  const auto leadOut = leadCurvatureBound(*bounds, path.parameters().leadOut);
  const double machining = (bounds->second / bounds->transverseFirst) / bounds->transverseFirst;
  if (!leadIn || !leadOut || !positiveFinite(machining)) return std::nullopt;
  return std::array<double, 3>{*leadIn, machining, *leadOut};
}

struct PreviewSchedule
{
  QVector<ChamferCentralPreviewSpan> spans;
  qsizetype points = 1; // Shared starts/ends are stored only once.
  QString error;
};

PreviewSchedule makePreviewSchedule(const ChamferPath& path, const ChamferCentralMotion& motion)
{
  const auto curvature = phaseCurvatureBounds(path);
  if (!curvature) return {{}, 0, QStringLiteral("Cannot bound TCP curvature: degenerate projection, ill-conditioned lead dimensions or numerical range exceeded.")};
  PreviewSchedule schedule;
  for (int index = 0; index < 3; ++index) {
    const auto phase = static_cast<ChamferPhase>(index);
    const auto profile = motion.profile(phase);
    if (!profile) return {{}, 0, QStringLiteral("Missing preview phase profile.")};
    const double start = motion.phaseTimes()[index];
    const double end = motion.phaseTimes()[index + 1];
    QVector<double> stops{start};
    if (index != 1 && profile->cruiseSeconds > 0.0)
      stops.append(start + (index == 0 ? profile->rampSeconds : profile->cruiseSeconds));
    stops.append(end);
    const double normalAcceleration = profile->speed * (profile->speed
        * (*curvature)[index]);
    for (qsizetype part = 1; part < stops.size(); ++part) {
      const bool ramp = index == 0 ? part == 1 : index == 2 && part == stops.size() - 1;
      const double bound = std::hypot(ramp ? profile->acceleration : 0.0, normalAcceleration);
      const double duration = stops[part] - stops[part - 1];
      const double count = std::max(1.0, std::ceil(duration
          * std::sqrt(bound / (8.0 * PreviewPositionTolerance))));
      if (!std::isfinite(bound) || bound <= 0.0 || !std::isfinite(duration) || duration <= 0.0
          || !std::isfinite(count) || count > MaximumSamples - schedule.points)
        return {{}, 0, QStringLiteral("Preview geometric schedule exceeds numerical range or 200000 points.")};
      const int intervals = static_cast<int>(count);
      if (!(duration / intervals > 1e-9))
        return {{}, 0, QStringLiteral("Preview geometric interval is below time resolution.")};
      schedule.spans.append({stops[part - 1], stops[part], intervals,
          index == 0 ? QStringLiteral("Lead-in") : index == 1 ? QStringLiteral("Machining")
                                                            : QStringLiteral("Lead-out")});
      schedule.points += intervals;
    }
  }
  return schedule;
}

} // namespace

ChamferCentralPreparation::ChamferCentralPreparation(const ChamferPath& path,
                                                     const ChamferTimingParameters& timing)
  : m_path(path), m_timing(timing) {}

QString ChamferCentralPreparation::prepare()
{
  if (m_attempted) return m_error;
  m_attempted = true;
  auto motion = ChamferCentralMotion::create(m_path, m_timing);
  m_error = motion.error;
  if (motion.motion) {
    m_motion = std::move(motion.motion);
    auto schedule = makePreviewSchedule(m_path, *m_motion);
    m_error = schedule.error;
    if (m_error.isEmpty()) {
      m_spans = std::move(schedule.spans);
      m_plannedSamples = schedule.points;
    }
  }
  return m_error;
}

std::optional<ChamferCentralPose> ChamferCentralPreparation::evaluateTarget(qsizetype index,
                                                                         double seconds) const
{
  if (index >= 0 && index < m_targets.size()) return m_targets[index];
  if (!m_motion || index != m_targets.size() || index >= m_plannedSamples)
    return std::nullopt;
  const auto target = m_motion->evaluate(seconds);
  m_targets.append(target); // A failed target remains at this exact scheduled index.
  return target;
}

PathSchedule CentralPath::schedule() const
{
  PathSchedule result;
  const auto& spans = m_preparation.spans();
  result.times.reserve(m_preparation.plannedSamples() - 1);
  for (qsizetype index = 0; index < spans.size(); ++index) {
    const auto& span = spans[index];
    for (int point = 1; point <= span.intervals; ++point)
      result.times.append(point == span.intervals ? span.end
          : span.start + (span.end - span.start) * (double(point) / span.intervals));
    if (index + 1 == spans.size() || spans[index + 1].phase != span.phase)
      result.phases.append(std::pair<qsizetype, QString>{result.times.size(), span.phase});
  }
  return result;
}

std::optional<PathTarget> CentralPath::target(qsizetype index, double seconds) const
{
  const auto pose = m_preparation.evaluateTarget(index, seconds);
  if (!pose) return std::nullopt;
  // The shared point at a phase end belongs to the ending phase.
  const auto& times = m_preparation.motion().phaseTimes();
  const bool phaseEnd = seconds == times[1] || seconds == times[2] || seconds == times[3];
  return PathTarget{pose->tcp, phaseEnd ? 1.0 : pose->progress};
}
