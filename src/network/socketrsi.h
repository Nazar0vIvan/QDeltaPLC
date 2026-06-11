#ifndef SOCKETRSI_H
#define SOCKETRSI_H

#include <QUdpSocket>
#include <QHostAddress>
#include <QNetworkDatagram>
#include <QByteArray>
#include <QVector>
#include <QQueue>
#include <QTimer>
#include <QVariantMap>
#include <QDomDocument>
#include <QXmlStreamReader>

#include <QRandomGenerator>

#include <array>

#include "logger.h"
#include "socketfts.h"
#include "pathplanner.h"
#include "bladejsonloader.h"

// QByteArray moveCommand{ "<Sen Type=\"ImFree\"><AKorr A1=\"0.01\" A2=\"0.0\" A3=\"0.0\" A4=\"0.0\" A5=\"0.0\" A6=\"0.0\" /><IPOC>00000000</IPOC></Sen>" };
// QByteArray defaultCommand{ "<Sen Type=\"ImFree\"><AKorr A1=\"0.0\" A2=\"0.0\" A3=\"0.0\" A4=\"0.0\" A5=\"0.0\" A6=\"0.0\" /><IPOC>00000000</IPOC></Sen>" };

/*
"<Sen Type=\"ImFree\">"
  "<RKorr X=\"0.0\" Y=\"0.0\" Z=\"0.0\" A=\"0.0\" B=\"0.0\" C=\"0.0\" />"
  "<F Fz=\"0.0\" Fth=\"0.0\" />"
  "<Flags FzTrip=\"0\" />"
  "<IPOC>00000000</IPOC>"
"</Sen>";
*/

struct RandomData {
  QVector<double> values;
  quint64 ipoc;
};

struct RsiTxFrame {
  quint64 ipoc = 0;
  bool shouldStop = false;
  std::array<double, 6> corr{};
};

class SocketRSI : public QUdpSocket
{
  Q_OBJECT

public:
  explicit SocketRSI(const QString& name, QObject *parent = nullptr);

  struct RsiResponse {
    QVector<double> pose{0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
    quint64 ipoc = 0;
  };

  Q_INVOKABLE void stop();
  Q_INVOKABLE QVariantMap parseConfigFile(const QVariantMap& data);
  Q_INVOKABLE void setSocketConfig(const QVariantMap& config);
  Q_INVOKABLE void generateTrajectory();
  Q_INVOKABLE QVariantMap loadBladeJson(const QVariantMap& json);

  Q_INVOKABLE void startStreaming();
  Q_INVOKABLE void stopStreaming();

  Q_INVOKABLE void test();

signals:
  void motionStarted();
  void motionFinished();
  void motionActiveChanged(bool active);
  void trajectoryReady();

  void logMessage(const LoggerMessage& msg);

public slots:
  void setForce(const RDTResponse& sample); // FOR NOW

private slots:
  void onReadyRead();
  void onErrorOccurred(QAbstractSocket::SocketError socketError);
  void onStateChanged(QAbstractSocket::SocketState state);
  void onCooldownFinished();

private:
  enum class MotionState { Idle, Moving, Cooldown };
  MotionState m_state = MotionState::Idle;

  void setMotionState(MotionState s);
  void finishMotion(bool enterCooldown);

  QHostAddress m_la;
  quint16 m_lp = 0;
  QHostAddress m_pa;
  quint16 m_pp = 0;

  QString stateToString(SocketState state);
  void handleFirstRead(const QNetworkDatagram& dg);

  static constexpr double COUNT_FACTOR = 1000000.0;
  static constexpr int COOLDOWN_MS = 10000;

  // rsi
  QByteArray defaultCommand =
    "<Sen Type=\"ImFree\">"
    "<RKorr X=\"0.0\" Y=\"0.0\" Z=\"0.0\" A=\"0.0\" B=\"0.0\" C=\"0.0\" />"
    "<Flags ShouldStop=\"0\" />"
    "<IPOC>00000000</IPOC>"
    "</Sen>";

  QVector<V6d> m_offsets;
  int m_offsetIdx = 0;

  double m_Fz  = 0.0;
  double m_Fth = 50.0; // !!!

  bool m_onlysend = false;  // kept (you parse it); wire up when needed
  bool m_isFirstRead = true;

  // --- bounded debug logs ---
  static constexpr int MAX_LOG_FRAMES = 200;
  QQueue<QNetworkDatagram> m_rxLog;
  QQueue<QByteArray>       m_txXmlLog;

  void pushRxLog(const QNetworkDatagram& dg);
  void pushTxLog(const QByteArray& xml);

  // --- timers ---
  QTimer m_cooldownTimer;

  // --- logic helpers ---
  std::array<double, 6> tickMotion(bool& shouldStopOut);
  RsiTxFrame makeTxFrame(quint64 ipoc);

  // --- serialization/parsing ---
  QByteArray subsXml(const RsiTxFrame& tx);
  RsiResponse parseRsiResponse(const QByteArray& xmlBytes);
  QVector<double> readCartesian6(const QXmlStreamAttributes& attrs);

  Airfoil m_af;

};

#endif // SOCKETRSI_H
