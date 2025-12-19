#ifndef SOCKETRUNNER_H
#define SOCKETRUNNER_H

#include <QObject>
#include <QThread>
#include <QTcpSocket>
#include <QVector>
#include <QPointF>
#include <QTimer>
#include <QMetaMethod>
#include <QtMinMax>

#include "logger.h"
#include "socketfts.h"
#include "socketrsi.h"
#include "socketdeltaplc.h"

// ----- AbstractSocketRunner -----

class AbstractSocketRunner : public QObject
{
  Q_OBJECT

public:
  explicit AbstractSocketRunner(QAbstractSocket* socket, QObject* parent = nullptr);
  ~AbstractSocketRunner() override;

  Q_PROPERTY(int socketState READ socketState NOTIFY socketStateChanged)
  Q_INVOKABLE void invoke(const QString& method, const QVariantMap& args = {});

  int socketState() const { return m_socketState; }

signals:
  void logMessage(const LoggerMessage& msg);
  void bufferChanged();
  void socketStateChanged();
  void resultReady(const QString& method, const QVariantMap& result);

public slots:
  void start();
  void stop();

  void onSocketStateChanged(QAbstractSocket::SocketState state);
  void onThreadStarted();
  void onThreadFinished();

protected:
  QAbstractSocket* m_socket = nullptr;
  QThread* m_thread = nullptr;
  QStringList m_api = {};

private:
  void attachSocket(QAbstractSocket* sock);
  QStringList invokableMethodNames() const;
  bool allowed(const QString& methodName) const;
  int  indexOfSignature(const QByteArray& sig) const;
  bool returnsVariantMap(int idx) const;

  int m_socketState = QAbstractSocket::UnconnectedState;
  QVariant m_buffer;
};

// ----- TcpSocketRunner -----

class TcpSocketRunner : public AbstractSocketRunner
{
  Q_OBJECT

public:
  explicit TcpSocketRunner(QTcpSocket* socket, QObject* parent = nullptr);
  ~TcpSocketRunner() override = default;
};

// ----- UdpSocketRunner -----

class UdpSocketRunner : public AbstractSocketRunner
{
  Q_OBJECT

public:
  explicit UdpSocketRunner(QUdpSocket* socket, QObject* parent = nullptr);
  ~UdpSocketRunner() override;

  Q_PROPERTY(bool isStreaming READ isStreaming NOTIFY isStreamingChanged)

  bool isStreaming() const { return m_isStreaming; }

signals:
  void isStreamingChanged();

private slots:
  void onPulse();

private:
  bool m_isStreaming = false;
  QTimer m_timer;
};

// ----- PlcRunner -----


class PlcRunner : public TcpSocketRunner {
  Q_OBJECT

public:
  explicit PlcRunner(QTcpSocket* socket, QObject* parent = nullptr);
  ~PlcRunner() override = default;

signals:
  void dataReady(const QVariantMap& data);
};

// ----- FtsRunner -----

class FtsRunner : public UdpSocketRunner
{
  Q_OBJECT

public:
  explicit FtsRunner(QUdpSocket* socket, QObject* parent = nullptr);
  ~FtsRunner() override = default;

  Q_PROPERTY(RDTResponse sample READ sample NOTIFY sampleReady)
  Q_PROPERTY(quint32 sampleSeq READ sampleSeq NOTIFY sampleReady)

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
  void publish(const RDTResponse& s);

  RDTResponse m_lastPublished{};
  bool        m_hasPublished = false;

  RDTResponse m_sample{};
  double      m_tolerance = 0.05;
};

// ----- RsiRunner -----

class RsiRunner : public UdpSocketRunner
{

  Q_OBJECT

public:
  explicit RsiRunner(QUdpSocket* socket, QObject* parent = nullptr);
  ~RsiRunner() override = default;

  Q_PROPERTY(bool motionActive READ motionActive NOTIFY motionActiveChanged)

  bool motionActive() const { return m_motionActive; }

signals:
  void trajectoryReady();
  void motionStarted();
  void motionFinished();
  void motionActiveChanged();

private slots:
  void onMotionStarted();
  void onMotionFinished();
  void onMotionActiveChanged(bool active);

private:
  bool m_motionActive = false;


};

#endif // SOCKETRUNNER_H
