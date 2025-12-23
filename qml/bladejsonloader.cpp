#include "bladejsonloader.h"

#include <QFile>
#include <QUrl>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>
#include <QJsonParseError>

namespace {

V3d jsonValueToVec3(const QJsonValue& v)
{
  const QJsonArray a = v.toArray(); // expected [x,y,z]
  return V3d(a[0].toDouble(), a[1].toDouble(), a[2].toDouble());
}

QVector<V3d> jsonArrayToProfile(const QJsonArray& arr)
{
  QVector<V3d> out;
  out.reserve(arr.size());
  for (const QJsonValue& v : arr) {
    out.push_back(jsonValueToVec3(v));
  }
  return out;
}

BladeProfile jsonObjectToBladeProfile(const QJsonObject& obj)
{
  BladeProfile bp;
  bp.cx = jsonArrayToProfile(obj.value("cx").toArray());
  bp.cv = jsonArrayToProfile(obj.value("cv").toArray());
  bp.re = jsonArrayToProfile(obj.value("re").toArray());
  bp.le = jsonArrayToProfile(obj.value("le").toArray());
  return bp;
}

} // namespace

BladeJsonLoader::LoadResult BladeJsonLoader::load(const QVariantMap& data)
{
  LoadResult res;

  if (data.value("path").isNull() || !data.value("path").canConvert<QUrl>()) {
    res.ok = false;
    res.error = "Empty/Invalid path";
    res.path.clear();
    return res;
  }

  res.path = data.value("path").toUrl().toLocalFile();
  return loadFromFile(res.path);
}

BladeJsonLoader::LoadResult BladeJsonLoader::loadFromFile(const QString& path)
{
  LoadResult res;
  res.path = path;

  QFile f(path);
  if (!f.open(QIODevice::ReadOnly)) {
    res.ok = false;
    res.error = f.errorString();
    res.path.clear();
    return res;
  }

  const QByteArray bytes = f.readAll();
  if (bytes.isEmpty()) {
    res.ok = false;
    res.error = "JSON file is empty";
    res.path.clear();
    return res;
  }

  QJsonParseError err;
  const QJsonDocument doc = QJsonDocument::fromJson(bytes, &err);
  if (err.error != QJsonParseError::NoError) {
    res.ok = false;
    res.error = QString("JSON parse error at offset %1: %2")
                    .arg(err.offset)
                    .arg(err.errorString());
    res.path.clear();
    return res;
  }

  if (!doc.isArray()) {
    res.ok = false;
    res.error = "Invalid JSON: top-level value must be an array";
    res.path.clear();
    return res;
  }

  const QJsonArray top = doc.array();

  Airfoil af;
  af.reserve(top.size());

  for (const QJsonValue& v : top) {
    if (!v.isObject()) {
      res.ok = false;
      res.error = "Invalid JSON: array element is not an object";
      res.path.clear();
      return res;
    }
    af.push_back(jsonObjectToBladeProfile(v.toObject()));
  }

  res.airfoil = std::move(af);
  res.ok = true;
  return res;
}
