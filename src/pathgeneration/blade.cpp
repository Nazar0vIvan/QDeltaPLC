#include "blade.h"
#include "geometry/utils.h"

#include <cmath>
#include <utility>

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QJsonValue>
#include <QUrl>

namespace {

std::optional<V3d> jsonValueToVec3(const QJsonValue& value, QString* error)
{
  if (!value.isArray()) {
    if (error) *error = "Expected point as [x, y, z]";
    return std::nullopt;
  }

  const QJsonArray array = value.toArray();

  if (array.size() != 3) {
    if (error) *error = "Point array must contain exactly 3 numbers";
    return std::nullopt;
  }

  if (!array[0].isDouble() || !array[1].isDouble() || !array[2].isDouble()) {
    if (error) *error = "Point coordinates must be numbers";
    return std::nullopt;
  }

  return V3d{ array[0].toDouble(), array[1].toDouble(), array[2].toDouble() };
}

std::optional<QVector<V3d>> jsonArrayToProfile(const QJsonObject& object, const QString& fieldName, QString* error)
{
  const QJsonValue value = object.value(fieldName);

  if (!value.isArray()) {
    if (error) *error = QString("Invalid JSON: field '%1' must be an array").arg(fieldName);
    return std::nullopt;
  }

  const QJsonArray array = value.toArray();

  QVector<V3d> points;
  points.reserve(array.size());

  for (int i = 0; i < array.size(); ++i) {
    QString pointError;
    const auto point = jsonValueToVec3(array[i], &pointError);
    if (!point) {
      if (error) { *error = QString("Invalid JSON: field '%1', point %2: %3").arg(fieldName).arg(i).arg(pointError); }
      return std::nullopt;
    }
    points.push_back(*point);
  }
  return points;
}

std::optional<BladeProfile> jsonObjectToBladeProfile(const QJsonObject& object, QString* error)
{
  BladeProfile profile;

  const auto cx = jsonArrayToProfile(object, "cx", error);
  if (!cx) return std::nullopt;

  const auto cv = jsonArrayToProfile(object, "cv", error);
  if (!cv) return std::nullopt;

  const auto re = jsonArrayToProfile(object, "re", error);
  if (!re) return std::nullopt;

  const auto le = jsonArrayToProfile(object, "le", error);
  if (!le) return std::nullopt;

  profile.cx = *cx;
  profile.cv = *cv;
  profile.re = *re;
  profile.le = *le;

  return profile;
}

bool isValidCxCvInput(const QVector<V3d>& cx, const QVector<V3d>& cxNext, double length)
{
  return cx.size() >= 3 &&
         cxNext.size() == cx.size() &&
         std::isfinite(length);
}

} // namespace

BladeJsonLoader::LoadResult BladeJsonLoader::load(const QVariantMap& data)
{
  LoadResult result;

  const QVariant pathValue = data.value("path");

  if (pathValue.isNull() || !pathValue.canConvert<QUrl>()) {
    result.error = "Empty/Invalid path";
    return result;
  }

  const QUrl url = pathValue.toUrl();
  const QString path = url.toLocalFile();

  if (path.isEmpty()) {
    result.error = "Empty/Invalid local file path";
    return result;
  }

  return loadFromFile(path);
}

BladeJsonLoader::LoadResult BladeJsonLoader::loadFromFile(const QString& path)
{
  LoadResult result;
  result.path = path;

  QFile file(path);

  if (!file.open(QIODevice::ReadOnly)) {
    result.error = file.errorString();
    return result;
  }

  const QByteArray bytes = file.readAll();

  if (bytes.isEmpty()) {
    result.error = "JSON file is empty";
    return result;
  }

  QJsonParseError parseError;
  const QJsonDocument document = QJsonDocument::fromJson(bytes, &parseError);

  if (parseError.error != QJsonParseError::NoError) {
    result.error = QString("JSON parse error at offset %1: %2").arg(parseError.offset).arg(parseError.errorString());
    return result;
  }

  if (!document.isArray()) {
    result.error = "Invalid JSON: top-level value must be an array";
    return result;
  }

  const QJsonArray top = document.array();

  Airfoil airfoil;
  airfoil.reserve(top.size());

  for (int i = 0; i < top.size(); ++i) {
    if (!top[i].isObject()) {
      result.error = QString("Invalid JSON: array element %1 is not an object").arg(i);
      return result;
    }

    QString profileError;
    const auto profile = jsonObjectToBladeProfile(top[i].toObject(), &profileError);

    if (!profile) {
      result.error = QString("Invalid JSON: profile %1: %2")
      .arg(i)
          .arg(profileError);
      return result;
    }

    airfoil.push_back(*profile);
  }

  result.airfoil = std::move(airfoil);
  result.ok = true;

  return result;
}

std::optional<Pose> getCxCvStartFrenet(const QVector<V3d>& cx, double length, const Pose& frenet)
{
  if (cx.size() < 2 || !std::isfinite(length)) {
    return std::nullopt;
  }

  const auto direction = normalize(cx[0] - cx[1]);

  if (!direction) return std::nullopt;

  const V3d point = cx[0] + length * *direction;

  return Pose::fromAxes(frenet.t(), frenet.b(), frenet.n(), point);
}

std::optional<Pose> getCxCvEndFrenet(const QVector<V3d>& cx, double length, const Pose& frenet)
{
  if (cx.size() < 2 || !std::isfinite(length)) {
    return std::nullopt;
  }

  const int n = cx.size();

  const auto direction = normalize(cx[n - 1] - cx[n - 2]);

  if (!direction) return std::nullopt;

  const V3d point = cx[n - 1] + length * *direction;

  return Pose::fromAxes(frenet.t(), frenet.b(), frenet.n(), point);
}

std::optional<Pose> getCxCvFrenet(const V3d& point, const V3d& coeffs, const V3d& v0)
{
  const auto tangent = normalize(deriv2d(point, coeffs));
  if (!tangent)  return std::nullopt;

  const auto radial = normalize(v0 - point);
  if (!radial) return std::nullopt;

  const auto normal = normalize(tangent->cross(*radial));
  if (!normal) return std::nullopt;

  const auto binormal = normalize(normal->cross(*tangent));
  if (!binormal) return std::nullopt;

  return Pose::fromAxes(*tangent, *binormal, *normal, point);
}

std::optional<QVector<Pose>> getCxCvFrenets(const QVector<V3d>& cx,  const QVector<V3d>& cxNext, double length)
{
  if (!isValidCxCvInput(cx, cxNext, length)) {
    return std::nullopt;
  }

  const int n = cx.size();

  QVector<Pose> frenets;
  frenets.reserve(n + 2);

  const auto firstCoeffs = polyfit2d(cx[0], cx[1], cx[2]);
  if (!firstCoeffs) return std::nullopt;

  const auto firstFrenet = getCxCvFrenet(cx[0], *firstCoeffs, cxNext[0]);
  if (!firstFrenet) return std::nullopt;

  const auto startFrenet = getCxCvStartFrenet(cx, length, *firstFrenet);
  if (!startFrenet)  return std::nullopt;

  frenets.push_back(*startFrenet);
  frenets.push_back(*firstFrenet);

  for (int i = 1; i < n - 1; ++i) {
    const auto coeffs = polyfit2d(cx[i - 1], cx[i], cx[i + 1]);
    if (!coeffs) return std::nullopt;

    const auto frenet = getCxCvFrenet(cx[i], *coeffs, cxNext[i]);
    if (!frenet) return std::nullopt;

    frenets.push_back(*frenet);
  }

  const auto lastCoeffs = polyfit2d(cx[n - 3], cx[n - 2], cx[n - 1]);
  if (!lastCoeffs) return std::nullopt;

  const auto lastFrenet = getCxCvFrenet(cx[n - 1], *lastCoeffs, cxNext[n - 1]);
  if (!lastFrenet) return std::nullopt;

  const auto endFrenet = getCxCvEndFrenet(cx, length, *lastFrenet);
  if (!endFrenet) return std::nullopt;

  frenets.push_back(*lastFrenet);
  frenets.push_back(*endFrenet);

  return frenets;
}
