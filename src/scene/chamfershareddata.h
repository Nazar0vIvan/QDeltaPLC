#pragma once

#include "pathgeneration/chamfer/chamferjob.h"
#include <QByteArray>

struct ChamferSharedDataResult
{
  PreparedChamferJobResult prepared;
  QByteArray contents;
  QString error;
};

// Copied-input worker: numerical preparation and file payload, without disk writes.
ChamferSharedDataResult prepareChamferSharedData(ChamferJobSource source,
                                               RsiCorrectionLimitConfiguration limits);
// Called only after the coordinator accepts the worker's current job.
QString writeChamferSharedData(const QByteArray& contents);
