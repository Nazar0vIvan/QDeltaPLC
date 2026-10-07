#include "rsiposeencoder.h"

#include "geometry/utils.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <utility>

namespace {

using namespace RoboCrap3D;

bool isRigidPose(const M4d& pose)
{
  return pose.allFinite()
      && isBasis(pose.block<3, 1>(0, 0), pose.block<3, 1>(0, 1), pose.block<3, 1>(0, 2), Kr10RotEps)
      && (pose.row(3) - Eigen::RowVector4d(0.0, 0.0, 0.0, 1.0)).cwiseAbs().maxCoeff() <= Kr10RotEps;
}

bool isCentralSample(const ChamferCartesianSample& sample)
{
  return std::isfinite(sample.time) && std::isfinite(sample.progress)
      && sample.progress >= 0.0 && sample.progress <= 1.0
      && sample.pose.phase >= ChamferMotionPhase::LeadIn
      && sample.pose.phase <= ChamferMotionPhase::LeadOut && isRigidPose(sample.pose.tcp);
}

double unwrapDegrees(double angle, double previous)
{
  return angle + 360.0 * std::round((previous - angle) / 360.0);
}

V3d unwrapEulerDegrees(const V3d& angles, const V3d& previous)
{
  V3d result = V3d::Zero();
  for (int axis = 0; axis < 3; ++axis)
    result[axis] = unwrapDegrees(angles[axis], previous[axis]);
  return result;
}

V3d principalEulerDegrees(const M3d& rotation)
{
  // atan2 retains pitch accuracy near +/-90 degrees, where asin can lose it.
  const double cosinePitch = std::hypot(rotation(0, 0), rotation(1, 0));
  if (cosinePitch <= GeomConst::Eps)
    return {std::atan2(-rotation(0, 1), rotation(1, 1)) * GeomConst::RadToDeg,
            std::copysign(90.0, -rotation(2, 0)), 0.0};
  return {std::atan2(rotation(1, 0), rotation(0, 0)) * GeomConst::RadToDeg,
          std::atan2(-rotation(2, 0), cosinePitch) * GeomConst::RadToDeg,
          std::atan2(rotation(2, 1), rotation(2, 2)) * GeomConst::RadToDeg};
}

double rotationEntryError(const M3d& target, const V3d& angles)
{
  const M3d reconstructed = euler2rot(angles[0], angles[1], angles[2]);
  return (reconstructed - target).cwiseAbs().maxCoeff();
}

std::optional<V3d> continuousEulerDegrees(const M3d& rotation, const V3d& previous)
{
  const V3d principal = principalEulerDegrees(rotation);
  V3d result = V3d::Zero();
  if (std::hypot(rotation(0, 0), rotation(1, 0)) <= GeomConst::Eps) {
    // At B=+90, A-C is fixed; at B=-90, A+C is fixed. Share its change
    // between A/C to remain closest to the preceding serialized attitude.
    const double sign = std::copysign(1.0, -rotation(2, 0));
    const double previousCoupled = previous[0] - sign * previous[2];
    const double change = unwrapDegrees(principal[0], previousCoupled) - previousCoupled;
    result << previous[0] + 0.5 * change, unwrapDegrees(principal[1], previous[1]),
              previous[2] - sign * 0.5 * change;
  } else {
    const V3d first = unwrapEulerDegrees(principal, previous);
    const V3d second = unwrapEulerDegrees(
        V3d(principal[0] + 180.0, 180.0 - principal[1], principal[2] + 180.0), previous);
    result = (first - previous).squaredNorm() <= (second - previous).squaredNorm()
        ? first : second;
  }
  // Reject an unresolved Euler branch rather than emit a large coordinate jump.
  // This is an encoding-continuity guard, not a configured controller limit.
  if (!result.allFinite() || (result - previous).cwiseAbs().maxCoeff() >= 90.0
      || rotationEntryError(rotation, result) > Kr10RotEps) return std::nullopt;
  return result;
}

struct PoseEncodingResult
{
  std::optional<RsiPoseCorrection> correction;
  QString error;
};

class AdditiveTcpPoseEncoder
{
public:
  explicit AdditiveTcpPoseEncoder(const V6d& start) : m_start(start) {}

  PoseEncodingResult encode(const M4d& target)
  {
    const V3d previousAngles = m_start.tail<3>() + m_encodedOffset.tail<3>();
    const auto angles = continuousEulerDegrees(M3d(target.block<3, 3>(0, 0)), previousAngles);
    if (!angles)
      return {{}, QStringLiteral("Cannot unwrap TCP A/B/C continuously for RSI encoding.")};
    V6d coordinates = V6d::Zero();
    coordinates.head<3>() = target.block<3, 1>(0, 3);
    coordinates.tail<3>() = *angles;
    const V6d desiredOffset = coordinates - m_start;
    const V6d increment = desiredOffset - m_encodedOffset;
    if (!desiredOffset.allFinite() || !increment.allFinite())
      return {{}, QStringLiteral("Invalid nominal RSI correction.")};
    RsiPoseCorrection correction;
    for (int axis = 0; axis < 6; ++axis) {
      // QString number conversion always uses the C locale. Parsing the generated
      // text reconciles host cumulative state with the serialized values.
      correction.attributes[axis] = QString::number(increment[axis], 'g',
                                                    std::numeric_limits<double>::max_digits10);
      correction.increment[axis] = correction.attributes[axis].toDouble();
    }
    const V6d encodedOffset = m_encodedOffset + correction.increment;
    const V6d reconstructed = m_start + encodedOffset;
    if (!correction.increment.allFinite() || !encodedOffset.allFinite()
        || !reconstructed.allFinite()
        || (reconstructed.head<3>() - coordinates.head<3>()).norm() > Kr10PosEps
        || (reconstructed.tail<3>() - *angles).cwiseAbs().maxCoeff() > Kr10RotEps * GeomConst::RadToDeg
        || rotationEntryError(M3d(target.block<3, 3>(0, 0)), reconstructed.tail<3>()) > Kr10RotEps)
      return {{}, QStringLiteral("Cannot serialize/reconstruct the RSI TCP correction.")};
    m_encodedOffset = encodedOffset;
    return {std::move(correction), {}};
  }

private:
  V6d m_start = V6d::Zero();
  V6d m_encodedOffset = V6d::Zero();
};

// The values, configured bounds, diagnostic quantity and original sample index
// are distinct inputs to one bound comparison.
std::optional<RsiCorrectionLimitViolation> findBoundViolation(
    const V6d& values, const RsiCorrectionBounds& bounds, const QString& quantity, qsizetype sampleIndex)
{
  constexpr std::array<char, 6> axes{'X', 'Y', 'Z', 'A', 'B', 'C'};
  for (int axis = 0; axis < 6; ++axis) {
    if (values[axis] < bounds.minimum[axis] || values[axis] > bounds.maximum[axis])
      return RsiCorrectionLimitViolation{sampleIndex, quantity + QLatin1Char(' ') + QLatin1Char(axes[axis]),
                                        axis < 3 ? QStringLiteral("mm") : QStringLiteral("deg"),
                                        values[axis], bounds.minimum[axis], bounds.maximum[axis]};
  }
  return std::nullopt;
}

RsiCorrectionBounds posCorrBounds(const RsiPosCorrLimits& limits)
{
  RsiCorrectionBounds result;
  result.minimum.head<3>() = limits.minimumTranslation;
  result.maximum.head<3>() = limits.maximumTranslation;
  result.minimum.tail<3>().setConstant(-limits.maximumRotationDegrees);
  result.maximum.tail<3>().setConstant(limits.maximumRotationDegrees);
  return result;
}

RsiCorrectionBounds posCorrMonBounds(const RsiPosCorrMonLimits& limits)
{
  RsiCorrectionBounds result;
  result.minimum.head<3>().setConstant(-limits.maximumTranslationMillimetres);
  result.maximum.head<3>().setConstant(limits.maximumTranslationMillimetres);
  result.minimum.tail<3>().setConstant(-limits.maximumRotationDegrees);
  result.maximum.tail<3>().setConstant(limits.maximumRotationDegrees);
  return result;
}

RsiCorrectionLimitValidation validateCorrectionLimits(const RsiNominalEncoding& encoding)
{
  const auto& configuration = encoding.limitConfiguration;
  if (!configuration.profile || !configuration.error.isEmpty()) {
    const QString reason = configuration.error.isEmpty()
        ? QStringLiteral("Configure RSI limits in rsi-correction-limits.json.") : configuration.error;
    return {configuration.supplied || configuration.profile
                ? RsiCorrectionLimitStatus::InvalidConfiguration : RsiCorrectionLimitStatus::Unconfigured,
            {}, reason};
  }
  const auto& profile = *configuration.profile;
  const QString invalid = validateRsiCorrectionLimitProfile(profile);
  if (!invalid.isEmpty()) return {RsiCorrectionLimitStatus::InvalidConfiguration, {}, invalid};
  const auto objectBounds = posCorrBounds(profile.poscorr);
  const auto monitorBounds = posCorrMonBounds(profile.poscorrmon);
  V6d cumulative = V6d::Zero();
  for (qsizetype index = 0; index < encoding.corrections.size(); ++index) {
    V6d increment = V6d::Zero();
    for (int axis = 0; axis < 6; ++axis)
      increment[axis] = encoding.corrections[index].attributes[axis].toDouble();
    cumulative += increment;
    if (!increment.allFinite() || !cumulative.allFinite())
      return {RsiCorrectionLimitStatus::InvalidConfiguration, {},
              QStringLiteral("Nonfinite serialized RSI correction at sample %1.").arg(index + 1)};
    auto violation = findBoundViolation(increment, profile.increment,
                                        QStringLiteral("Increment"), index + 1);
    if (!violation)
      violation = findBoundViolation(cumulative, profile.cumulative,
                                      QStringLiteral("Cumulative"), index + 1);
    if (!violation)
      violation = findBoundViolation(cumulative, objectBounds, QStringLiteral("POSCORR"), index + 1);
    if (!violation)
      violation = findBoundViolation(cumulative, monitorBounds, QStringLiteral("POSCORRMON"), index + 1);
    if (violation)
      return {RsiCorrectionLimitStatus::Exceeded, violation, describeRsiCorrectionLimitViolation(*violation)};
  }
  return {RsiCorrectionLimitStatus::Passed, {}, {}};
}

class NominalEncodingCompiler
{
public:
  NominalEncodingCompiler(const QVector<ChamferCartesianSample>& samples,
                          const RsiCorrectionLimitConfiguration& limits)
    : m_samples(samples), m_limits(limits) {}

  RsiNominalEncodingResult run()
  {
    if (m_samples.size() < 2 || !isCentralSample(m_samples.front())
        || m_samples.front().time != 0.0 || m_samples.front().terminal || !m_samples.back().terminal)
      return {{}, QStringLiteral("RSI encoding requires complete central samples and an initial pose.")};
    const M4d& initial = m_samples.front().pose.tcp;
    const Eigen::Quaterniond startRotation(M3d(initial.block<3, 3>(0, 0)));
    RsiNominalEncoding encoding;
    encoding.initialCoordinates.head<3>() = initial.block<3, 1>(0, 3);
    encoding.initialCoordinates.tail<3>() = principalEulerDegrees(M3d(initial.block<3, 3>(0, 0)));
    if (!encoding.initialCoordinates.allFinite()
        || rotationEntryError(M3d(initial.block<3, 3>(0, 0)), encoding.initialCoordinates.tail<3>()) > Kr10RotEps)
      return {{}, QStringLiteral("Cannot reconstruct the initial RSI TCP orientation.")};
    AdditiveTcpPoseEncoder encoder(encoding.initialCoordinates);
    encoding.corrections.reserve(m_samples.size() - 1);
    V6d reconstructedOffset = V6d::Zero();
    for (qsizetype index = 1; index < m_samples.size(); ++index) {
      const auto& sample = m_samples[index];
      const auto& previous = m_samples[index - 1];
      if (!isCentralSample(sample) || sample.time <= previous.time
          || sample.pose.phase < previous.pose.phase
          || (sample.terminal && index != m_samples.size() - 1))
        return {{}, atSample(QStringLiteral("Invalid central sample for RSI encoding."), index)};
      const auto encoded = encoder.encode(sample.pose.tcp);
      if (!encoded.correction) return {{}, atSample(encoded.error, index)};
      const auto& correction = *encoded.correction;
      // Reconstruct from the actual text, independently of the encoder's state.
      for (int axis = 0; axis < 6; ++axis)
        reconstructedOffset[axis] += correction.attributes[axis].toDouble();
      const V6d reconstructed = encoding.initialCoordinates + reconstructedOffset;
      const double error = (reconstructed.head<3>() - sample.pose.tcp.block<3, 1>(0, 3)).norm();
      const double rotationError = rotationEntryError(M3d(sample.pose.tcp.block<3, 3>(0, 0)),
                                                      reconstructed.tail<3>());
      if (!reconstructedOffset.allFinite() || !reconstructed.allFinite()
          || !std::isfinite(error) || error > Kr10PosEps
          || !std::isfinite(rotationError) || rotationError > Kr10RotEps)
        return {{}, atSample(QStringLiteral("Serialized RSI TCP pose exceeds reconstruction tolerance."), index)};
      const double incrementNorm = correction.increment.head<3>().norm();
      const double offsetNorm = reconstructedOffset.head<3>().norm();
      if (!std::isfinite(incrementNorm) || !std::isfinite(offsetNorm))
        return {{}, atSample(QStringLiteral("Invalid nominal RSI translation envelope."), index)};
      auto& envelope = encoding.envelope;
      envelope.minimumTranslationIncrement = envelope.minimumTranslationIncrement.cwiseMin(correction.increment.head<3>());
      envelope.maximumTranslationIncrement = envelope.maximumTranslationIncrement.cwiseMax(correction.increment.head<3>());
      envelope.minimumTranslationOffset = envelope.minimumTranslationOffset.cwiseMin(reconstructedOffset.head<3>());
      envelope.maximumTranslationOffset = envelope.maximumTranslationOffset.cwiseMax(reconstructedOffset.head<3>());
      envelope.minimumAngleIncrement = envelope.minimumAngleIncrement.cwiseMin(correction.increment.tail<3>());
      envelope.maximumAngleIncrement = envelope.maximumAngleIncrement.cwiseMax(correction.increment.tail<3>());
      envelope.minimumAngleOffset = envelope.minimumAngleOffset.cwiseMin(reconstructedOffset.tail<3>());
      envelope.maximumAngleOffset = envelope.maximumAngleOffset.cwiseMax(reconstructedOffset.tail<3>());
      envelope.maximumTranslationIncrementNorm = std::max(envelope.maximumTranslationIncrementNorm,
                                                         incrementNorm);
      envelope.maximumTranslationOffsetNorm = std::max(envelope.maximumTranslationOffsetNorm,
                                                      offsetNorm);
      envelope.maximumTranslationReconstructionError = std::max(envelope.maximumTranslationReconstructionError, error);
      envelope.maximumRotationReconstructionError = std::max(envelope.maximumRotationReconstructionError, rotationError);
      const Eigen::Quaterniond rotation(M3d(sample.pose.tcp.block<3, 3>(0, 0)));
      const Eigen::Quaterniond previousRotation(M3d(previous.pose.tcp.block<3, 3>(0, 0)));
      const double cycleAngle = previousRotation.angularDistance(rotation) * GeomConst::RadToDeg;
      const double offsetAngle = startRotation.angularDistance(rotation) * GeomConst::RadToDeg;
      envelope.maximumCycleRotationDegrees = std::max(envelope.maximumCycleRotationDegrees, cycleAngle);
      envelope.maximumOffsetRotationDegrees = std::max(envelope.maximumOffsetRotationDegrees, offsetAngle);
      envelope.rotationTravelDegrees += cycleAngle;
      if (!std::isfinite(cycleAngle) || !std::isfinite(offsetAngle)
          || !std::isfinite(envelope.rotationTravelDegrees))
        return {{}, atSample(QStringLiteral("Invalid nominal RSI rotation envelope."), index)};
      encoding.corrections.append(correction);
    }
    encoding.orientationRequired = encoding.envelope.maximumOffsetRotationDegrees > Kr10RotEps * GeomConst::RadToDeg;
    encoding.limitConfiguration = m_limits;
    encoding.limitValidation = validateCorrectionLimits(encoding);
    encoding.unavailableReason = encoding.limitValidation.reason;
    return {std::move(encoding), {}};
  }

private:
  QString atSample(const QString& error, qsizetype index) const
  {
    return QStringLiteral("%1 (sample %2)").arg(error).arg(index);
  }

  const QVector<ChamferCartesianSample>& m_samples;
  const RsiCorrectionLimitConfiguration& m_limits;
};

} // namespace

RsiNominalEncodingResult prepareRsiNominalEncoding(const QVector<ChamferCartesianSample>& samples,
                                                  const RsiCorrectionLimitConfiguration& limits)
{
  return NominalEncodingCompiler(samples, limits).run();
}
