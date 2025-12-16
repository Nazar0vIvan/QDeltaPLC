#ifndef SOCKETFTS_H
#define SOCKETFTS_H

#include <QObject>
#include <QUdpSocket>
#include <QNetworkDatagram>
#include <QDebug>
#include <QVector>
#include <QByteArray>
#include <QtEndian>
#include <QElapsedTimer>
#include <QVariantList>
#include <QVariant>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDir>

#include "logger.h"

struct RDTRequest
{
  uint16_t header;
  uint16_t command;
  uint32_t sampleCount;
};

struct RDTResponse
{
  Q_GADGET
  Q_PROPERTY(uint32_t rdt_sequence MEMBER rdt_sequence)
  Q_PROPERTY(uint32_t ft_sequence MEMBER ft_sequence)
  Q_PROPERTY(uint32_t status MEMBER status)
  Q_PROPERTY(int32_t Fx MEMBER Fx)
  Q_PROPERTY(int32_t Fy MEMBER Fy)
  Q_PROPERTY(int32_t Fz MEMBER Fz)
  Q_PROPERTY(int32_t Tx MEMBER Tx)
  Q_PROPERTY(int32_t Ty MEMBER Ty)
  Q_PROPERTY(int32_t Tz MEMBER Tz)
  Q_PROPERTY(double timestamp MEMBER timestamp)

public:
  uint32_t rdt_sequence;
  uint32_t ft_sequence;
  uint32_t status;
  int32_t Fx;
  int32_t Fy;
  int32_t Fz;
  int32_t Tx;
  int32_t Ty;
  int32_t Tz;
  double timestamp;
};

Q_DECLARE_METATYPE(RDTResponse)

class SocketFTS : public QUdpSocket
{
    Q_OBJECT

public:
  explicit SocketFTS(const QString& name, QObject* parent = nullptr);

  Q_INVOKABLE void startStreaming();
  Q_INVOKABLE void stopStreaming();
  Q_INVOKABLE void setSocketConfig(const QVariantMap& config);

  // logger
  Q_INVOKABLE void setLogRecordingEnabled(bool enabled);
  Q_INVOKABLE void startLogRecording();
  Q_INVOKABLE void stopLogRecording();

  Q_INVOKABLE bool saveLogToDefaultFile();

signals:
  void dataSampleHFReady(const RDTResponse& sample); // for rsi
  void dataSampleLFReady(const RDTResponse& sample); // for ui
  void dataBatchReady(const QVector<RDTResponse>& samples); // for ui
  void streamReset();

  // logger
  void logRecordingEnabledChanged(bool enabled);
  void logRecordingReady(const QVector<RDTResponse>& samples); // emitted when recording stops

  void logMessage(const LoggerMessage& msg);

private slots:
  void onReadyRead();
  void onErrorOccurred(QAbstractSocket::SocketError socketError);
  void onStateChanged(QAbstractSocket::SocketState state);

private:
  QNetworkDatagram req2dtg(const RDTRequest& request);
  RDTResponse dtg2resp(const QNetworkDatagram& networkDatagram) const;
  QString stateToString(SocketState state);

  QVector<RDTResponse> m_batch;
  QElapsedTimer m_emitTimer;
  quint32       m_baseSeq = 0;
  bool          m_isFirstRead = false;
  int           m_emitIntervalMs = 16; // ~60 Hz GUI updates

  QHostAddress m_la;
  quint16 m_lp = 0;
  QHostAddress m_pa;
  quint16 m_pp = 0;

  // logger
  void appendLogSample(const RDTResponse& sample);
  QVector<RDTResponse> exportLogSamples() const;
  void clearLog();

  bool m_logEnabled = false;
  int  m_logCapacity = 7500;   // number of LF samples to keep
  int  m_logWriteIdx = 0;      // next write position
  int  m_logCount    = 0;      // number of valid samples (<= capacity)
  QVector<RDTResponse> m_log;  // fixed-size storage (resized to capacity)

  bool saveLogToFileImpl(const QString& filePath);
  QString m_logFilePath = "record.json";
};

#endif // SOCKETFTS_H
