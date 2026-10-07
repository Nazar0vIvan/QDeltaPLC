#include "rsicorrectionlimits.h"

#include <QCryptographicHash>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStringList>
#include <cmath>
#include <limits>
#include <utility>

namespace {

constexpr qint64 kMaximumConfigurationBytes = 65536;

bool hasKeys(const QJsonObject& object, const QStringList& keys)
{
  if (object.size() != keys.size()) return false;
  for (const QString& key : keys)
    if (!object.contains(key)) return false;
  return true;
}

template<int Size>
std::optional<Eigen::Matrix<double, Size, 1>> readVector(const QJsonValue& value)
{
  if (!value.isArray()) return std::nullopt;
  const QJsonArray array = value.toArray();
  if (array.size() != Size) return std::nullopt;
  Eigen::Matrix<double, Size, 1> result = Eigen::Matrix<double, Size, 1>::Zero();
  for (int index = 0; index < Size; ++index) {
    if (!array[index].isDouble() || !std::isfinite(array[index].toDouble())) return std::nullopt;
    result[index] = array[index].toDouble();
  }
  return result;
}

std::optional<RsiCorrectionBounds> readBounds(const QJsonValue& value)
{
  if (!value.isObject()) return std::nullopt;
  const QJsonObject object = value.toObject();
  if (!hasKeys(object, {QStringLiteral("minimum"), QStringLiteral("maximum")})) return std::nullopt;
  const auto minimum = readVector<6>(object.value(QStringLiteral("minimum")));
  const auto maximum = readVector<6>(object.value(QStringLiteral("maximum")));
  if (!minimum || !maximum) return std::nullopt;
  return RsiCorrectionBounds{*minimum, *maximum};
}

bool validBounds(const RsiCorrectionBounds& bounds)
{
  return bounds.minimum.allFinite() && bounds.maximum.allFinite()
      && (bounds.minimum.array() <= 0.0).all() && (bounds.maximum.array() >= 0.0).all();
}

std::optional<RsiPosCorrLimits> readPosCorr(const QJsonValue& value)
{
  if (!value.isObject()) return std::nullopt;
  const QJsonObject object = value.toObject();
  if (!hasKeys(object, {QStringLiteral("minimumTranslation"), QStringLiteral("maximumTranslation"),
                        QStringLiteral("maxRotAngleDegrees")})) return std::nullopt;
  const auto minimum = readVector<3>(object.value(QStringLiteral("minimumTranslation")));
  const auto maximum = readVector<3>(object.value(QStringLiteral("maximumTranslation")));
  const QJsonValue rotation = object.value(QStringLiteral("maxRotAngleDegrees"));
  if (!minimum || !maximum || !rotation.isDouble()) return std::nullopt;
  return RsiPosCorrLimits{*minimum, *maximum, rotation.toDouble()};
}

std::optional<RsiPosCorrMonLimits> readPosCorrMon(const QJsonValue& value)
{
  if (!value.isObject()) return std::nullopt;
  const QJsonObject object = value.toObject();
  if (!hasKeys(object, {QStringLiteral("maxTransMillimetres"), QStringLiteral("maxRotAngleDegrees")}))
    return std::nullopt;
  const QJsonValue translation = object.value(QStringLiteral("maxTransMillimetres"));
  const QJsonValue rotation = object.value(QStringLiteral("maxRotAngleDegrees"));
  if (!translation.isDouble() || !rotation.isDouble()) return std::nullopt;
  return RsiPosCorrMonLimits{translation.toDouble(), rotation.toDouble()};
}

RsiCorrectionLimitConfiguration decodeConfiguration(const QByteArray& bytes)
{
  RsiCorrectionLimitConfiguration result;
  result.supplied = true;
  result.revision = QCryptographicHash::hash(bytes, QCryptographicHash::Sha256);
  const QJsonDocument document = QJsonDocument::fromJson(bytes);
  if (!document.isObject()) {
    result.error = QStringLiteral("RSI correction limits must be a JSON object.");
    return result;
  }
  const QJsonObject object = document.object();
  if (!hasKeys(object, {QStringLiteral("schemaVersion"), QStringLiteral("profileId"),
                        QStringLiteral("cyclePeriodSeconds"), QStringLiteral("reference"),
                        QStringLiteral("rotationRule"), QStringLiteral("increment"),
                        QStringLiteral("cumulative"), QStringLiteral("poscorr"), QStringLiteral("poscorrmon")})
      || !object.value(QStringLiteral("schemaVersion")).isDouble()
      || object.value(QStringLiteral("schemaVersion")).toDouble() != 1.0
      || !object.value(QStringLiteral("cyclePeriodSeconds")).isDouble()
      || object.value(QStringLiteral("cyclePeriodSeconds")).toDouble() != 0.004
      || object.value(QStringLiteral("reference")).toString() != QStringLiteral("BASE")
      || object.value(QStringLiteral("rotationRule")).toString() != QStringLiteral("startingTcpAngleAddition")) {
    result.error = QStringLiteral("RSI limits require schema 1, BASE, 4 ms and startingTcpAngleAddition.");
    return result;
  }
  const auto increment = readBounds(object.value(QStringLiteral("increment")));
  const auto cumulative = readBounds(object.value(QStringLiteral("cumulative")));
  const auto poscorr = readPosCorr(object.value(QStringLiteral("poscorr")));
  const auto poscorrmon = readPosCorrMon(object.value(QStringLiteral("poscorrmon")));
  if (!increment || !cumulative || !poscorr || !poscorrmon) {
    result.error = QStringLiteral("Fill finite RSI increment/cumulative bounds and POSCORR/POSCORRMON limits.");
    return result;
  }
  RsiCorrectionLimitProfile profile{object.value(QStringLiteral("profileId")).toString().trimmed(),
                                    *increment, *cumulative, *poscorr, *poscorrmon};
  result.error = validateRsiCorrectionLimitProfile(profile);
  if (result.error.isEmpty()) result.profile = std::move(profile);
  return result;
}

} // namespace

QString validateRsiCorrectionLimitProfile(const RsiCorrectionLimitProfile& profile)
{
  if (profile.id.trimmed().isEmpty()) return QStringLiteral("Give the RSI limit profile an identity.");
  if (!validBounds(profile.increment) || !validBounds(profile.cumulative))
    return QStringLiteral("RSI application bounds must be finite, ordered and include zero in every channel.");
  if (!profile.poscorr.minimumTranslation.allFinite() || !profile.poscorr.maximumTranslation.allFinite()
      || !(profile.poscorr.minimumTranslation.array() <= 0.0).all()
      || !(profile.poscorr.maximumTranslation.array() >= 0.0).all())
    return QStringLiteral("POSCORR translation bounds must be finite, ordered and include zero.");
  if (!std::isfinite(profile.poscorr.maximumRotationDegrees) || profile.poscorr.maximumRotationDegrees < 0.0
      || !std::isfinite(profile.poscorrmon.maximumTranslationMillimetres)
      || profile.poscorrmon.maximumTranslationMillimetres < 0.0
      || !std::isfinite(profile.poscorrmon.maximumRotationDegrees) || profile.poscorrmon.maximumRotationDegrees < 0.0)
    return QStringLiteral("POSCORR/POSCORRMON scalar limits must be finite and nonnegative.");
  return {};
}

RsiCorrectionLimitConfiguration readRsiCorrectionLimitConfiguration(const QString& path)
{
  QFile file(path);
  if (!file.exists())
    return {{}, QStringLiteral("Configure RSI limits in rsi-correction-limits.json."), QByteArrayLiteral("missing"), false};
  if (!file.open(QIODevice::ReadOnly))
    return {{}, QStringLiteral("Cannot read RSI correction limits: %1").arg(file.errorString()),
            QByteArrayLiteral("unreadable"), true};
  if (file.size() > kMaximumConfigurationBytes)
    return {{}, QStringLiteral("RSI correction limits exceed 64 KiB."), QByteArrayLiteral("oversized"), true};
  const QByteArray bytes = file.read(kMaximumConfigurationBytes + 1);
  if (file.error() != QFileDevice::NoError || bytes.size() > kMaximumConfigurationBytes)
    return {{}, QStringLiteral("Cannot read the complete RSI correction limits."), QByteArrayLiteral("read-error"), true};
  return decodeConfiguration(bytes);
}

QString describeRsiCorrectionLimitViolation(const RsiCorrectionLimitViolation& violation)
{
  constexpr int precision = std::numeric_limits<double>::max_digits10;
  return QStringLiteral("%1 at sample %2: %3 %4 outside [%5, %6].")
      .arg(violation.quantity).arg(violation.sampleIndex)
      .arg(QString::number(violation.value, 'g', precision)).arg(violation.unit)
      .arg(QString::number(violation.minimum, 'g', precision))
      .arg(QString::number(violation.maximum, 'g', precision));
}
