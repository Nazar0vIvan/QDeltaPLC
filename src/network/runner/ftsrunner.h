#pragma once

#include "network/runner/udpsocketrunner.h"
#include "network/fts/rdtmessage.h" // if not extracted yet, use "network/socketfts.h"

#include <QString>

class QUdpSocket;

class FtsRunner : public UdpSocketRunner
{
  Q_OBJECT
  Q_PROPERTY(RDTResponse sample READ sample NOTIFY sampleReady)
  Q_PROPERTY(quint32 sampleSeq READ sampleSeq NOTIFY sampleReady)

public:
  explicit FtsRunner(QUdpSocket* socket, QObject* parent = nullptr);
  ~FtsRunner() override = default;

  Q_INVOKABLE double axisValue(const QString& tag) const;

  RDTResponse sample() const { return m_sample; }
  quint32 sampleSeq() const { return m_sample.rdt_sequence; }

signals:
  void sampleReady();

public slots:
  void onDataSampleLFReady(const RDTResponse& sample);

private:
  static void copyMeta(RDTResponse& dst, const RDTResponse& src);
  static bool applyDeadbandAxes(RDTResponse& dst, const RDTResponse& src, qint64 tolCounts);
  void publish(const RDTResponse& sample);

  RDTResponse m_lastPublished{};
  bool m_hasPublished = false;

  RDTResponse m_sample{};
  double m_tolerance = 0.05;
};