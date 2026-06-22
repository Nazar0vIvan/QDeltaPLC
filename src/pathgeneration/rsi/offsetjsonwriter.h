#pragma once

#include <QString>
#include <QVector>

#include "geometry/mathtypes.h"

bool writeOffsetsToJson(const QVector<V6d>& offsets, const QString& filePath, int decimals = 3);