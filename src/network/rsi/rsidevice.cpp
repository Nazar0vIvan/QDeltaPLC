#include "network/rsi/rsidevice.h"

#include "geometry/cylinder.h"
#include "geometry/plane.h"
#include "geometry/pose.h"
#include "geometry/utils.h"
#include "pathgeneration/rsi/offsetjsonwriter.h"
#include "pathgeneration/rsi/rsipath.h"

#include <QNetworkDatagram>
#include <QLocale>
#include <QTimer>
#include <QUdpSocket>
#include <QXmlStreamWriter>

#include <iostream>
#include <utility>

namespace {

constexpr double kCount = 1'000'000.0;
constexpr int kCooldownMs = 10'000;

} // namespace

RsiDevice::RsiDevice(const QString& name, QObject* parent)
  : AbstractDevice(name, parent) {}

void RsiDevice::startDevice()
{
  Q_ASSERT(!m_sock);
  Q_ASSERT(!m_cooldown);

  m_sock = new QUdpSocket(this);
  m_cooldown = new QTimer(this);

  m_cooldown->setSingleShot(true);

  attachSocket(m_sock);

  QObject::connect(
      m_sock,
      &QUdpSocket::readyRead,
      this,
      &RsiDevice::onReadyRead);

  QObject::connect(
      m_cooldown,
      &QTimer::timeout,
      this,
      &RsiDevice::onCooldownFinished);

  emit stateReady({
    {"motionActive", false},
    {"trajectoryReady", false}
  });
}

void RsiDevice::stopDevice()
{
  if (!m_sock) return;

  disconnect();

  delete std::exchange(m_cooldown, nullptr);
  delete std::exchange(m_sock, nullptr);
}

void RsiDevice::connect(const QVariantMap& config)
{
  if (!m_sock) {
    emit logMessage({
      "RSI device is not started",
      0,
      objectName()
    });
    return;
  }

  if (!config.isEmpty()) {
    const QHostAddress la(config.value("localAddress").toString());
    const QHostAddress pa(config.value("peerAddress").toString());

    bool lpOk = false;
    const uint lp = config.value("localPort").toUInt(&lpOk);

    if (la.isNull()
        || pa.isNull()
        || !lpOk
        || lp == 0
        || lp > 65535) {
      emit logMessage({
        "Invalid socket configuration",
        0,
        objectName()
      });
      return;
    }

    m_la = la;
    m_lp = static_cast<quint16>(lp);
    m_pa = pa;

    // RSI peer port comes from the first incoming datagram.
    m_pp = 0;
  }

  if (m_la.isNull() || m_lp == 0 || m_pa.isNull()) {
    emit logMessage({
      "Socket configuration is incomplete",
      0,
      objectName()
    });
    return;
  }

  if (m_sock->state() != QAbstractSocket::UnconnectedState)
    disconnect();

  m_firstRead = true;

  if (!m_sock->bind(m_la, m_lp)) {
    emit logMessage({
      QString("Bind failed: %1").arg(m_sock->errorString()),
      0,
      objectName()
    });
    return;
  }

  emit logMessage({
    QString("Socket connected: %1:%2")
        .arg(m_la.toString())
        .arg(m_lp),
    1,
    objectName()
  });
}

void RsiDevice::disconnect()
{
  if (!m_sock) return;

  stopStreaming();

  m_sock->close();

  m_firstRead = true;
  m_pp = 0;

  emit logMessage({
    "Socket disconnected",
    1,
    objectName()
  });
}

void RsiDevice::generateTrajectory()
{
  // A previously generated trajectory is no longer current.
  emit stateReady({
    {"trajectoryReady", false}
  });

  m_offsets.clear();
  m_offIdx = 0;

  const V3d ur(0.999349, -0.036055, 0.000879);
  const V3d cr(926.290032, -59.007181, 623.760314);
  const double rr = 20.043646;

  Cylinder roller = *Cylinder::fromAxis(ur, cr, rr, Axis::X);
  roller.setSurfacePose(-13.0, -45.0);

  const Pose surf = roller.surfacePose();
  const double dz = 0.0;
  const auto shifted = *surf.offsetPose(Axis::Z, dz);

  std::cout << shifted.frame() << "\n\n";

  Plane plane = *Plane::fromJsonFile("://files/blank-plane-bottom.json");

  const V3d p214{8.377225, 26.999799, 156.207670};
  const V3d p211{8.464722, -27.000830, 156.213933};

  const V3d prj214 = *prjPointToPlane(p214, plane.coeffs);
  const V3d prj211 = *prjPointToPlane(p211, plane.coeffs);

  const auto ux = -plane.normal();
  const auto uy = *normalize(prj214 - prj211);
  const auto uz = *normalize(-plane.normal().cross(uy));

  Pose pt1 = *Pose::fromAxes(ux, uy, uz, prj214);
  Pose pt2 = *Pose::fromAxes(ux, uy, uz, prj211);

  Pose pt0 = *pt1.offsetPose(Axis::Y, 40.0);
  Pose ptn = *pt2.offsetPose(Axis::Y, -40.0);

  M4d ais;
  ais << 0.0, 0.0, 1.0, 0.0,
         0.0, 1.0, 0.0, 0.0,
        -1.0, 0.0, 0.0, 0.0,
         0.0, 0.0, 0.0, 1.0;

  const V6d p1 = RsiPath::fromSurfPose(pt0, ais)->frame();
  const V6d p2 = RsiPath::fromSurfPose(ptn, ais)->frame();

  m_offsets = RsiPath::lin(p1, p2, {10, 4});

  if (m_offsets.empty()) {
    emit logMessage({
      "Generated RSI trajectory is empty",
      0,
      objectName()
    });
    return;
  }

  writeOffsetsToJson(m_offsets, "offsets.json");

  emit stateReady({
    {"trajectoryReady", true}
  });
}

void RsiDevice::startStreaming()
{
  m_offIdx = 0;
  m_firstRead = true;

  if (m_cooldown) m_cooldown->stop();

  setMotionState(MotionState::Moving);
}

void RsiDevice::stopStreaming()
{
  if (m_cooldown) m_cooldown->stop();

  finishMotion(false);
}

void RsiDevice::setForce(const RDTResponse& sample)
{
  m_fz = sample.Fz / kCount;
}

void RsiDevice::onReadyRead()
{
  while (m_sock && m_sock->hasPendingDatagrams()) {
    const QNetworkDatagram dg = m_sock->receiveDatagram();

    if (m_firstRead) handleFirstRead(dg);

    const RsiResponse resp = parseRsiResponse(dg.data());
    const RsiTxFrame tx = makeTxFrame(resp.ipoc);
    const QByteArray reply = subsXml(tx);

    m_sock->writeDatagram(reply, m_pa, m_pp);
  }
}

void RsiDevice::onCooldownFinished()
{
  setMotionState(MotionState::Idle);
}

void RsiDevice::setMotionState(MotionState state)
{
  if (m_state == state) return;

  const bool wasActive = m_state == MotionState::Moving;
  const bool isActive = state == MotionState::Moving;

  m_state = state;

  if (wasActive == isActive) return;

  emit stateReady({
    {"motionActive", isActive}
  });
}

void RsiDevice::finishMotion(bool cooldown)
{
  if (m_state == MotionState::Idle) {
    m_offIdx = 0;
    return;
  }

  m_offIdx = 0;

  if (cooldown) {
    setMotionState(MotionState::Cooldown);

    if (m_cooldown)
      m_cooldown->start(kCooldownMs);

    return;
  }

  setMotionState(MotionState::Idle);
}

void RsiDevice::handleFirstRead(const QNetworkDatagram& dg)
{
  m_pa = dg.senderAddress();
  m_pp = dg.senderPort();
  m_firstRead = false;
}

std::array<double, 6> RsiDevice::tickMotion(bool& stop)
{
  stop = false;

  std::array<double, 6> corr{};
  corr.fill(0.0);

  if (m_state != MotionState::Moving) return corr;

  if (m_offsets.isEmpty()) {
    stop = true;
    finishMotion(false);
    return corr;
  }

  if (m_offIdx < m_offsets.size()) {
    const V6d& offset = m_offsets[m_offIdx++];

    for (int i = 0; i < 6; ++i)
      corr[static_cast<size_t>(i)] = offset(i);

    if (m_offIdx >= m_offsets.size()) {
      stop = true;
      finishMotion(false);
    }

    return corr;
  }

  stop = true;
  finishMotion(false);

  return corr;
}

RsiTxFrame RsiDevice::makeTxFrame(quint64 ipoc)
{
  RsiTxFrame tx;
  tx.ipoc = ipoc;

  tx.corr = tickMotion(tx.shouldStop);

  return tx;
}

QByteArray RsiDevice::subsXml(const RsiTxFrame& tx)
{
  QByteArray out;
  out.reserve(256);

  QXmlStreamWriter xml(&out);
  xml.setAutoFormatting(false);

  xml.writeStartElement("Sen");
  xml.writeAttribute("Type", "ImFree");

  xml.writeEmptyElement("RKorr");

  static const char* keys[6] = {
    "X", "Y", "Z", "A", "B", "C"
  };

  const QLocale locale = QLocale::c();

  for (int i = 0; i < 6; ++i) {
    xml.writeAttribute(
        QLatin1String(keys[i]),
        locale.toString(tx.corr[static_cast<size_t>(i)], 'g', 10));
  }

  xml.writeTextElement("IPOC", QString::number(tx.ipoc));

  xml.writeEndElement();

  return out;
}

RsiDevice::RsiResponse RsiDevice::parseRsiResponse(const QByteArray& data)
{
  RsiResponse resp;
  QXmlStreamReader xml(data);

  if (!xml.readNextStartElement()) return resp;
  if (xml.name() != QLatin1String("Rob")) return resp;

  while (xml.readNextStartElement()) {
    const auto name = xml.name();

    if (name == QLatin1String("RIst")) {
      resp.pose = readCartesian6(xml.attributes());
      xml.skipCurrentElement();
    } else if (name == QLatin1String("IPOC")) {
      resp.ipoc = xml.readElementText().toULongLong();
    } else {
      xml.skipCurrentElement();
    }
  }

  return resp;
}

QVector<double> RsiDevice::readCartesian6(const QXmlStreamAttributes& attrs)
{
  QVector<double> vals = {
    0.0, 0.0, 0.0,
    0.0, 0.0, 0.0
  };

  static const char* keys[6] = {
    "X", "Y", "Z", "A", "B", "C"
  };

  for (int i = 0; i < 6; ++i) {
    vals[i] =
        attrs.value(QLatin1String(keys[i])).toString().toDouble();
  }

  return vals;
}