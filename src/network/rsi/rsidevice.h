#pragma once

#include "geometry/mathtypes.h"
#include "network/abstractdevice.h"
#include "network/fts/rdtmessage.h"

#include <QByteArray>
#include <QHostAddress>
#include <QVector>
#include <QXmlStreamReader>

#include <array>

class QNetworkDatagram;
class QTimer;
class QUdpSocket;

struct RsiTxFrame {
  quint64 ipoc = 0;
  bool shouldStop = false;
  std::array<double, 6> corr{};
};

class RsiDevice : public AbstractDevice
{
  Q_OBJECT

public:
  explicit RsiDevice(const QString& name, QObject* parent = nullptr);
  ~RsiDevice() override = default;

  Q_INVOKABLE void connect(const QVariantMap& config);
  Q_INVOKABLE void disconnect();
  Q_INVOKABLE void generateTrajectory();
  Q_INVOKABLE void startStreaming();
  Q_INVOKABLE void stopStreaming();

public slots:
  void setForce(const RDTResponse& sample);

protected:
  void startDevice() override;
  void stopDevice() override;

private slots:
  void onReadyRead();
  void onCooldownFinished();

private:
  struct RsiResponse {
    QVector<double> pose{0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
    quint64 ipoc = 0;
  };

  enum class MotionState {
    Idle,
    Moving,
    Cooldown
  };

  void setMotionState(MotionState state);
  void finishMotion(bool cooldown);
  void handleFirstRead(const QNetworkDatagram& dg);

  std::array<double, 6> tickMotion(bool& stop);
  RsiTxFrame makeTxFrame(quint64 ipoc);

  QByteArray subsXml(const RsiTxFrame& tx);
  RsiResponse parseRsiResponse(const QByteArray& xml);
  QVector<double> readCartesian6(const QXmlStreamAttributes& attrs);

  QUdpSocket* m_sock = nullptr;
  QTimer* m_cooldown = nullptr;

  QHostAddress m_la;
  quint16 m_lp = 0;
  QHostAddress m_pa;
  quint16 m_pp = 0;

  MotionState m_state = MotionState::Idle;

  QVector<V6d> m_offsets;
  int m_offIdx = 0;

  double m_fz = 0.0;

  bool m_firstRead = true;
};
