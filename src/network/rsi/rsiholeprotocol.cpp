#include "rsiholeprotocol.h"

#include "geometry/utils.h"
#include "3d/robot/kinematics/kr10kinematics.h"

#include <QXmlStreamReader>
#include <QXmlStreamWriter>
#include <algorithm>
#include <cmath>
#include <utility>

namespace {

constexpr std::array<const char*, 6> CoordinateNames{"X", "Y", "Z", "A", "B", "C"};
constexpr qsizetype MaximumPacketBytes = 16384;

std::optional<int> readNonnegativeInt(const QString& text)
{
  if (text.isEmpty()) return std::nullopt;
  for (const QChar character : text) {
    if (character < QLatin1Char('0') || character > QLatin1Char('9')) return std::nullopt;
  }
  bool ok = false;
  const int value = text.toInt(&ok);
  return ok ? std::optional<int>(value) : std::nullopt;
}

std::optional<RsiHoleStatus> readStatus(const QXmlStreamAttributes& attributes)
{
  const auto job = readNonnegativeInt(attributes.value(QLatin1String("Job")).toString());
  const auto sample = readNonnegativeInt(attributes.value(QLatin1String("Sample")).toString());
  const auto result = readNonnegativeInt(attributes.value(QLatin1String("Result")).toString());
  if (!job || !sample || !result || *result > static_cast<int>(RsiHoleResult::Faulted))
    return std::nullopt;
  return RsiHoleStatus{*job, *sample, static_cast<RsiHoleResult>(*result)};
}

std::optional<RsiPoseCorrection> readCorrection(const QXmlStreamAttributes& attributes)
{
  RsiPoseCorrection correction;
  for (int axis = 0; axis < 6; ++axis) {
    correction.attributes[axis] = attributes.value(QLatin1String(CoordinateNames[axis])).toString();
    bool ok = false;
    correction.increment[axis] = correction.attributes[axis].toDouble(&ok);
    if (!ok || !std::isfinite(correction.increment[axis])) return std::nullopt;
  }
  return correction;
}

bool hasValidCorrection(const RsiPoseCorrection& correction)
{
  for (int axis = 0; axis < 6; ++axis) {
    bool ok = false;
    const double parsed = correction.attributes[axis].toDouble(&ok);
    if (!ok || !std::isfinite(parsed) || parsed != correction.increment[axis]) return false;
  }
  return true;
}

bool hasValidStatus(const RsiHoleStatus& status)
{
  const int result = static_cast<int>(status.result);
  return status.job >= 0 && status.sample >= 0 && result >= 0
      && result <= static_cast<int>(RsiHoleResult::Faulted);
}

struct ParsedReply
{
  RsiPoseCorrection correction;
  RsiHoleStatus status;
  bool stop = false;
  quint64 ipoc = 0;
};

std::optional<ParsedReply> readReply(const QByteArray& packet)
{
  QXmlStreamReader xml(packet);
  if (!xml.readNextStartElement() || xml.name() != QLatin1String("Sen")
      || xml.attributes().value(QLatin1String("Type")) != QLatin1String("ImFree"))
    return std::nullopt;
  ParsedReply reply;
  bool hasCorrection = false;
  bool hasStatus = false;
  bool hasStop = false;
  bool hasIpoc = false;
  while (xml.readNextStartElement()) {
    if (xml.name() == QLatin1String("RKorr")) {
      const auto correction = readCorrection(xml.attributes());
      if (hasCorrection || !correction) return std::nullopt;
      reply.correction = *correction;
      hasCorrection = true;
      xml.skipCurrentElement();
    } else if (xml.name() == QLatin1String("State")) {
      const auto status = readStatus(xml.attributes());
      if (hasStatus || !status) return std::nullopt;
      reply.status = *status;
      hasStatus = true;
      xml.skipCurrentElement();
    } else if (xml.name() == QLatin1String("Stop")) {
      const auto value = readNonnegativeInt(xml.readElementText());
      if (hasStop || !value || *value > 1) return std::nullopt;
      reply.stop = *value == 1;
      hasStop = true;
    } else if (xml.name() == QLatin1String("IPOC")) {
      const QString text = xml.readElementText();
      if (hasIpoc || text.isEmpty()) return std::nullopt;
      for (const QChar character : text) {
        if (character < QLatin1Char('0') || character > QLatin1Char('9')) return std::nullopt;
      }
      bool ok = false;
      reply.ipoc = text.toULongLong(&ok);
      if (!ok) return std::nullopt;
      hasIpoc = true;
    } else {
      return std::nullopt;
    }
  }
  while (!xml.atEnd()) xml.readNext();
  if (xml.hasError() || !hasCorrection || !hasStatus || !hasStop || !hasIpoc) return std::nullopt;
  return reply;
}

QString sampleError(const QString& error, qsizetype index)
{
  return QStringLiteral("%1 (sample %2)").arg(error).arg(index);
}

double positionError(const V6d& coordinates, const M4d& target)
{
  return (coordinates.head<3>() - target.block<3, 1>(0, 3)).norm();
}

double rotationError(const V6d& coordinates, const M4d& target)
{
  return (euler2rot(coordinates[3], coordinates[4], coordinates[5])
          - target.block<3, 3>(0, 0)).cwiseAbs().maxCoeff();
}

} // namespace

RsiPoseCorrection RsiHoleProtocol::zeroCorrection()
{
  RsiPoseCorrection correction;
  correction.attributes.fill(QStringLiteral("0"));
  return correction;
}

RsiHolePacketResult RsiHoleProtocol::encode(const RsiHoleReply& reply)
{
  if (!hasValidStatus(reply.status) || !hasValidCorrection(reply.correction)
      || (reply.stop && reply.status.result == RsiHoleResult::None)
      || ((reply.stop || reply.status.result != RsiHoleResult::None
           || reply.status.job == 0 || reply.status.sample == 0)
          && !reply.correction.increment.isZero(0.0)))
    return {{}, QStringLiteral("Invalid RSI_Hole correction or terminal state.")};
  QByteArray packet;
  QXmlStreamWriter xml(&packet);
  xml.writeStartElement(QStringLiteral("Sen"));
  xml.writeAttribute(QStringLiteral("Type"), QStringLiteral("ImFree"));
  xml.writeEmptyElement(QStringLiteral("RKorr"));
  for (int axis = 0; axis < 6; ++axis)
    xml.writeAttribute(QLatin1String(CoordinateNames[axis]), reply.correction.attributes[axis]);
  xml.writeTextElement(QStringLiteral("Stop"), reply.stop ? QStringLiteral("1") : QStringLiteral("0"));
  xml.writeEmptyElement(QStringLiteral("State"));
  xml.writeAttribute(QStringLiteral("Job"), QString::number(reply.status.job));
  xml.writeAttribute(QStringLiteral("Sample"), QString::number(reply.status.sample));
  xml.writeAttribute(QStringLiteral("Result"), QString::number(static_cast<int>(reply.status.result)));
  xml.writeTextElement(QStringLiteral("IPOC"), QString::number(reply.ipoc));
  xml.writeEndElement();
  if (xml.hasError() || packet.size() > MaximumPacketBytes)
    return {{}, QStringLiteral("Cannot serialize RSI_Hole reply.")};
  return {std::move(packet), {}};
}

RsiHoleDecodeResult RsiHoleProtocol::decode(const QByteArray& packet)
{
  if (packet.size() > MaximumPacketBytes)
    return {{}, QStringLiteral("RSI_Hole request is too large.")};
  // Reuse the established framing, IPOC and finite robot-telemetry validation.
  const auto robot = RsiProtocol::decode(packet);
  if (!robot || !robot->pose)
    return {{}, QStringLiteral("RSI_Hole requires valid IPOC and RIst.")};
  QXmlStreamReader xml(packet);
  if (!xml.readNextStartElement()
      || xml.attributes().value(QLatin1String("TYPE")) != QLatin1String("KUKA"))
    return {{}, QStringLiteral("Invalid RSI_Hole controller type.")};
  std::optional<RsiHoleStatus> status;
  bool ready = false;
  int correctionStatus = 0;
  while (xml.readNextStartElement()) {
    if (xml.name() == QLatin1String("State")) {
      const auto parsed = readStatus(xml.attributes());
      const auto gate = readNonnegativeInt(xml.attributes().value(QLatin1String("Ready")).toString());
      const auto correction = readNonnegativeInt(
          xml.attributes().value(QLatin1String("CorrectionStatus")).toString());
      if (status || !parsed || !gate || *gate > 1 || !correction)
        return {{}, QStringLiteral("Invalid or duplicate RSI_Hole State.")};
      status = *parsed;
      ready = *gate == 1;
      correctionStatus = *correction;
    }
    xml.skipCurrentElement();
  }
  if (!status) return {{}, QStringLiteral("RSI_Hole controller State is missing.")};
  return {RsiHoleRequest{*robot, *status, ready, correctionStatus}, {}};
}

RsiHoleNominalAuditResult RsiHoleProtocol::auditNominalJob(const PreparedChamferJob& job)
{
  using namespace RoboCrap3D;
  const auto& prepared = job.encoding();
  if (!prepared.data)
    return {{}, prepared.error.isEmpty() ? QStringLiteral("Nominal RSI encoding is missing.") : prepared.error};
  const auto& encoding = *prepared.data;
  const auto& samples = job.samples();
  if (samples.size() < 2 || encoding.corrections.size() != samples.size() - 1
      || job.cyclePeriod() != 0.004 || samples.front().time != 0.0
      || samples.front().terminal || !samples.back().terminal
      || !encoding.initialCoordinates.allFinite() || !samples.front().pose.tcp.allFinite()
      || !std::isfinite(job.duration()) || job.duration() <= 0.0
      || std::abs(samples.back().time - job.duration()) > GeomConst::Eps)
    return {{}, QStringLiteral("Incomplete nominal RSI_Hole sample/encoding correspondence.")};
  RsiHoleNominalAudit report;
  report.initialCoordinates = encoding.initialCoordinates;
  report.finalCoordinates = report.initialCoordinates;
  report.savedDuration = job.duration();
  report.controllerDuration = encoding.corrections.size() * job.cyclePeriod();
  report.limitStatus = encoding.limitValidation.status;
  report.maximumPositionError = positionError(report.initialCoordinates, samples.front().pose.tcp);
  report.maximumRotationError = rotationError(report.initialCoordinates, samples.front().pose.tcp);
  if (!std::isfinite(report.maximumPositionError) || report.maximumPositionError > Kr10PosEps
      || !std::isfinite(report.maximumRotationError) || report.maximumRotationError > Kr10RotEps)
    return {{}, QStringLiteral("RSI_Hole initial XYZABC does not reconstruct sample zero.")};
  V6d accumulated = V6d::Zero();
  for (qsizetype index = 1; index < samples.size(); ++index) {
    const auto& sample = samples[index];
    const double clockTime = index * job.cyclePeriod();
    if (!sample.pose.tcp.allFinite() || !std::isfinite(sample.time)
        || sample.time <= samples[index - 1].time
        || (!sample.terminal && std::abs(sample.time - clockTime) > GeomConst::Eps)
        || (sample.terminal && (index != samples.size() - 1
            || sample.time > clockTime + GeomConst::Eps
            || sample.time <= clockTime - job.cyclePeriod())))
      return {{}, sampleError(QStringLiteral("Invalid nominal controller-clock target."), index)};
    // IPOC/job values here are offline audit labels, never controller timestamps.
    const RsiHoleReply reply{static_cast<quint64>(index), encoding.corrections[index - 1],
                            {1, static_cast<int>(index), RsiHoleResult::None}, false};
    const auto encoded = encode(reply);
    if (!encoded.error.isEmpty()) return {{}, sampleError(encoded.error, index)};
    const auto parsed = readReply(encoded.packet);
    if (!parsed || parsed->ipoc != reply.ipoc || parsed->status.job != reply.status.job
        || parsed->status.sample != reply.status.sample || parsed->status.result != RsiHoleResult::None
        || parsed->stop || parsed->correction.attributes != reply.correction.attributes)
      return {{}, sampleError(QStringLiteral("RSI_Hole XML changed a prepared correction."), index)};
    accumulated += parsed->correction.increment;
    report.finalCoordinates = report.initialCoordinates + accumulated;
    const double position = positionError(report.finalCoordinates, sample.pose.tcp);
    const double rotation = rotationError(report.finalCoordinates, sample.pose.tcp);
    if (!report.finalCoordinates.allFinite() || !std::isfinite(position) || position > Kr10PosEps
        || !std::isfinite(rotation) || rotation > Kr10RotEps)
      return {{}, sampleError(QStringLiteral("RSI_Hole wire increments do not reconstruct the nominal TCP."), index)};
    report.maximumPositionError = std::max(report.maximumPositionError, position);
    report.maximumRotationError = std::max(report.maximumRotationError, rotation);
    ++report.correctionCount;
  }
  return {std::move(report), {}};
}
