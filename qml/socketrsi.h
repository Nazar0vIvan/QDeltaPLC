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
#include "pathplanner.h"

#define PC_RSI_PORT 58961;

// P1 = { 478.453461, 400.827942, 357.948029, 0.0, 89.9999924, 0.0 }

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
    QVector<double> aiPos{0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
    QVector<double> maCur{0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
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

  Q_INVOKABLE void bind();
  Q_INVOKABLE void stop();
  Q_INVOKABLE QVariantMap parseConfigFile(const QVariantMap& data);
  Q_INVOKABLE void setSocketConfig(const QVariantMap& config);
  Q_INVOKABLE void generateTrajectory();

  Q_INVOKABLE void startStreaming();
  Q_INVOKABLE void stopStreaming();

  Q_INVOKABLE void test();

signals:
  void logMessage(const LoggerMessage& msg);
  void rsiReady();

public slots:
  void setForce(double Fz) { m_Fz = Fz; }

private slots:
  void onReadyRead();
  void onErrorOccurred(QAbstractSocket::SocketError socketError);
  void onStateChanged(QAbstractSocket::SocketState state);

private:
  QVector<Vec6d> m_offsets;
  int m_offsetIdx = 0;
  double m_Fz  = 0.0;
  double m_Fth = 0.0;

  // QByteArray moveCommand{ "<Sen Type=\"ImFree\"><AKorr A1=\"0.01\" A2=\"0.0\" A3=\"0.0\" A4=\"0.0\" A5=\"0.0\" A6=\"0.0\" /><IPOC>00000000</IPOC></Sen>" };
  // QByteArray defaultCommand{ "<Sen Type=\"ImFree\"><AKorr A1=\"0.0\" A2=\"0.0\" A3=\"0.0\" A4=\"0.0\" A5=\"0.0\" A6=\"0.0\" /><IPOC>00000000</IPOC></Sen>" };
  QByteArray defaultCommand =
      "<Sen Type=\"ImFree\">"
        "<RKorr X=\"0.0\" Y=\"0.0\" Z=\"0.0\" A=\"0.0\" B=\"0.0\" C=\"0.0\" />"
        "<F Fz=\"0.0\" Fth=\"0.0\" />"
        "<IPOC>00000000</IPOC>"
      "</Sen>";

  bool m_onlysend = false;

  QHostAddress m_la;
  quint16 m_lp = 0;
  QHostAddress m_pa;
  quint16 m_pp = 0;

  QString stateToString(SocketState state);

  // parsing
  RsiResponce parseRsiResponce(const QByteArray& xmlBytes);
  QVector<double> readAxis6(const QXmlStreamAttributes& attrs);
  QVector<double> readCartesian6(const QXmlStreamAttributes& attrs);

  QByteArray subsXml(const QList<double> &corr,
                     double Fz, double Fth,
                     quint64 ipoc, int indent=2);
  QByteArray subsIPOC(const QByteArray& xml, quint64 ipoc);
  bool m_isFirstRead = false;
  bool m_isMoving = false;

  QVector<Vec6d> m_trajectory;

  QVector<QNetworkDatagram> dgs;
};

#endif // SOCKETRSI_H
