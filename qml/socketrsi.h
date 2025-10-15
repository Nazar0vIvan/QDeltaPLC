#ifndef SOCKETRSI_H
#define SOCKETRSI_H

#include <QUdpSocket>
#include <QNetworkDatagram>
#include <QVector>
#include <QByteArray>
#include <QTimer>
#include <QFile>
#include <QFileDialog>
#include <QDebug>
#include <QXmlStreamReader>
#include <QDomDocument>
#include <QNetworkDatagram>
#include <QDebug>
#include <QRegularExpression>
#include <QString>

#include "logger.h"

#define RSI_PEER_ADDRESS "192.168.1.2"

class SocketRSI : public QUdpSocket
{
  Q_OBJECT

public:
  explicit SocketRSI(const QString& name, QObject *parent = nullptr);

  Q_INVOKABLE void startStreaming();
  Q_INVOKABLE void stopStreaming();
  Q_INVOKABLE QVariantMap parseConfigFile(const QVariantMap& data);
  Q_INVOKABLE void setSocketConfig(const QVariantMap& config);
  Q_INVOKABLE void unbind();

signals:
  void logMessage(const LoggerMessage& msg);

private slots:
  void onReadyRead();
  void onErrorOccurred(QAbstractSocket::SocketError socketError);
  void onStateChanged(QAbstractSocket::SocketState state);

private:
  QByteArray moveCommand{ "<Sen Type=\"ImFree\"><AKorr A1=\"0.01\" A2=\"0.0\" A3=\"0.0\" A4=\"0.0\" A5=\"0.0\" A6=\"0.0\" /><IPOC>00000000</IPOC></Sen>" };
  QByteArray defaultCommand{ "<Sen Type=\"ImFree\"><AKorr A1=\"0.0\" A2=\"0.0\" A3=\"0.0\" A4=\"0.0\" A5=\"0.0\" A6=\"0.0\" /><IPOC>00000000</IPOC></Sen>" };
  bool m_onlysend = false;

  QString stateToString(SocketState state);
};

#endif // SOCKETRSI_H
