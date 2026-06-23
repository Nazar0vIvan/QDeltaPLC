#pragma once

#include <QAbstractSocket>
#include <QElapsedTimer>
#include <QHostAddress>
#include <QUdpSocket>
#include <QVariantMap>
#include <QVector>

#include "logger.h"
#include "rdtmessage.h"

class QNetworkDatagram;

class SocketFTS : public QUdpSocket
{
  Q_OBJECT

public:
  explicit SocketFTS(const QString& name, QObject* parent = nullptr);

  Q_INVOKABLE void startStreaming();
  Q_INVOKABLE void stopStreaming();
  Q_INVOKABLE void bias();
  Q_INVOKABLE void setSocketConfig(const QVariantMap& config);

  Q_INVOKABLE void setLogRecordingEnabled(bool enabled);
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

  void logMessage(const LoggerMessage& msg);

private slots:
  void onReadyRead();
  void onErrorOccurred(QAbstractSocket::SocketError socketError);
  void onStateChanged(QAbstractSocket::SocketState state);

private:
  static QNetworkDatagram req2dtg(const RDTRequest& request);
  static RDTResponse dtg2resp(const QNetworkDatagram& networkDatagram);

  void appendLogSample(const RDTResponse& sample);
  QVector<RDTResponse> exportLogSamples() const;
  void clearLog();

  void saveLogToFileImpl(const QString& filePath);

  QVector<RDTResponse> m_batch;
  QElapsedTimer m_emitTimer;

  quint32 m_baseSeq = 0;
  bool m_isFirstRead = false;
  int m_emitIntervalMs = 16;

  QHostAddress m_la;
  quint16 m_lp = 0;

  QHostAddress m_pa;
  quint16 m_pp = 0;

  bool m_logEnabled = false;
  int m_logCapacity = 7500;
  QVector<RDTResponse> m_log;

  QString m_logFilePath = QStringLiteral("record.json");
};
