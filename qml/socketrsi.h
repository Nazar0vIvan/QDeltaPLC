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
#include <QXmlStreamReader>
#include <QRandomGenerator>
#include <QTextStream>

#include "logger.h"

#define RSI_PEER_ADDRESS "192.168.1.4"

struct RandomData {
  QVector<double> values;
  quint64 ipoc;
};

class SocketRSI : public QUdpSocket
{
  Q_OBJECT

public:
  explicit SocketRSI(const QString& name, QObject *parent = nullptr);

  struct RsiResponce {
    std::array<double, 6> aiPos{{0,0,0,0,0,0}};
    std::array<double, 6> maCur{{0,0,0,0,0,0}};
    quint64 ipoc = 0;

    void qdump() const {
      QString s;

      s = "AIPos: ";
      for (int i = 0; i < 6; ++i)
        s += QString("A%1 = %2").arg(i+1).arg(aiPos[i], 0, 'f', 2) + " | ";
      qDebug().noquote() << s;

      s = "MACur: ";
      for (int i = 0; i < 6; ++i)
        s += QString("A%1 = %2").arg(i+1).arg(maCur[i], 0, 'f', 2) + " | ";
      qDebug().noquote() << s;

      qDebug() << "IPOC: " << ipoc;
    }
  };

  Q_INVOKABLE void startStreaming();
  Q_INVOKABLE void stopStreaming();
  Q_INVOKABLE QVariantMap parseConfigFile(const QVariantMap& data);
  Q_INVOKABLE void setSocketConfig(const QVariantMap& config);
  Q_INVOKABLE void xmlTest();

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

  QHostAddress m_la;
  qint16 m_lp = 0;
  QHostAddress m_pa;
  qint16 m_pp = 0;

  QString stateToString(SocketState state);
  // parsing
  RsiResponce parseRsiResponce(const QByteArray& xmlBytes);
  std::array<double, 6> readAxis6(const QXmlStreamAttributes& attrs);

  QByteArray subsXml(const QList<double> &vec, quint64 ipoc, int indent = 2);

};

#endif // SOCKETRSI_H
