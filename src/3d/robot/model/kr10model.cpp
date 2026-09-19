#include "kr10model.h"

#include <optional>
#include <cmath>

#include <QDebug>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QJsonValue>

#include "3d/math/utils.h"

namespace RoboCrap3D {

namespace
{

std::optional<QJsonObject> readRoot(const QString& file)
{
  QFile fs(file);

  if (!fs.open(QIODevice::ReadOnly)) {
    qWarning() << "Cannot open KR10 JSON file:" << file;
    return std::nullopt;
  }

  QJsonParseError err{};
  const QJsonDocument doc = QJsonDocument::fromJson(fs.readAll(), &err);

  if (err.error != QJsonParseError::NoError) {
    qWarning() << "Cannot parse KR10 JSON:" << err.errorString();
    return std::nullopt;
  }

  if (!doc.isObject()) {
    qWarning() << "KR10 JSON root is not an object";
    return std::nullopt;
  }

  return doc.object();
}

std::optional<double> readDouble(const QJsonObject& obj, const QString& key)
{
  const QJsonValue val = obj.value(key);

  if (!val.isDouble() || !std::isfinite(val.toDouble())) {
    qWarning() << "Missing or non-numeric JSON field:" << key;
    return std::nullopt;
  }

  return val.toDouble();
}

std::optional<V6d> readV6d(const QJsonArray& arr, const QString& key)
{
  if (arr.size() != static_cast<int>(DofCount)) {
    qWarning() << "Invalid array size for" << key << ":" << arr.size();
    return std::nullopt;
  }

  V6d vals{};

  for (std::size_t idx = 0; idx < DofCount; ++idx) {
    const QJsonValue val = arr.at(static_cast<int>(idx));
    if (!val.isDouble() || !std::isfinite(val.toDouble())) {
      qWarning() << "Invalid numeric value in" << key << "at index" << idx;
      return std::nullopt;
    }

    vals[idx] = val.toDouble();
  }

  return vals;
}

std::optional<V3d> readAxis(const QJsonObject& obj)
{
  const std::optional<V3d> axis = jsonValueToPoint(obj.value(QStringLiteral("axis")));

  if (!axis) {
    qWarning() << "Joint axis is missing or invalid";
    return std::nullopt;
  }

  const std::optional<V3d> unitAxis = normalize(*axis);

  if (!unitAxis || std::abs(unitAxis->X()) > GeomConst::Eps
      || std::abs(unitAxis->Y()) > GeomConst::Eps
      || !nearlyEqual(std::abs(unitAxis->Z()), 1.0)) {
    qWarning() << "KR10 analytical IK requires signed local Z joint axes";
    return std::nullopt;
  }

  return unitAxis;
}

std::optional<JointModel> readJoint(const QJsonObject& obj)
{
  const QJsonValue offVal = obj.value(QStringLiteral("offset"));

  if (!offVal.isObject()) {
    qWarning() << "Joint offset is missing or invalid";
    return std::nullopt;
  }

  const QJsonObject off = offVal.toObject();

  const std::optional<double> dx = readDouble(off, QStringLiteral("dx"));
  const std::optional<double> dy = readDouble(off, QStringLiteral("dy"));
  const std::optional<double> dz = readDouble(off, QStringLiteral("dz"));
  const std::optional<double> da = readDouble(off, QStringLiteral("da"));
  const std::optional<double> db = readDouble(off, QStringLiteral("db"));
  const std::optional<double> dc = readDouble(off, QStringLiteral("dc"));
  const std::optional<V3d> axis = readAxis(obj);
  const std::optional<double> qMin = readDouble(obj, QStringLiteral("min"));
  const std::optional<double> qMax = readDouble(obj, QStringLiteral("max"));

  if (!dx || !dy || !dz || !da || !db || !dc || !axis || !qMin || !qMax) {
    return std::nullopt;
  }

  if (*qMin > *qMax || std::abs(*qMin) > 36000.0 || std::abs(*qMax) > 36000.0) {
    qWarning() << "Invalid KR10 joint limits";
    return std::nullopt;
  }

  const M4d T = makeTranslation(V3d{*dx, *dy, *dz})
          .Multiplied(makeRotation(*da, V3d{0.0, 0.0, 1.0}))
          .Multiplied(makeRotation(*db, V3d{0.0, 1.0, 0.0}))
          .Multiplied(makeRotation(*dc, V3d{1.0, 0.0, 0.0}));

  return JointModel{*axis, T, *qMin, *qMax};
}

std::optional<Kr10Model::IkParams> readIk(const QJsonObject& obj)
{
  const std::optional<double> sx = readDouble(obj, QStringLiteral("sx"));
  const std::optional<double> sz = readDouble(obj, QStringLiteral("sz"));
  const std::optional<double> a =  readDouble(obj, QStringLiteral("a"));
  const std::optional<double> bx = readDouble(obj, QStringLiteral("bx"));
  const std::optional<double> by = readDouble(obj, QStringLiteral("by"));
  const std::optional<double> dF = readDouble(obj, QStringLiteral("dF"));

  if (!sx || !sz || !a || !bx || !by || !dF) {
    return std::nullopt;
  }

  return Kr10Model::IkParams{*sx, *sz, *a, *bx, *by, *dF};
}

std::optional<std::array<JointModel, DofCount>> readJoints(const QJsonArray& arr)
{
  if (arr.size() != static_cast<int>(DofCount)) {
    qWarning() << "Invalid joints array size:" << arr.size();
    return std::nullopt;
  }

  std::array<JointModel, DofCount> joints{};

  for (std::size_t idx = 0; idx < DofCount; ++idx) {
    const QJsonValue val = arr.at(static_cast<int>(idx));

    if (!val.isObject()) {
      qWarning() << "Invalid joint object at index:" << idx;
      return std::nullopt;
    }

    const std::optional<JointModel> joint = readJoint(val.toObject());

    if (!joint) {
      qWarning() << "Cannot parse joint at index:" << idx;
      return std::nullopt;
    }

    joints[idx] = *joint;
  }

  return joints;
}

std::optional<LinkModel> readLink(const QJsonObject& obj)
{
  const QString file = obj.value(QStringLiteral("filename")).toString();

  if (file.isEmpty()) {
    qWarning() << "Link filename is missing";
    return std::nullopt;
  }

  const QJsonArray arr = obj.value(QStringLiteral("rgb")).toArray();

  if (arr.size() != 3) {
      qWarning() << "Invalid RGB array for link:" << file;
      return std::nullopt;
    }

  for (int idx = 0; idx < 3; ++idx) {
      if (!arr.at(idx).isDouble()) {
          qWarning() << "Invalid RGB value for link:" << file;
          return std::nullopt;
        }
    }

  OccPartProps props{};
  props.color = rgb(arr.at(0).toInt(), arr.at(1).toInt(), arr.at(2).toInt());

  return LinkModel{file, props};
}

std::optional<std::array<LinkModel, LinkCount>> readLinks(const QJsonArray& arr)
{
  if (arr.size() != static_cast<int>(LinkCount)) {
    qWarning() << "Invalid links array size:" << arr.size();
    return std::nullopt;
  }

  std::array<LinkModel, LinkCount> links{};

  for (std::size_t idx = 0; idx < LinkCount; ++idx) {
    const QJsonValue val = arr.at(static_cast<int>(idx));

    if (!val.isObject()) {
      qWarning() << "Invalid link object at index:" << idx;
      return std::nullopt;
    }

    const std::optional<LinkModel> link = readLink(val.toObject());

    if (!link) {
      qWarning() << "Cannot parse link at index:" << idx;
      return std::nullopt;
    }

    links[idx] = *link;
  }

  return links;
}

} // namespace

std::optional<Kr10Model> Kr10Model::fromJson(const QString& file)
{
  const std::optional<QJsonObject> root = readRoot(file);

  if (!root) return std::nullopt;

  const QString name = root->value(QStringLiteral("name")).toString();

  if (name.isEmpty()) {
    qWarning() << "KR10 model name is missing";
    return std::nullopt;
  }

  const std::optional<std::array<JointModel, DofCount>> joints = readJoints(root->value(QStringLiteral("joints")).toArray());

  if (!joints) {
    qWarning() << "Cannot parse KR10 joints";
    return std::nullopt;
  }

  const std::optional<std::array<LinkModel, LinkCount>> links = readLinks(root->value(QStringLiteral("links")).toArray());

  if (!links) {
    qWarning() << "Cannot parse KR10 links";
    return std::nullopt;
  }

  const QJsonValue effVal = root->value(QStringLiteral("end_effector"));

  if (!effVal.isObject()) {
    qWarning() << "KR10 end_effector is missing";
    return std::nullopt;
  }

  const std::optional<LinkModel> eff = readLink(effVal.toObject());

  if (!eff) {
    qWarning() << "Cannot parse KR10 end_effector";
    return std::nullopt;
  }

  const QJsonValue ikVal = root->value(QStringLiteral("IkParams"));

  if (!ikVal.isObject()) {
    qWarning() << "KR10 analyticalIK is missing";
    return std::nullopt;
  }

  const std::optional<Kr10Model::IkParams> ik = readIk(ikVal.toObject());

  if (!ik) {
    qWarning() << "Cannot parse KR10 analyticalIK";
    return std::nullopt;
  }

  const std::optional<V6d> qHome = readV6d(root->value(QStringLiteral("qHome")).toArray(), QStringLiteral("qHome"));

  if (!qHome) {
    qWarning() << "Cannot parse KR10 qHome";
    return std::nullopt;
  }

  const std::optional<V6d> pHome = readV6d(root->value(QStringLiteral("pHome")).toArray(), QStringLiteral("pHome"));

  if (!pHome) {
    qWarning() << "Cannot parse KR10 pHome";
    return std::nullopt;
  }

  for (std::size_t i = 0; i < DofCount; ++i) {
    if ((*qHome)[i] < (*joints)[i].qMin || (*qHome)[i] > (*joints)[i].qMax) {
      qWarning() << "KR10 home joint is outside its limits:" << i;
      return std::nullopt;
    }
  }

  return Kr10Model{name, *joints, *links, *eff, *ik, *qHome, *pHome};
}

} // namespace RoboCrap3D
