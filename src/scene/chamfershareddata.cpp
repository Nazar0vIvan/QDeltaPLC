#include "chamfershareddata.h"

#include "geometry/utils.h"
#include <QDir>
#include <QSaveFile>
#include <QStringList>
#include <cmath>
#include <limits>
#include <utility>

namespace {

bool matchesFrame(const V6d& coordinates, const M4d& target)
{
  return coordinates.allFinite() && target.allFinite()
      && (coordinates.head<3>() - target.block<3, 1>(0, 3)).norm() <= RoboCrap3D::Kr10PosEps
      && (euler2rot(coordinates[3], coordinates[4], coordinates[5])
          - target.block<3, 3>(0, 0)).cwiseAbs().maxCoeff() <= RoboCrap3D::Kr10RotEps;
}

QString number(double value)
{
  return QString::number(value, 'g', std::numeric_limits<double>::max_digits10);
}

QString frameRow(const V6d& coordinates)
{
  QStringList fields;
  for (int axis = 0; axis < 6; ++axis) fields.append(number(coordinates[axis]));
  return fields.join(QLatin1Char(';')) + QStringLiteral("\r\n");
}

bool representableRate(double value)
{
  return std::isfinite(value) && value >= std::numeric_limits<float>::min()
      && value <= std::numeric_limits<float>::max();
}

} // namespace

ChamferSharedDataResult prepareChamferSharedData(ChamferJobSource source,
                                               RsiCorrectionLimitConfiguration limits)
{
  ChamferSharedDataResult result;
  result.prepared = PreparedChamferJob::create(std::move(source), std::move(limits));
  if (!result.prepared.job) return result;
  const auto& job = *result.prepared.job;
  if (!job.encodingReady()) {
    result.error = QStringLiteral("shared_data.txt not exported: nominal encoding is not ready.");
    return result;
  }
  const auto& encoding = *job.encoding().data;
  V6d accumulated = V6d::Zero();
  for (const auto& correction : encoding.corrections) accumulated += correction.increment;
  const V6d start = encoding.initialCoordinates;
  const V6d end = start + accumulated;
  V6d staging = start;
  staging.head<3>() = job.stagingInPose().tcp.block<3, 1>(0, 3);
  V6d stagingOut = end;
  stagingOut.head<3>() = staging.head<3>();
  if (!matchesFrame(staging, job.stagingInPose().tcp)
      || !matchesFrame(start, job.initialPose().tcp)
      || !matchesFrame(end, job.finalPose().tcp)
      || !matchesFrame(stagingOut, job.stagingOutPose().tcp)) {
    result.error = QStringLiteral("shared_data.txt not exported: transfer frames do not match the saved job.");
    return result;
  }
  const auto& timing = job.motion().robot().timing;
  const double velocity = timing.transferFeed / 1000.0;
  const double acceleration = timing.transferAcceleration / 1000.0;
  if (!representableRate(velocity) || !representableRate(acceleration)) {
    result.error = QStringLiteral("shared_data.txt not exported: transfer rates are not positive representable KRL values.");
    return result;
  }
  result.contents = (frameRow(staging) + frameRow(start) + frameRow(end)
      + number(velocity) + QStringLiteral("\r\n")
      + number(acceleration) + QStringLiteral("\r\n")).toLatin1();
  return result;
}

QString writeChamferSharedData(const QByteArray& contents)
{
  if (contents.isEmpty()) return QStringLiteral("shared_data.txt export failed: empty payload.");
  const QString directory = QStringLiteral("C:/RoboCrapJobs");
  if (!QDir().mkpath(directory))
    return QStringLiteral("Cannot create C:/RoboCrapJobs for shared_data.txt.");
  QSaveFile file(QDir(directory).filePath(QStringLiteral("shared_data.txt")));
  file.setDirectWriteFallback(false);
  if (!file.open(QIODevice::WriteOnly))
    return QStringLiteral("Cannot open shared_data.txt: %1").arg(file.errorString());
  if (file.write(contents) != contents.size()) {
    const QString error = file.errorString();
    file.cancelWriting();
    return QStringLiteral("Cannot write shared_data.txt: %1").arg(error);
  }
  if (!file.commit())
    return QStringLiteral("Cannot replace shared_data.txt: %1").arg(file.errorString());
  return {};
}
