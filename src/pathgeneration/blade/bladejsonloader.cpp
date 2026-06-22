#include "bladejsonloader.h"

#include <utility>

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QJsonValue>
#include <QUrl>
#include <QVariant>

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

  return V3d{array[0].toDouble(), array[1].toDouble(), array[2].toDouble()};
}

std::optional<QVector<V3d>> jsonArrayToProfile(const QJsonObject& object, const QString& fieldName, QString* error)
{
  const QJsonValue value = object.value(fieldName);

  if (!value.isArray()) {
      if (error) {
          *error = QString("Invalid JSON: field '%1' must be an array").arg(fieldName);
        }
      return std::nullopt;
    }

  const QJsonArray array = value.toArray();

  QVector<V3d> points;
  points.reserve(array.size());

  for (int i = 0; i < array.size(); ++i) {
      QString pointError;
      const auto point = jsonValueToVec3(array[i], &pointError);

      if (!point) {
          if (error) {
              *error = QString("Invalid JSON: field '%1', point %2: %3")
              .arg(fieldName)
                  .arg(i)
                  .arg(pointError);
            }
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
    result.error = QString("JSON parse error at offset %1: %2")
      .arg(parseError.offset)
      .arg(parseError.errorString());
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

