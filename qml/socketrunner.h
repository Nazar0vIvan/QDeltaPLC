#ifndef SOCKETRUNNER_H
#define SOCKETRUNNER_H

#include <QObject>
#include <QThread>
#include <QTcpSocket>
#include <QVector>
#include <QPointF>
#include <QTimer>
#include <QMetaMethod>

#include "logger.h"

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

class TcpSocketRunner : public AbstractSocketRunner
{
  Q_OBJECT

public:
  explicit TcpSocketRunner(QAbstractSocket* socket, QObject* parent = nullptr);
  ~TcpSocketRunner() override;

signals:
  void plcDataReady(const QVariantMap& data);
};

class UdpSocketRunner : public AbstractSocketRunner
{
  Q_OBJECT

public:
  explicit UdpSocketRunner(QAbstractSocket* socket, QObject* parent = nullptr);
  ~UdpSocketRunner() override;

  Q_PROPERTY(QVariantList lastReading READ lastReading NOTIFY lastReadingChanged)
  Q_PROPERTY(bool isStreaming READ isStreaming NOTIFY isStreamingChanged)

  QVariantList lastReading() const { return m_lastReading; }
  bool isStreaming() const { return m_isStreaming; }

signals:
  void lastReadingChanged();
  void isStreamingChanged();

public slots:
  void onBufferReady(const QVector<QVariantList>& readings);

private slots:
  void onPulse();

private:
  QVariantList m_lastReading = {};
  bool m_isStreaming = false;
  QTimer m_timer;
};

#endif // SOCKETRUNNER_H
