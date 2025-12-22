#include "socketrsi.h"
#include "pathplanner.h"

RandomData generateRandomData()
{
  RandomData data;
  auto generator = QRandomGenerator::global();

  // Use std::generate for cleaner loop
  std::generate_n(std::back_inserter(data.values), 6, [&](){ return 0.001 + generator->generateDouble() * 0.009; });
  data.ipoc = generator->generate64();

  return data;
}

SocketRSI::SocketRSI(const QString& name, QObject *parent) : QUdpSocket{parent}
{
  setObjectName(name);

  connect(this, &SocketRSI::readyRead,     this, &SocketRSI::onReadyRead);
  connect(this, &SocketRSI::errorOccurred, this, &SocketRSI::onErrorOccurred);
  connect(this, &SocketRSI::stateChanged,  this, &SocketRSI::onStateChanged);
  connect(this, &SocketRSI::logMessage, Logger::instance(), &Logger::push);

  m_cooldownTimer.setSingleShot(true);
  connect(&m_cooldownTimer, &QTimer::timeout, this, &SocketRSI::onCooldownFinished);
}

// Q_INVOKABLE

void SocketRSI::stop()
{
  stopStreaming();
  close();
  emit logMessage({ "Socket stopped and closed", 1, objectName()});
}

QVariantMap SocketRSI::parseConfigFile(const QVariantMap& data)
{
  const QString path = data.value("path").toUrl().toLocalFile();

  QFile f(path);
  if (!f.open(QIODevice::ReadOnly)) {
    emit logMessage({f.errorString(), 0, objectName()});
    return {};
  }

  QDomDocument dom;
  QDomDocument::ParseResult res = dom.setContent(&f, QDomDocument::ParseOption::Default);
  if (!res) {
    emit logMessage({QString("%1: %2:%3").arg(res.errorMessage).arg(res.errorLine).arg(res.errorColumn), 0, objectName()});
    return {};
  }

  const QDomElement cfg = dom.documentElement().firstChildElement("CONFIG");
  if (cfg.isNull()) {
    emit logMessage({"CONFIG section not found", 0, objectName()});
    return {};
  }

  auto txt = [&](const char* t){ return cfg.firstChildElement(QLatin1String(t)).text().trimmed(); };
  const QString ipStr   = txt("IP_NUMBER");
  const QString portStr = txt("PORT");
  const QString onlyStr = txt("ONLYSEND");

  QHostAddress ip; bool ipOk = ip.setAddress(ipStr);
  bool portOk = false; int port = portStr.toInt(&portOk);
  if (!ipOk || !portOk || port < 1 || port > 65535) {
    emit logMessage({QString("Invalid IP or port: %1:%2").arg(ipStr).arg(port), 0, objectName()});
    return {};
  }

  return { {"path", path}, {"port", portStr}, {"onlysend", onlyStr} };
}

void SocketRSI::setSocketConfig(const QVariantMap &config)
{
  m_la = QHostAddress(config.value("localAddress").toString());
  m_lp = config.value("localPort").toUInt();
  m_pa = QHostAddress(config.value("peerAddress").toString());

  if (!QUdpSocket::bind(m_la, m_lp)) {
    emit logMessage({QString("Bind failed: %1").arg(errorString()), 0, objectName()});
    return;
  }
  emit logMessage({QString("Socket is bound: %1:%2").arg(m_la.toString()).arg(m_lp), 1, objectName()});
}

void SocketRSI::generateTrajectory()
{
  /*
  const V6d P1 = { 478.453461, 400.827942, 357.948029, 0.0, 89.9999924, 0.0 };
  const V6d P2 = { 622.889465, 400.827942, 357.948029, 0.0, 89.9999924, 0.0 };

  m_offsets = rsi::lin(P1, P2, {10, 4});
  if (m_offsets.empty()) {
    emit logMessage({ "Generated RSI trajectory is empty", 0, objectName()});
    return;
  }
  */

  /*
  // WORKPIECE
  V3d Pc = { -0.113702, -0.012406, 111.290488 };

  V3d Pc11 = { -0.151981, -0.002515, 120.0},
                  Pc12 = { -0.153901, -0.003125, 120.0 },
                  Pc21 = { -0.422887,  0.061220, 180.0 },
                  Pc22 = { -0.423638,  0.065223, 180.0};

  V3d Pc1 = 0.5 * (Pc11 + Pc12),
                  Pc2 = 0.5 * (Pc21 + Pc22);

  double Rs[] = {
    12.991316,
    12.990244,
    12.998138,
    12.999339,
    13.008986,
    13.009134,
    13.019839,
    13.019753
  };

  const double Rc = Eigen::Map<const Eigen::Matrix<double,8,1>>(Rs).mean();
  const double Lc = 74.0;

  Cylinder cyl = Cylinder::fromTwoPoints(Pc1, Pc2, Pc, Rc, Lc, 'z');

  M4d AiT;
  AiT <<  -1.0, 0.0, 0.0, 0.0,
           0.0, 0.0, 1.0, 0.0,
           0.0, 1.0, 0.0, 0.0,
           0.0, 0.0, 0.0, 1.0; // ROLLER SURFACE

  QVector<Pose> ref_poses = pathFromSurfPoses(cyl.surfaceRing(100, 0.0), AiT);

  m_offsets = rsi::polyline(posesToFrames(ref_poses), {10, 3});
  m_offsets = m_offsets.mid(0, qsizetype(m_offsets.size() / 9));
  */

  // ROLLER
  const V3d ur(-0.0237168939, 0.9997013354, -0.0058948179);
  const V3d Cr(854.512911, -16.511844, 623.196742); // A point on the axis (near the data “middle”)
  const double Rr = 19.991300;
  const double Lr = 20.0;

  Cylinder rl = Cylinder::fromAxis(ur, Cr, Rr, Lr, 'y');
  Pose rl_s = rl.surfacePose('y', 0.0, 'y', -45.0, 'z', rl.R + 10.0, false);

  // BLADE
  const int i = 0;
  const QVector<V3d>& cx      = m_af[i].cx;
  const QVector<V3d>& cx_next = m_af[i+1].cx;

  QVector<Pose> cxFrenets = getCxCvFrenets(cx, cx_next , 20.0);

  M4d AiT;
  AiT <<  1.0,  0.0,  0.0, 0.0,
          0.0, -1.0,  0.0, 0.0,
          0.0,  0.0, -1.0, 0.0,
          0.0,  0.0,  0.0, 1.0;

  QVector<Pose> path = pathFromSurfPoses(cxFrenets, AiT);

  m_offsets = rsi::polyline(posesToFrames(path), MotionParams{10, 3});

  emit trajectoryReady();

  writeOffsetsToJson(m_offsets, "offsets.json");
}

QVariantMap SocketRSI::loadBladeJson(const QVariantMap &data)
{
  if(data.value("path").isNull() || !data.value("path").canConvert<QUrl>()) {
    emit logMessage({"Empty/Invalid path", 0, objectName()});
    return { {"path", ""}, {"parseResult", false} };
  }

  const QString path = data.value("path").toUrl().toLocalFile();

  QFile f(path);
  if (!f.open(QIODevice::ReadOnly)) {
    emit logMessage({f.errorString(), 0, objectName()});
    return { {"path", ""}, {"parseResult", false} };
  }

  const QByteArray bytes = f.readAll();

  if (bytes.isEmpty()) {
    emit logMessage({"JSON file is empty", 0, objectName()});
    return { {"path", ""}, {"parseResult", false} };
  }

  QJsonParseError err;
  const QJsonDocument doc = QJsonDocument::fromJson(bytes, &err);
  if (err.error != QJsonParseError::NoError) {
    const QString msg = QString("JSON parse error at offset %1: %2")
    .arg(err.offset)
        .arg(err.errorString());
    emit logMessage({msg, 0, objectName()});
    return { {"path", ""}, {"parseResult", false} };
  }

  if (!doc.isArray()) {
    emit logMessage({"Invalid JSON: top-level value must be an array", 0, objectName()});
    return { {"path", ""}, {"parseResult", false} };
  }

  const QJsonArray top = doc.array();

  m_af.reserve(top.size());

  for (const QJsonValue &v : top) {
    if (!v.isObject()) {
      emit logMessage({"Invalid JSON: array element is not an object", 0, objectName()});
      return { {"path", ""}, {"parseResult", false} };
    }
    m_af.push_back(jsonObjectToBladeProfile(v.toObject()));
  }

  emit logMessage({"JSON file is loaded successfully", 1, objectName()});
  return { {"path", path}, {"parseResult", true} };
}

void SocketRSI::startStreaming()
{
  m_offsetIdx = 0;
  m_isFirstRead = true;

  m_cooldownTimer.stop();
  setMotionState(MotionState::Moving);
}

void SocketRSI::stopStreaming()
{
  // stop immediately (no cooldown)
  m_cooldownTimer.stop();
  finishMotion(false);
}

// PUBLIC SLOTS

void SocketRSI::setForce(const RDTResponse& sample)
{
  m_Fz = sample.Fz / COUNT_FACTOR;
}

void SocketRSI::onReadyRead()
{
  while (hasPendingDatagrams()) {
    QNetworkDatagram dg = receiveDatagram();
    pushRxLog(dg);

    if (m_isFirstRead) {
      handleFirstRead(dg);
    }

    const RsiResponse resp = parseRsiResponse(dg.data());

    const RsiTxFrame tx = makeTxFrame(resp.ipoc);

    const QByteArray reply = subsXml(tx);
    pushTxLog(reply);

    writeDatagram(reply, m_pa, m_pp);
  }
}

void SocketRSI::onErrorOccurred(QAbstractSocket::SocketError socketError) {
  emit logMessage({this->errorString(), 0, objectName()});
}

void SocketRSI::onStateChanged(QAbstractSocket::SocketState state) {
  emit logMessage({stateToString(state), 2, objectName()});
}

void SocketRSI::onCooldownFinished()
{
  setMotionState(MotionState::Idle);
}

// PRIVATE

void SocketRSI::setMotionState(MotionState s)
{
  if (m_state == s) return;

  const MotionState prev = m_state;
  m_state = s;

  // transitions with signals
  if (prev != MotionState::Moving && s == MotionState::Moving) {
    emit motionStarted();
    emit motionActiveChanged(true);
  }

  if (prev == MotionState::Moving && s != MotionState::Moving) {
    emit motionFinished();
    emit motionActiveChanged(false);
  }
}

void SocketRSI::finishMotion(bool enterCooldown)
{
  if (m_state == MotionState::Idle) {
    m_offsetIdx = 0;
    return;
  }

  m_offsetIdx = 0;

  if (enterCooldown) {
    setMotionState(MotionState::Cooldown);
    m_cooldownTimer.start(COOLDOWN_MS);
  } else {
    setMotionState(MotionState::Idle);
  }
}

QString SocketRSI::stateToString(SocketState state)
{
  switch (state) {
    case QAbstractSocket::UnconnectedState: return "UnconnectedState";
    case QAbstractSocket::HostLookupState:  return "HostLookupState";
    case QAbstractSocket::ConnectingState:  return "ConnectingState";
    case QAbstractSocket::ConnectedState:   return "ConnectedState";
    case QAbstractSocket::BoundState:       return "BoundState";
    case QAbstractSocket::ClosingState:     return "ClosingState";
    case QAbstractSocket::ListeningState:   return "ListeningState";
    default: return "UnknownState";
  }
}

void SocketRSI::handleFirstRead(const QNetworkDatagram &dg)
{
  m_pa = dg.senderAddress();
  m_pp = dg.senderPort();
  m_isFirstRead = false;
}

void SocketRSI::pushRxLog(const QNetworkDatagram& dg)
{
  m_rxLog.enqueue(dg);
  while (m_rxLog.size() > MAX_LOG_FRAMES) {
    m_rxLog.dequeue();
  }
}

void SocketRSI::pushTxLog(const QByteArray& xml)
{
  m_txXmlLog.enqueue(xml);
  while (m_txXmlLog.size() > MAX_LOG_FRAMES) {
    m_txXmlLog.dequeue();
  }
}

std::array<double, 6> SocketRSI::tickMotion(bool& shouldStopOut)
{
  shouldStopOut = false;

  std::array<double, 6> corr{};
  corr.fill(0.0);

  if (m_state != MotionState::Moving)
    return corr;

  if (m_offsets.isEmpty()) {
    shouldStopOut = true;
    finishMotion(false);
    return corr;
  }

  if (m_offsetIdx < m_offsets.size()) {
    const V6d& dP = m_offsets[m_offsetIdx++];

    for (int i = 0; i < 6; ++i)
      corr[static_cast<size_t>(i)] = dP(i);

    const bool exhausted = (m_offsetIdx >= m_offsets.size());
    if (exhausted) {
      // last offset is sent in this frame -> stop in this same frame
      shouldStopOut = true;
      finishMotion(false);
    }

    return corr;
  }

  // safety fallback (already past end)
  shouldStopOut = true;
  finishMotion(false);
  return corr;
}

RsiTxFrame SocketRSI::makeTxFrame(quint64 ipoc)
{
  RsiTxFrame tx;
  tx.ipoc = ipoc;

  bool shouldStopOut = false;
  tx.corr = tickMotion(shouldStopOut);
  tx.shouldStop = shouldStopOut;

  return tx;
}

QByteArray SocketRSI::subsXml(const RsiTxFrame& tx)
{
  QByteArray out;
  out.reserve(256);

  QXmlStreamWriter w(&out);
  w.setAutoFormatting(false);

  w.writeStartElement("Sen");
  w.writeAttribute("Type", "ImFree");

  // RKorr
  w.writeEmptyElement("RKorr");
  static const char* k[6] = {"X","Y","Z","A","B","C"};
  const QLocale c = QLocale::c();
  for (int i = 0; i < 6; ++i) {
    w.writeAttribute(QLatin1String(k[i]), c.toString(tx.corr[static_cast<size_t>(i)], 'g', 10));
  }

  // Flags
  w.writeEmptyElement("Flags");
  w.writeAttribute("ShouldStop", tx.shouldStop ? "1" : "0");

  // IPOC
  w.writeTextElement("IPOC", QString::number(tx.ipoc));

  w.writeEndElement(); // Sen
  return out;
}

// PARSING

SocketRSI::RsiResponse SocketRSI::parseRsiResponse(const QByteArray& xmlBytes)
{
  RsiResponse r{};
  QXmlStreamReader xml(xmlBytes);

  if (!xml.readNextStartElement()) return r;
  if (xml.name() != QLatin1String("Rob")) return r;

  while (xml.readNextStartElement()) {
    const auto name = xml.name();

    if (name == QLatin1String("RIst")) {
      r.pose = readCartesian6(xml.attributes());
      xml.skipCurrentElement();
    } else if (name == QLatin1String("IPOC")) {
      r.ipoc = xml.readElementText().toULongLong();
    } else {
      xml.skipCurrentElement();
    }
  }

  return r;
}

QVector<double> SocketRSI::readCartesian6(const QXmlStreamAttributes &attrs)
{
  QVector<double> out = { 0.0, 0.0, 0.0, 0.0, 0.0, 0.0 };
  static const char* keys[6] = {"X","Y","Z","A","B","C"};
  for (size_t i = 0; i < 6; ++i) {
    out[i] = attrs.value(QLatin1String(keys[i])).toString().toDouble();
  }
  return out;
}

void SocketRSI::test()
{

}

// json

V3d SocketRSI::jsonValueToVec3(const QJsonValue &v)
{
  const QJsonArray a = v.toArray(); // main-case: [x,y,z]
  return V3d(a[0].toDouble(), a[1].toDouble(), a[2].toDouble());
}

QVector<V3d> SocketRSI::jsonArrayToProfile(const QJsonArray &arr)
{
  QVector<V3d> out;
  out.reserve(arr.size());
  for (const QJsonValue& v : arr) {
    out.push_back(jsonValueToVec3(v));
  }
  return out;
}

BladeProfile SocketRSI::jsonObjectToBladeProfile(const QJsonObject &obj)
{
  BladeProfile bp;
  bp.cx = jsonArrayToProfile(obj.value("cx").toArray());
  bp.cv = jsonArrayToProfile(obj.value("cv").toArray());
  bp.re = jsonArrayToProfile(obj.value("re").toArray());
  bp.le = jsonArrayToProfile(obj.value("le").toArray());
  return bp;
}

