#pragma once

#include "chamfercartesiansampler.h"
#include "rsiposeencoder.h"

#include <QMetaType>
#include <QString>
#include <memory>

// A saved numerical motion and its application provenance, copied on the GUI thread.
struct ChamferJobSource
{
  std::shared_ptr<const ChamferMotion> motion;
  QString jobId;
  quint32 pathId = 0;
  quint32 sourceEdgeId = 0;
  quint64 calibrationRevision = 0;
};

class PreparedChamferJob;
using PreparedChamferJobPtr = std::shared_ptr<const PreparedChamferJob>;
struct PreparedChamferJobResult;

// Nominal execution input only; contains no scene, viewport or device objects.
class PreparedChamferJob final
{
public:
  static PreparedChamferJobResult create(ChamferJobSource source,
                                         RsiCorrectionLimitConfiguration limits = {});
  const ChamferJobSource& source() const { return m_source; }
  const ChamferMotion& motion() const { return *m_source.motion; }
  double cyclePeriod() const;
  double motionStartTime() const { return m_motionStartTime; }
  // Job-relative start, machining start, lead-out start and terminal time.
  const std::array<double, 4>& phaseTimes() const { return m_phaseTimes; }
  double duration() const { return m_phaseTimes.back(); }
  double centralTimeScale() const { return motion().centralTimeScale(); }
  const QVector<ChamferCartesianSample>& samples() const { return m_samples; }
  int sampleCount() const { return static_cast<int>(m_samples.size()); }
  const RsiNominalEncodingResult& encoding() const { return m_encoding; }
  bool encodingReady() const { return m_encoding.data && m_encoding.data->ready(); }
  QString encodingUnavailableReason() const
  { return m_encoding.data ? m_encoding.data->unavailableReason : m_encoding.error; }
  const ChamferPlaybackPose& initialPose() const { return m_boundaryPoses[2]; }
  const ChamferPlaybackPose& machiningStartPose() const { return m_boundaryPoses[3]; }
  const ChamferPlaybackPose& machiningEndPose() const { return m_boundaryPoses[4]; }
  const ChamferPlaybackPose& finalPose() const { return m_boundaryPoses[5]; }
  const ChamferPlaybackPose& homePose() const { return m_boundaryPoses[0]; }
  const ChamferPlaybackPose& stagingInPose() const { return m_boundaryPoses[1]; }
  const ChamferPlaybackPose& stagingOutPose() const { return m_boundaryPoses[6]; }

private:
  explicit PreparedChamferJob(ChamferJobSource source);
  ChamferJobSource m_source;
  double m_motionStartTime = 0.0;
  std::array<double, 4> m_phaseTimes{};
  std::array<ChamferPlaybackPose, 8> m_boundaryPoses{};
  QVector<ChamferCartesianSample> m_samples;
  RsiNominalEncodingResult m_encoding;
};

struct PreparedChamferJobResult
{
  PreparedChamferJobPtr job;
  QString error;
};

Q_DECLARE_METATYPE(PreparedChamferJobPtr)
