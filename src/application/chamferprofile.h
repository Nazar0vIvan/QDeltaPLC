#pragma once
#include <QString>

// Offline application diagnostic; never creates devices or exports controller files.
int profileChamferPreparation(const QString& inputPath, const QString& reportPath);
