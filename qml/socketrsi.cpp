#include "socketrsi.h"

RandomData generateRandomData()
{
  RandomData data;
  auto generator = QRandomGenerator::global();

  // Use std::generate for cleaner loop
  std::generate_n(std::back_inserter(data.values), 6, [&](){ return 0.001 + generator->generateDouble() * 0.009; });
  data.ipoc = generator->generate64();

  return data;
}

SocketRSI::SocketRSI(const QString& name, QObject *parent) : QUdpSocket{parent}, m_isFirstRead(true), m_isMoving(false)
{
  setObjectName(name);

  connect(this, &SocketRSI::readyRead,     this, &SocketRSI::onReadyRead);
  connect(this, &SocketRSI::errorOccurred, this, &SocketRSI::onErrorOccurred);
  connect(this, &SocketRSI::stateChanged,  this, &SocketRSI::onStateChanged);
  connect(this, &SocketRSI::logMessage, Logger::instance(), &Logger::push);
}

// Q_INVOKABLE

void SocketRSI::stop()
{

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
  const QString ipStr = txt("IP_NUMBER");
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

  // emit logMessage({QString("Socket is configured:<br/>"
  //                  "&nbsp;&nbsp;Local: &nbsp;%1:%2<br/>"
  //                  "&nbsp;&nbsp;Peer: &nbsp;&nbsp;%3:SP").
  //                  arg(m_la.toString()).arg(m_lp).arg(m_pa.toString()),
  //                  1, objectName()});
}

void SocketRSI::generateTrajectory()
{
  const Vec6d P1 = { 478.453461, 400.827942, 357.948029, 0.0, 89.9999924, 0.0 };
  const Vec6d P2 = { 622.889465, 400.827942, 357.948029, 0.0, 89.9999924, 0.0 };

  m_offsets = rsi::lin(P1, P2, {10, 4});
  if (m_offsets.empty()) {
    emit logMessage({ "Generated RSI trajectory is empty", 0, objectName()});
    return;
  }

  emit trajectoryReady();

  // writeOffsetsToJson(m_offsets, "offsets");
}

void SocketRSI::startStreaming()
{
  m_offsetIdx = 0;
  m_isMoving  = true;
  m_motionFinishedEmitted = false;

  if (!m_motionActive) {
    m_motionActive = true;
    emit motionStarted();
    emit motionActiveChanged(true);
  }
  m_isDelayActive = false; // Reset delay status
}

void SocketRSI::stopStreaming()
{
  if (!m_motionActive) return;

  m_isMoving  = false;
  m_offsetIdx = 0;

  if (!m_motionFinishedEmitted) {
    m_motionFinishedEmitted = true;
    emit motionFinished();
  }

  m_motionActive = false;
  emit motionActiveChanged(false);
}

// PUBLIC SLOTS

void SocketRSI::onReadyRead()
{
  while (hasPendingDatagrams()) {
    QNetworkDatagram dg = receiveDatagram();
    dgs.append(dg);

    if (m_isFirstRead) {
      handleFirstRead(dg);
    }

    const RsiResponce resp = parseRsiResponce(dg.data());

    QList<double> corr = getMotionCorrections();

    QByteArray reply = subsXml(corr, m_Fz, m_Fth, resp.ipoc, 0);
    m_sentXmlLog.push_back(reply);
    writeDatagram(reply, m_pa, m_pp);
  }
}

void SocketRSI::onErrorOccurred(QAbstractSocket::SocketError socketError) {
  emit logMessage({this->errorString(), 0, objectName()});
}

void SocketRSI::onStateChanged(QAbstractSocket::SocketState state) {
  emit logMessage({stateToString(state), 2, objectName()});
}

// PRIVATE

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
    default: return "UnconnectedState";
  }
}

void SocketRSI::handleFirstRead(const QNetworkDatagram &dg)
{
  m_pa = dg.senderAddress();
  m_pp = dg.senderPort();
  m_isFirstRead = false;
}

QList<double> SocketRSI::getMotionCorrections()
{
  QList<double> corr(6, 0.0);

  if (m_isMoving) {
    if (m_offsetIdx < m_offsets.size()) {
      const Vec6d& dP = m_offsets[m_offsetIdx++];
      for (int i = 0; i < 6; ++i)
        corr[i] = dP(i);
      if (qFabs(m_Fz) >= qFabs(m_Fth) && !m_isDelayActive) {
        stopMotion(); // Stop motion when Fz reaches Fth
        m_isDelayActive = true; // Start the delay
        QTimer::singleShot(10000, this, &SocketRSI::onDelayFinished); // 10 seconds delay
      }
    } else {
      stopMotion();
    }
  }
  return corr;
}

void SocketRSI::stopMotion()
{
  m_isMoving = false;
  if (m_motionActive && !m_motionFinishedEmitted) {
    m_motionFinishedEmitted = true;
    emit motionFinished();
  }
  m_motionActive = false;
  emit motionActiveChanged(false);
}

QByteArray SocketRSI::subsXml(const QList<double> &corr,
                              double Fz, double Fth,
                              quint64 ipoc, int indent)
{
  QDomDocument doc;
  doc.setContent(defaultCommand);

  QDomElement root = doc.documentElement();
  QDomElement rk   = root.firstChildElement("RKorr");
  QDomElement fEl  = root.firstChildElement("F");
  QLocale c = QLocale::c();

  const char* keys[6] = { "X", "Y", "Z", "A", "B", "C" };

  const int n = qMin(corr.size(), 6);
  for (int i = 0; i < n; ++i) {
    rk.setAttribute(QLatin1String(keys[i]), c.toString(corr.at(i), 'g', 10));
  }

  fEl.setAttribute("Fz",  c.toString(Fz,  'g', 10));
  fEl.setAttribute("Fth", c.toString(Fth, 'g', 10));

  QDomElement ipocEl = root.firstChildElement("IPOC");
  ipocEl.firstChild().setNodeValue(QString::number(ipoc));

  return doc.toByteArray(indent);
}

QByteArray SocketRSI::subsIPOC(const QByteArray& xml, quint64 ipoc)
{
  QDomDocument doc;
  doc.setContent(xml);

  QDomElement root = doc.documentElement();
  QDomElement ipocEl = root.firstChildElement("IPOC");
  ipocEl.firstChild().setNodeValue(QString::number(ipoc));

  return doc.toByteArray();
}

// PARSING

SocketRSI::RsiResponce SocketRSI::parseRsiResponce(const QByteArray& xmlBytes)
{
  RsiResponce r{};
  QXmlStreamReader xml(xmlBytes);

  // Go to the root element
  if (!xml.readNextStartElement())
    return r;

  if (xml.name() != QLatin1String("Rob")) {
    // unexpected root
    return r;
  }

  // Now we are on <Rob>. Parse its children.
  while (xml.readNextStartElement()) {
    const auto name = xml.name();

    if (name == QLatin1String("RIst")) {
      r.aiPos = readAxis6(xml.attributes());
      xml.skipCurrentElement();   // for <AIPos .../>
    } /* else if (name == QLatin1String("MACur")) {
      r.maCur = readAxis6(xml.attributes());
      xml.skipCurrentElement();
    } */ else if (name == QLatin1String("IPOC")) {
      r.ipoc = xml.readElementText().toULongLong();
    } else {
      xml.skipCurrentElement(); // skip unknown children of <Rob>
    }
  }

  return r;
}

QVector<double> SocketRSI::readAxis6(const QXmlStreamAttributes &attrs)
{
  QVector<double> out = { 0.0, 0.0, 0.0, 0.0, 0.0, 0.0 };
  static const char* keys[6] = {"A1","A2","A3","A4","A5","A6"};
  for (size_t i = 0; i < 6; ++i) {
    out[i] = attrs.value(QLatin1String(keys[i])).toString().toDouble();
  }
  return out;
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

void SocketRSI::setForce(const RDTResponse& sample)
{
  m_Fz = sample.Fz / COUNT_FACTOR;
}

void SocketRSI::onDelayFinished()
{
  // After 10 seconds, the motion can be resumed or the correction stopped
  if (!m_motionFinishedEmitted) {
    m_motionFinishedEmitted = true;
    emit motionFinished();
  }
  m_isMoving = false; // Disable further corrections
  m_motionActive = false;
  emit motionActiveChanged(false);
  m_isDelayActive = false; // Reset the delay status
}
