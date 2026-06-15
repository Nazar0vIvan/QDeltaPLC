#include "blade.h"

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

Pose getCxCvStartFrenet(const QVector<V3d>& cx, double L, const Pose& frenet)
{
  const V3d& pt0 = cx[0];
  const V3d& pt1 = cx[1];
  const V3d& pt = pt0 + L * (pt0 - pt1).normalized();
  return Pose::fromAxes(frenet.t, frenet.b, frenet.n, pt);
}

Pose getCxCvEndFrenet(const QVector<V3d>& cx, double L, const Pose& frenet)
{
  const int n = cx.size();
  const V3d& ptn_1 = cx[n - 2];
  const V3d& ptn   = cx[n - 1];
  const V3d& pt = ptn + L * (ptn - ptn_1).normalized();
  return Pose::fromAxes(frenet.t, frenet.b, frenet.n, pt);
}

Pose getCxCvFrenet(V3d pt, const V3d& poly, const V3d& v0)
{
  V3d t = tanByPoly(pt, poly);
  V3d tanv = (v0 - pt).normalized();
  V3d n = t.cross(tanv).normalized();
  V3d b = n.cross(t).normalized();
  return Pose::fromAxes(t, b, n, pt);
}

QVector<Pose> getCxCvFrenets(const QVector<V3d>& cx, const QVector<V3d>& cx_next, double L)
{
  int n = cx.size();

  QVector<Pose> frenets;
  frenets.reserve(n + 2);

  V3d coef0 = poly(cx[0], cx[1], cx[2]);
  Pose frenet0 = getCxCvFrenet(cx[0], coef0, cx_next[0]);

  Pose startFrenet = getCxCvStartFrenet(cx, L, frenet0);

  frenets << startFrenet << frenet0;

  for (int j=1; j < n-2; j++) {
      V3d coef = poly(cx[j-1], cx[j], cx[j+1]);
      Pose frenet = getCxCvFrenet(cx[j], coef, cx_next[j]);
      frenets.push_back(frenet);
    }

  V3d coefn = poly(cx[n-3], cx[n-2], cx[n-1]);
  Pose nfrenet = getCxCvFrenet(cx[n-1], coefn, cx_next[n-1]);

  Pose endFrenet = getCxCvEndFrenet(cx, L, nfrenet);

  frenets << nfrenet << endFrenet;

  return frenets;
}