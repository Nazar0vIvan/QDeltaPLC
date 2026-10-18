#include "trajectories/chamferpath.h"

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
      && std::isfinite(lead.spanDegrees) && lead.spanDegrees > 0.0 && lead.spanDegrees <= 360.0;
}

struct LeadProgress
{
  double angle = 0.0; // Ellipse parameter, radians.
  double angleFirst = 0.0; // Derivatives with respect to dimensionless progress.
  double angleSecond = 0.0;
  double axial = 0.0; // mm, outward displacement from machining TCP.
  double axialFirst = 0.0;
  double axialSecond = 0.0;
};

// Lead dimensions, traversal side and progress define position and its derivatives.
LeadProgress leadProgress(const ChamferLeadParameters& lead, bool entering, double progress)
{
  const double rate = 0.5 * GeomConst::Pi;
  const double span = lead.spanDegrees * GeomConst::DegToRad;
  const double sine = progress == 1.0 ? 1.0 : std::sin(rate * progress);
  const double cosine = progress == 1.0 ? 0.0 : std::cos(rate * progress);
  if (entering)
    return {-span * cosine, span * rate * sine, span * rate * rate * cosine,
            lead.clearance * (1.0 - sine), -lead.clearance * rate * cosine,
            lead.clearance * rate * rate * sine};
  return {2.0 * GeomConst::Pi + span * sine, span * rate * cosine,
          -span * rate * rate * sine, lead.clearance * (1.0 - cosine),
          lead.clearance * rate * sine, lead.clearance * rate * rate * cosine};
}

V3d transverse(const V3d& vector, const V3d& axis)
{
  return vector - vector.dot(axis) * axis;
}

struct VectorDerivatives
{
  V3d value = V3d::Zero();
  V3d first = V3d::Zero();
  V3d second = V3d::Zero();
};

std::optional<VectorDerivatives> normalizedDerivatives(const VectorDerivatives& input)
{
  const double length = input.value.stableNorm();
  if (!std::isfinite(length) || length <= GeomConst::Eps) return std::nullopt;
  VectorDerivatives result;
  result.value = input.value / length;
  const double firstLength = result.value.dot(input.first);
  result.first = (input.first - firstLength * result.value) / length;
  const double secondLength = result.first.dot(input.first) + result.value.dot(input.second);
  result.second = (input.second - secondLength * result.value
                   - 2.0 * firstLength * result.first) / length;
  if (!result.value.allFinite() || !result.first.allFinite() || !result.second.allFinite())
    return std::nullopt;
  return result;
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
  const double chamferAngle = m_parameters.chamferAngleDegrees * GeomConst::DegToRad;
  m_chamferSine = std::sin(chamferAngle);
  m_chamferCosine = std::cos(chamferAngle);
  m_chamferTangent = std::tan(chamferAngle);
}

ChamferPathResult ChamferPath::create(ChamferPathParameters parameters)
{
  if (!validLead(parameters.leadIn) || !validLead(parameters.leadOut))
    return {{}, QStringLiteral("Leads require positive axial clearance and span in (0, 360] degrees.")};
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
  if (!std::isfinite(parameters.chamferAngleDegrees)
      || parameters.chamferAngleDegrees <= 0.0 || parameters.chamferAngleDegrees >= 90.0)
    return {{}, QStringLiteral("Chamfer angle must be between 0 and 90 degrees (exclusive).")};
  if (!std::isfinite(parameters.stagingDistance) || parameters.stagingDistance <= 0.0)
    return {{}, QStringLiteral("P_s axial distance must be finite and positive.")};
  const auto& edge = parameters.edge;
  if (std::abs(edge.majorAxis.cross(edge.minorAxis).dot(parameters.cylinderAxis))
      <= GeomConst::Eps)
    return {{}, QStringLiteral("The hole axis is parallel to the opening plane.")};
  const V3d planeNormal = edge.majorAxis.cross(edge.minorAxis).normalized();
  const double axialCosine = std::abs(planeNormal.dot(parameters.cylinderAxis));
  const double maximumSlope = planeNormal.cross(parameters.cylinderAxis).stableNorm() / axialCosine;
  const double cotangent = 1.0 / std::tan(parameters.chamferAngleDegrees * GeomConst::DegToRad);
  if (cotangent <= maximumSlope + FrameTolerance)
    return {{}, QStringLiteral("Chamfer angle is too large for the opening plane tilt; reduce the angle.")};
  if (!matchesCylinder(parameters))
    return {{}, QStringLiteral("The edge does not lie on the specified cylindrical hole.")};
  const V3d staging = edge.center + (parameters.flipAxis ? -1.0 : 1.0)
      * parameters.stagingDistance * parameters.cylinderAxis;
  if (!staging.allFinite())
    return {{}, QStringLiteral("P_s position exceeds the numerical range.")};
  ChamferPath path(std::move(parameters));
  const QString error = path.initializeLeadFrames();
  if (!error.isEmpty()) return {{}, error};
  return {std::move(path), {}};
}

QString ChamferPath::initializeLeadFrames()
{
  const auto leadIn = evaluate(0.0);
  const auto leadOut = evaluate(2.0 * GeomConst::Pi);
  if (!leadIn || !leadOut) return QStringLiteral("Cannot construct lead junction frames.");
  m_leadIn = *leadIn;
  m_leadOut = *leadOut;
  for (const auto phase : {ChamferPhase::LeadIn, ChamferPhase::LeadOut})
    for (double progress : {0.0, 1.0})
      if (!evaluatePhase(phase, progress) || !phaseArcDifferential(phase, progress))
        return QStringLiteral("Spatial lead endpoint or differential exceeds numerical range.");
  return {};
}

V3d ChamferPath::stagingPoint() const
{
  // The ellipse center is the intersection of the end plane and cylinder axis.
  return m_parameters.edge.center + m_parameters.stagingDistance * m_outwardAxis;
}

std::optional<M4d> ChamferPath::stagingPose(ChamferPhase lead) const
{
  if (lead != ChamferPhase::LeadIn && lead != ChamferPhase::LeadOut)
    return std::nullopt;
  const auto endpoint = evaluatePhase(lead, lead == ChamferPhase::LeadIn ? 0.0 : 1.0);
  if (!endpoint) return std::nullopt;
  // Copy the free-end rotation exactly; only the origin changes for the LIN endpoint.
  M4d pose = endpoint->tcp;
  pose.block<3, 1>(0, 3) = stagingPoint();
  return pose;
}

std::optional<ChamferPathSample> ChamferPath::evaluate(double angleRad) const
{
  return machiningSample(angleRad, true);
}

std::optional<ChamferPathSample> ChamferPath::machiningSample(double angleRad,
                                                           bool includeOrientation) const
{
  if (!std::isfinite(angleRad)) return std::nullopt;
  const auto& edge = m_parameters.edge;
  const double angle = m_direction * std::remainder(angleRad, 2.0 * GeomConst::Pi);
  const double cosine = std::cos(angle);
  const double sine = std::sin(angle);
  const V3d offset = edge.majorRadius * cosine * edge.majorAxis
      + edge.minorRadius * sine * edge.minorAxis;
  // The center is on the axis; avoid subtracting large world coordinates here.
  const V3d radial = transverse(offset, m_outwardAxis);
  const double radialLength = radial.stableNorm();
  if (!std::isfinite(radialLength) || radialLength <= GeomConst::Eps)
    return std::nullopt;
  const V3d r = radial / radialLength;
  const V3d planeNormal = edge.majorAxis.cross(edge.minorAxis);
  // In the longitudinal (r,k) section, this direction lies on the end plane
  // and points away from the hole. c measures length along the end plane.
  const auto faceDirection = normalize(r - (planeNormal.dot(r)
      / planeNormal.dot(m_outwardAxis)) * m_outwardAxis);
  if (!faceDirection) return std::nullopt;
  const V3d localX = r.cross(m_outwardAxis).normalized();
  const V3d localY = m_chamferSine * r + m_chamferCosine * m_outwardAxis;
  const V3d localZ = localX.cross(localY).normalized();
  if (localZ.dot(m_outwardAxis) <= 0.0 || localZ.dot(r) >= 0.0
      || localY.dot(m_outwardAxis) <= 0.0
      || !isBasis(localX, localY, localZ, FrameTolerance))
    return std::nullopt;

  ChamferPathSample sample;
  sample.edgePoint = edge.center + offset;
  sample.radialNormal = r;
  // Join p+c*u on the plane to p-depth*k on the wall at the requested angle.
  const double depthRatio = faceDirection->dot(r) / m_chamferTangent
      - faceDirection->dot(m_outwardAxis);
  if (!std::isfinite(depthRatio) || depthRatio <= 0.0) return std::nullopt;
  const V3d origin = sample.edgePoint
      + (0.5 * m_parameters.chamferSize) * (*faceDirection - depthRatio * m_outwardAxis);
  if (!sample.edgePoint.allFinite() || !origin.allFinite()) return std::nullopt;
  if (!includeOrientation) {
    sample.tcp.block<3, 1>(0, 3) = origin;
    return sample;
  }
  const M4d localFrame = makeTransform(basis2rot({localX, localY, localZ}), origin);
  // The drawing specifies coincident origins, X_T=X_i, Y_T=-Y_i, Z_T=-Z_i.
  const M4d localToTcp = makeRotation(180.0, Axis::X);
  sample.tcp = localFrame * localToTcp;
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
  if ((entering && progress == 1.0) || (!entering && progress == 0.0))
    return entering ? m_leadIn : m_leadOut;
  const auto& lead = entering ? m_parameters.leadIn : m_parameters.leadOut;
  const auto law = leadProgress(lead, entering, progress);
  auto result = machiningSample(law.angle, false);
  if (!result) return std::nullopt;
  result->tcp.block<3, 1>(0, 3) += law.axial * m_outwardAxis;
  const double quarterTurn = 0.5 * GeomConst::Pi * progress;
  const double tiltFraction = entering ? std::sin(quarterTurn)
      : progress == 1.0 ? 0.0 : std::cos(quarterTurn);
  const double tilt = m_parameters.chamferAngleDegrees * GeomConst::DegToRad * tiltFraction;
  const V3d tcpX = result->radialNormal.cross(m_outwardAxis).normalized();
  const V3d tcpY = (-std::sin(tilt) * result->radialNormal
                   - std::cos(tilt) * m_outwardAxis).normalized();
  const V3d tcpZ = tcpX.cross(tcpY).normalized();
  result->tcp.block<3, 3>(0, 0) = basis2rot({tcpX, tcpY, tcpZ});
  if (!result->tcp.allFinite()
      || !isBasis(result->tcp.block<3, 1>(0, 0), result->tcp.block<3, 1>(0, 1),
                  result->tcp.block<3, 1>(0, 2), FrameTolerance)
      || result->tcp.block<3, 1>(0, 1).dot(m_outwardAxis) >= 0.0)
    return std::nullopt;
  return result;
}

std::optional<ChamferPath::PositionDerivatives> ChamferPath::machiningDerivatives(
    double angleRad) const
{
  if (!std::isfinite(angleRad)) return std::nullopt;
  const double angle = m_direction * std::remainder(angleRad, 2.0 * GeomConst::Pi);
  const double firstAngle = m_direction;
  const auto& edge = m_parameters.edge;
  const V3d offset = edge.majorRadius * std::cos(angle) * edge.majorAxis
      + edge.minorRadius * std::sin(angle) * edge.minorAxis;
  const V3d angularTangent = -edge.majorRadius * std::sin(angle) * edge.majorAxis
      + edge.minorRadius * std::cos(angle) * edge.minorAxis;
  const V3d firstOffset = firstAngle * angularTangent;
  const V3d secondOffset = -offset;
  const auto radial = normalizedDerivatives({transverse(offset, m_outwardAxis),
      transverse(firstOffset, m_outwardAxis), transverse(secondOffset, m_outwardAxis)});
  if (!radial) return std::nullopt;
  const V3d normal = edge.majorAxis.cross(edge.minorAxis);
  const double cosine = normal.dot(m_outwardAxis);
  // Same normalized end-plane section direction as evaluate().
  const auto face = normalizedDerivatives({
      radial->value - (normal.dot(radial->value) / cosine) * m_outwardAxis,
      radial->first - (normal.dot(radial->first) / cosine) * m_outwardAxis,
      radial->second - (normal.dot(radial->second) / cosine) * m_outwardAxis});
  if (!face) return std::nullopt;
  const double cotangent = 1.0 / m_chamferTangent;
  const double firstDepth = cotangent * (face->first.dot(radial->value)
      + face->value.dot(radial->first)) - face->first.dot(m_outwardAxis);
  const double secondDepth = cotangent * (face->second.dot(radial->value)
      + 2.0 * face->first.dot(radial->first) + face->value.dot(radial->second))
      - face->second.dot(m_outwardAxis);
  const double halfSize = 0.5 * m_parameters.chamferSize;
  const V3d first = firstOffset + halfSize * (face->first - firstDepth * m_outwardAxis);
  const V3d second = secondOffset + halfSize * (face->second - secondDepth * m_outwardAxis);
  if (!first.allFinite() || !second.allFinite()) return std::nullopt;
  return PositionDerivatives{first, second};
}

std::optional<ChamferArcDifferential> ChamferPath::phaseArcDifferential(
    ChamferPhase phase, double progress) const
{
  if (!std::isfinite(progress) || progress < 0.0 || progress > 1.0)
    return std::nullopt;
  double angle = 2.0 * GeomConst::Pi * progress;
  double angleFirst = 2.0 * GeomConst::Pi;
  double angleSecond = 0.0;
  double axialFirst = 0.0;
  double axialSecond = 0.0;
  if (phase == ChamferPhase::LeadIn || phase == ChamferPhase::LeadOut) {
    const bool entering = phase == ChamferPhase::LeadIn;
    const auto& lead = entering ? m_parameters.leadIn : m_parameters.leadOut;
    const auto law = leadProgress(lead, entering, progress);
    angle = law.angle;
    angleFirst = law.angleFirst;
    angleSecond = law.angleSecond;
    axialFirst = law.axialFirst;
    axialSecond = law.axialSecond;
  } else if (phase != ChamferPhase::Machining) return std::nullopt;
  const auto differential = machiningDerivatives(angle);
  if (!differential) return std::nullopt;
  const V3d first = angleFirst * differential->first + axialFirst * m_outwardAxis;
  const V3d second = (angleFirst * angleFirst) * differential->second
      + angleSecond * differential->first + axialSecond * m_outwardAxis;
  const double speed = first.stableNorm();
  if (!first.allFinite() || !second.allFinite() || !std::isfinite(speed)
      || speed <= GeomConst::Eps) return std::nullopt;
  const double derivative = (first / speed).dot(second);
  if (!std::isfinite(derivative)) return std::nullopt;
  return ChamferArcDifferential{speed, derivative};
}

