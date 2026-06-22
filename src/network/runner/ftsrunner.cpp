#include "network/runner/ftsrunner.h"

#include "network/fts/socketfts.h"

#include <QUdpSocket>
#include <cstdlib>

FtsRunner::FtsRunner(QUdpSocket* socket, QObject* parent) : UdpSocketRunner(socket, parent)
{
  auto* fts = qobject_cast<SocketFTS*>(socket);
  if (!fts) {
    emit logMessage({"FtsRunner: socket is not SocketFTS", 0, objectName()});
    return;
  }

  connect(fts, &SocketFTS::dataSampleLFReady, this, &FtsRunner::onDataSampleLFReady, Qt::QueuedConnection);
}

double FtsRunner::axisValue(const QString& tag) const
{
  if (!m_hasPublished) return 0.0;

  if (tag == "Fx") return m_sample.Fx;
  if (tag == "Fy") return m_sample.Fy;
  if (tag == "Fz") return m_sample.Fz;
  if (tag == "Tx") return m_sample.Tx;
  if (tag == "Ty") return m_sample.Ty;
  if (tag == "Tz") return m_sample.Tz;

  return 0.0;
}

void FtsRunner::onDataSampleLFReady(const RDTResponse& sample)
{
  if (!m_hasPublished) {
    m_lastPublished = sample;
    m_hasPublished = true;
    publish(sample);
    return;
  }

  RDTResponse out = m_lastPublished;
  copyMeta(out, sample);

  static constexpr double countFactor = 1'000'000.0;
  const auto tolCounts = static_cast<qint64>(m_tolerance * countFactor);

  if (!applyDeadbandAxes(out, sample, tolCounts)) return;

  m_lastPublished = out;
  publish(out);
}

void FtsRunner::copyMeta(RDTResponse& dst, const RDTResponse& src)
{
  dst.rdt_sequence = src.rdt_sequence;
  dst.ft_sequence = src.ft_sequence;
  dst.status = src.status;
  dst.timestamp = src.timestamp;
}

static inline bool acceptAxis(int32_t& dst, int32_t src, qint64 tolCounts)
{
  const qint64 diff = qint64(src) - qint64(dst);

  if (std::llabs(diff) >= tolCounts) {
    dst = src;
    return true;
  }

  return false;
}

bool FtsRunner::applyDeadbandAxes(RDTResponse& dst, const RDTResponse& src, qint64 tolCounts)
{
  bool any = false;

  any |= acceptAxis(dst.Fx, src.Fx, tolCounts);
  any |= acceptAxis(dst.Fy, src.Fy, tolCounts);
  any |= acceptAxis(dst.Fz, src.Fz, tolCounts);
  any |= acceptAxis(dst.Tx, src.Tx, tolCounts);
  any |= acceptAxis(dst.Ty, src.Ty, tolCounts);
  any |= acceptAxis(dst.Tz, src.Tz, tolCounts);

  return any;
}

void FtsRunner::publish(const RDTResponse& sample)
{
  m_sample = sample;
  emit sampleReady();
}
