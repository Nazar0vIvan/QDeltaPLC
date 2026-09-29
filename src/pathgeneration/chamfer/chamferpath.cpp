#include "chamferpath.h"

#include "geometry/utils.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <utility>

namespace {

constexpr double FrameTolerance = 16.0 * GeomConst::Eps;

bool validLead(const ChamferLeadParameters& lead)
{
  return std::isfinite(lead.clearance) && lead.clearance > 0.0
      && std::isfinite(lead.spanDegrees) && lead.spanDegrees > 0.0
      && lead.spanDegrees <= 360.0 && lead.intervals > 0 && lead.intervals <= 100000;
}

V3d transverse(const V3d& vector, const V3d& axis)
{
  return vector - vector.dot(axis) * axis;
}

// An ellipse on a circular cylinder projects to a centered circle in the
// transverse plane. Checking its two harmonic coefficients validates the
// entire curve, rather than just a handful of sampled points.
bool matchesCylinder(const ChamferPathParameters& parameters)
{
  const auto& edge = parameters.edge;
  const V3d& axis = parameters.cylinderAxis;
  const V3d center = transverse(edge.center - parameters.cylinderOrigin, axis);
  const V3d u = transverse(edge.majorRadius * edge.majorAxis, axis);
  const V3d v = transverse(edge.minorRadius * edge.minorAxis, axis);
  const double radius = edge.minorRadius;
  const double magnitude = std::max(edge.center.cwiseAbs().maxCoeff(),
                                    parameters.cylinderOrigin.cwiseAbs().maxCoeff());
  const double tolerance = FrameTolerance * std::max(1.0, edge.majorRadius)
      + 32.0 * std::numeric_limits<double>::epsilon() * magnitude;
  if (!center.allFinite() || !u.allFinite() || !v.allFinite()
      || !std::isfinite(tolerance) || center.stableNorm() > tolerance)
    return false;
  return std::abs(u.stableNorm() - radius) <= tolerance
      && std::abs(v.stableNorm() - radius) <= tolerance
      && std::abs((u / radius).dot(v / radius)) <= FrameTolerance;
}

} // namespace

ChamferPath::ChamferPath(ChamferPathParameters parameters)
  : m_parameters(std::move(parameters))
{
  m_outwardAxis = m_parameters.cylinderAxis;
  if (m_parameters.flipAxis) m_outwardAxis = -m_outwardAxis;
  const auto& edge = m_parameters.edge;
  if (edge.majorAxis.cross(edge.minorAxis).dot(m_outwardAxis) < 0.0)
    m_direction = -1.0;
}

ChamferPathResult ChamferPath::create(ChamferPathParameters parameters)
{
  if (!validLead(parameters.leadIn) || !validLead(parameters.leadOut))
    return {{}, QStringLiteral("Leads require positive clearance, span in (0, 360] degrees and 1–100000 intervals.")};
  if (!EdgeGeometry::fromEllipse(parameters.edge))
    return {{}, QStringLiteral("Invalid chamfer edge ellipse.")};
  if (!parameters.cylinderOrigin.allFinite() || !parameters.cylinderAxis.allFinite())
    return {{}, QStringLiteral("The hole axis must be finite.")};
  const double axisLength = parameters.cylinderAxis.stableNorm();
  if (!std::isfinite(axisLength) || axisLength <= GeomConst::Eps)
    return {{}, QStringLiteral("The hole axis must have a nonzero direction.")};
  parameters.cylinderAxis /= axisLength;
  if (!std::isfinite(parameters.chamferSize) || parameters.chamferSize < 0.0)
    return {{}, QStringLiteral("Chamfer size must be finite and nonnegative.")};
  const auto& edge = parameters.edge;
  if (std::abs(edge.majorAxis.cross(edge.minorAxis).dot(parameters.cylinderAxis))
      <= GeomConst::Eps)
    return {{}, QStringLiteral("The hole axis is parallel to the opening plane.")};
  if (!matchesCylinder(parameters))
    return {{}, QStringLiteral("The edge does not lie on the specified cylindrical hole.")};
  return {ChamferPath(std::move(parameters)), {}};
}

std::optional<ChamferPathSample> ChamferPath::evaluate(double angleRad) const
{
  if (!std::isfinite(angleRad)) return std::nullopt;
  const auto& edge = m_parameters.edge;
  const double angle = m_direction * std::remainder(angleRad, 2.0 * GeomConst::Pi);
  const double cosine = std::cos(angle);
  const double sine = std::sin(angle);
  const V3d offset = edge.majorRadius * cosine * edge.majorAxis
      + edge.minorRadius * sine * edge.minorAxis;
  const V3d derivative = m_direction * (-edge.majorRadius * sine * edge.majorAxis
                                       + edge.minorRadius * cosine * edge.minorAxis);
  const double tangentLength = derivative.stableNorm();
  // The center is on the axis; avoid subtracting large world coordinates here.
  const V3d radial = transverse(offset, m_outwardAxis);
  const double radialLength = radial.stableNorm();
  if (!std::isfinite(tangentLength) || !std::isfinite(radialLength)
      || tangentLength <= GeomConst::Eps || radialLength <= GeomConst::Eps)
    return std::nullopt;
  const V3d x = derivative / tangentLength;
  const V3d r = radial / radialLength;
  if (std::abs(x.dot(r)) > FrameTolerance) return std::nullopt;
  const V3d q = r.cross(x).normalized();
  // Reorthogonalize only the small accepted roundoff, preserving the tangent.
  const V3d normal = (q - x.cross(q)).normalized();
  const V3d y = normal.cross(x).normalized();
  if (normal.dot(m_outwardAxis) <= 0.0 || normal.dot(r) >= 0.0
      || !isBasis(x, y, normal, FrameTolerance))
    return std::nullopt;

  ChamferPathSample sample;
  sample.edgePoint = edge.center + offset;
  sample.radialNormal = r;
  // c is a leg size. For a perpendicular circle this is +c/2 radially
  // into material and -c/2 axially, at the midpoint of the chamfer section.
  const V3d origin = sample.edgePoint - (m_parameters.chamferSize / std::sqrt(2.0)) * normal;
  if (!sample.edgePoint.allFinite() || !origin.allFinite()) return std::nullopt;
  sample.tcp = makeTransform(basis2rot({x, y, normal}), origin);
  return sample;
}

std::optional<ChamferPathSample> ChamferPath::evaluatePhase(ChamferPhase phase, double progress) const
{
  if (!std::isfinite(progress) || progress < 0.0 || progress > 1.0)
    return std::nullopt;
  if (phase == ChamferPhase::Machining)
    return evaluate(2.0 * GeomConst::Pi * progress);
  if (phase != ChamferPhase::LeadIn && phase != ChamferPhase::LeadOut)
    return std::nullopt;

  const bool entering = phase == ChamferPhase::LeadIn;
  const auto& lead = entering ? m_parameters.leadIn : m_parameters.leadOut;
  const double span = lead.spanDegrees * GeomConst::DegToRad;
  const double angle = entering ? (progress - 1.0) * span
                                : 2.0 * GeomConst::Pi + progress * span;
  auto result = evaluate(angle);
  if (!result) return std::nullopt;
  const double smooth = progress * progress * (3.0 - 2.0 * progress);
  const double clearWeight = entering ? 1.0 - smooth : smooth;
  // Preserve the exact shared machining poses, including their rotation entries.
  if (clearWeight == 0.0) return result;

  const V3d clearY = -m_outwardAxis;
  const V3d clearZ = -result->radialNormal;
  const V3d clearX = clearY.cross(clearZ).normalized();
  const M3d clearRotation = basis2rot({clearX, clearY, clearZ});
  const Eigen::Quaterniond machining(M3d(result->tcp.block<3, 3>(0, 0)));
  Eigen::Quaterniond clear(clearRotation);
  // Hemisphere alignment avoids quaternion sign changes across the ellipse seam.
  if (machining.dot(clear) < 0.0) clear.coeffs() *= -1.0;
  result->tcp.block<3, 3>(0, 0) = clearWeight == 1.0 ? clearRotation
      : machining.slerp(clearWeight, clear).normalized().toRotationMatrix();
  result->tcp.block<3, 1>(0, 3) += (lead.clearance * clearWeight) * m_outwardAxis;
  if (!result->tcp.allFinite()) return std::nullopt;
  return result;
}

namespace {

class ChamferSampler
{
public:
  ChamferSampler(const ChamferPath& path, const ChamferSamplingParameters& parameters)
    : m_path(path), m_parameters(parameters) {}

  ChamferSamplingResult run()
  {
    const auto& inputs = m_path.parameters();
    const std::array<int, 3> counts{inputs.leadIn.intervals, 16, inputs.leadOut.intervals};
    const std::array<ChamferPhase, 3> phases{
        ChamferPhase::LeadIn, ChamferPhase::Machining, ChamferPhase::LeadOut};
    for (std::size_t index = 0; index < phases.size(); ++index) {
      m_phase = phases[index];
      auto first = point(0.0);
      if (!first) return failure();
      if (m_output.points.isEmpty()) m_output.points.append(*first);
      for (int i = 1; i <= counts[index]; ++i) {
        const auto last = point(static_cast<double>(i) / counts[index]);
        if (!last || !appendInterval(*first, *last, 0)) return failure();
        first = last;
      }
      m_output.boundaries[index + 1] = m_output.points.size() - 1;
    }
    return {std::move(m_output), {}};
  }

private:
  std::optional<ChamferCurvePoint> point(double progress) const
  {
    const auto pose = m_path.evaluatePhase(m_phase, progress);
    if (!pose) return std::nullopt;
    return ChamferCurvePoint{progress, *pose};
  }

  bool withinTolerance(const ChamferCurvePoint& first, const ChamferCurvePoint& last) const
  {
    const Eigen::Quaterniond start(M3d(first.sample.tcp.block<3, 3>(0, 0)));
    const Eigen::Quaterniond end(M3d(last.sample.tcp.block<3, 3>(0, 0)));
    // Also bound angular steps, so large rotations cannot alias a midpoint check.
    if (start.angularDistance(end) > 5.0 * GeomConst::DegToRad) return false;
    for (double fraction : {0.25, 0.5, 0.75}) {
      const auto actual = point(first.progress + fraction * (last.progress - first.progress));
      if (!actual) return false;
      const V3d linear = (1.0 - fraction) * first.sample.tcp.block<3, 1>(0, 3)
          + fraction * last.sample.tcp.block<3, 1>(0, 3);
      const Eigen::Quaterniond rotation(M3d(actual->sample.tcp.block<3, 3>(0, 0)));
      if ((actual->sample.tcp.block<3, 1>(0, 3) - linear).stableNorm()
              > m_parameters.positionTolerance
          || start.slerp(fraction, end).angularDistance(rotation)
              > m_parameters.orientationToleranceDegrees * GeomConst::DegToRad)
        return false;
    }
    return true;
  }

  // Endpoint pair defines the interval; depth bounds adaptive subdivision.
  bool appendInterval(const ChamferCurvePoint& first, const ChamferCurvePoint& last, int depth)
  {
    if (m_output.points.size() >= 200000) return false;
    if (withinTolerance(first, last)) {
      m_output.points.append(last);
      return true;
    }
    if (depth >= 20) return false;
    const auto middle = point(0.5 * (first.progress + last.progress));
    return middle && appendInterval(first, *middle, depth + 1)
        && appendInterval(*middle, last, depth + 1);
  }

  ChamferSamplingResult failure() const
  {
    return {{}, QStringLiteral("Cannot sample chamfer phase %1: invalid pose or refinement limit reached.")
                    .arg(static_cast<int>(m_phase))};
  }

  const ChamferPath& m_path;
  const ChamferSamplingParameters& m_parameters;
  ChamferPhase m_phase = ChamferPhase::LeadIn;
  ChamferSampledPath m_output;
};

} // namespace

ChamferSamplingResult ChamferPath::sample(const ChamferSamplingParameters& parameters) const
{
  if (!std::isfinite(parameters.positionTolerance) || parameters.positionTolerance <= 0.0
      || !std::isfinite(parameters.orientationToleranceDegrees)
      || parameters.orientationToleranceDegrees <= 0.0)
    return {{}, QStringLiteral("Sampling tolerances must be finite and positive.")};
  return ChamferSampler(*this, parameters).run();
}
