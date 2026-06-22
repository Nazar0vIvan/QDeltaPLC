#include "offsetjsonwriter.h"

#include <algorithm>
#include <cmath>

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

namespace {

int normalizedDecimals(int decimals)
{
  return std::clamp(decimals, 0, 12);
}

double roundToDecimals(double value, int decimals)
{
  const double scale = std::pow(10.0, normalizedDecimals(decimals));
  return std::round(value * scale) / scale;
}

QJsonArray vectorToJsonArray(const V6d& v, int decimals)
{
  QJsonArray array;

  for (int i = 0; i < 6; ++i) {
    array.append(roundToDecimals(v(i), decimals));
  }

  return array;
}

} // namespace

bool writeOffsetsToJson(const QVector<V6d>& offsets, const QString& filePath, int decimals)
{
  if (filePath.isEmpty()) return false;

  QJsonArray root;

  for (int i = 0; i < offsets.size(); ++i) {
    const V6d& offset = offsets[i];

    if (!offset.allFinite()) return false;

    QJsonObject object;
    object["index"] = i;
    object["offset"] = vectorToJsonArray(offset, decimals);

    root.append(object);
  }

  QFile file(filePath);

  if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
    return false;
  }

  file.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
  return true;
}

