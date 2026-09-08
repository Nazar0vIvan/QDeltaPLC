#pragma once

#include "network/abstractdevice.h"
#include "network/fts/rdtmessage.h"

#include <QElapsedTimer>
#include <QHostAddress>
#include <QVector>

class QNetworkDatagram;
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
  void dataSampleLFReady(const RDTResponse& sample);
  void dataBatchReady(const QVector<RDTResponse>& samples);
  void streamReset();

  void logRecordingEnabledChanged(bool enabled);
  void logRecordingReady(const QVector<RDTResponse>& samples);

protected:
  void startDevice() override;
  void stopDevice() override;

private slots:
  void onReadyRead();
  void onPulseTimeout();

private:
  static QNetworkDatagram req2dtg(const RDTRequest& request);
  static RDTResponse dtg2resp(const QNetworkDatagram& datagram);

  void appendLogSample(const RDTResponse& sample);
  void saveLogToFileImpl(const QString& filePath);
  void setLogRecordingEnabled(bool enabled);
  void setStreaming(bool enabled);
  void publishState(const RDTResponse& sample);
  void sendRequest(quint16 cmd, quint32 count = 0);

  QUdpSocket* m_sock = nullptr;
  QTimer* m_pulse = nullptr;

  QVector<RDTResponse> m_batch;
  QElapsedTimer m_emitTimer;

  RDTResponse m_lastPub{};
  bool m_hasPub = false;
  double m_tol = 0.05;

  quint32 m_baseSeq = 0;
  bool m_firstRead = false;
  int m_emitMs = 16;
  bool m_streaming = false;

  QHostAddress m_la;
  quint16 m_lp = 0;
  QHostAddress m_pa;
  quint16 m_pp = 0;

  bool m_logEnabled = false;
  int m_logCap = 7500;
  QVector<RDTResponse> m_log;
  QString m_logFile = QStringLiteral("record.json");
};
