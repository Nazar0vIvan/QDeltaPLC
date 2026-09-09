#pragma once

#include "network/abstractdevice.h"
#include "network/fts/rdtmessage.h"
#include <QElapsedTimer>
#include <QHostAddress>
#include <QVector>

class QTimer;
class QUdpSocket;

class FtsDevice : public AbstractDevice
{
  Q_OBJECT

public:
  explicit FtsDevice(const QString& name, QObject* parent = nullptr);
  ~FtsDevice() override = default;

  Q_INVOKABLE void startStreaming();
  Q_INVOKABLE void stopStreaming();
  Q_INVOKABLE void bias();
  Q_INVOKABLE void connect(const QVariantMap& config);
  Q_INVOKABLE void disconnect();
  Q_INVOKABLE void startLogRecording();
  Q_INVOKABLE void stopLogRecording();
  Q_INVOKABLE void saveLogToDefaultFile();

signals:
  void dataSampleHFReady(const RDTResponse& sample);
  void dataBatchReady(const QVector<RDTResponse>& samples);
  void streamReset();

protected:
  void startDevice() override;
  void stopDevice() override;

private slots:
  void onReadyRead();
  void onBatchTick();

private:
  void appendLogSample(const RDTResponse& sample);
  void saveLogToFileImpl(const QString& filePath);
  void setReceiving(bool enabled);
  void publishState(const RDTResponse& sample);
  void sendRequest(quint16 cmd, quint32 count = 0);

  QUdpSocket* m_sock = nullptr;

  QTimer* m_batchTimer = nullptr;
  QElapsedTimer m_clock;
  qint64 m_lastRxMs = 0;

  QVector<RDTResponse> m_batch;

  RDTResponse m_lastPub{};
  bool m_hasPub = false;

  quint32 m_baseSeq = 0;
  bool m_needBase = true;
  bool m_receiving = false;

  QHostAddress m_la;
  quint16 m_lp = 0;
  QHostAddress m_pa;
  quint16 m_pp = 0;

  bool m_logEnabled = false;
  QVector<RDTResponse> m_log;
};

// CHANGE END
