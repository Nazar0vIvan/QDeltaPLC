#include "socketrsi.h"

RandomData generateRandomData() {
  RandomData data;
  QRandomGenerator* generator = QRandomGenerator::global();
  for (int i=0; i<6; i++)
    data.values.append(0.001 + generator->generateDouble() * 0.009);
  data.ipoc = generator->generate64();
  return data;
}

SocketRSI::SocketRSI(const QString& name, QObject *parent) : QUdpSocket{parent}
{
  setObjectName(name);

  connect(this, &SocketRSI::logMessage,  Logger::instance(), &Logger::push);
  connect(this, &SocketRSI::readyRead, this, &SocketRSI::onReadyRead);
  connect(this, &SocketRSI::errorOccurred, this, &SocketRSI::onErrorOccurred);
  connect(this, &SocketRSI::stateChanged,  this, &SocketRSI::onStateChanged);

  const quint16 port = 59152;
  if (!bind(QHostAddress("192.168.2.100"), port, QUdpSocket::ShareAddress | QUdpSocket::ReuseAddressHint)) {
    emit logMessage({QString("Bind failed: %1").arg(errorString()), 0, objectName()});
  }
  m_isFirstRead = true;
  m_isMoving = false;
}

// Q_INVOKABLE

void SocketRSI::startStreaming()
{
  m_RsiOK = true;
}

void SocketRSI::stopStreaming()
{
  m_RsiOK = false;
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
  m_pp = config.value("peerPort").toUInt();

  emit logMessage({QString("Socket configured:<br/>"
                   "&nbsp;&nbsp;Local: &nbsp;%1:%2<br/>"
                   "&nbsp;&nbsp;Peer: &nbsp;&nbsp;%3:%4<br/>").
                   arg(m_la.toString()).arg(m_lp).arg(m_pa.toString()).arg(m_pp),
                   1, objectName()});
}

void SocketRSI::test()
{
  for(auto& dg : dgs) {
    qDebug() << dg.data() << "\n\n";
  }
  qDebug() << m_pa;
  qDebug() << m_pp;
}

// PUBLIC SLOTS

void SocketRSI::onReadyRead()
{
  while(hasPendingDatagrams()) {
    QNetworkDatagram dg = receiveDatagram();

    dgs.append(dg);

    qDebug() << dg.data();

    if (m_isFirstRead) {
      m_pa = dg.senderAddress();
      m_pp = dg.senderPort();
      m_isFirstRead = false;
    }

    const RsiResponce resp = parseRsiResponce(dg.data());

    if (!m_isMoving) {
      writeDatagram(subsIPOC(defaultCommand, resp.ipoc), m_pa, m_pp);
    }

  };
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

QByteArray SocketRSI::subsXml(const QList<double> &vec, quint64 ipoc, int indent)
{
  QDomDocument doc;
  doc.setContent(defaultCommand);

  QDomElement root = doc.documentElement();
  QDomElement rk = root.firstChildElement("AKorr");
  QLocale c = QLocale::c();

  for (int i = 0; i < 6; ++i) {
    QString attrName = QString("A%1").arg(i + 1);
    rk.setAttribute(attrName, c.toString(vec.at(i), 'g', 10));
  }

  QDomElement ipocEl = root.firstChildElement("IPOC");
  ipocEl.firstChild().setNodeValue(QString::number(ipoc));

  return doc.toByteArray(indent);
}

QByteArray SocketRSI::subsIPOC(const QByteArray &xml, quint64 ipoc)
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

  while (xml.readNextStartElement()) {
    const auto name = xml.name();
    if (name == QLatin1String("Rob")) {
      xml.readNext();
      continue;
    }
    if (name == QLatin1String("AIPos")) {
      r.aiPos = readAxis6(xml.attributes());
      xml.skipCurrentElement();
    // } else if (name == QLatin1String("MACur")) {
    //   r.maCur = readAxis6(xml.attributes());
    //   xml.skipCurrentElement();
    } else if (name == QLatin1String("IPOC")) {
      r.ipoc = xml.readElementText().toULongLong();
    } else {
      xml.skipCurrentElement();
    }
  }
  return r;
}

std::array<double, 6> SocketRSI::readAxis6(const QXmlStreamAttributes &attrs)
{
  std::array<double, 6> out{};
  static const char* keys[6] = {"A1","A2","A3","A4","A5","A6"};
  for (size_t i = 0; i < 6; ++i) {
    out[i] = attrs.value(QLatin1String(keys[i])).toString().toDouble();
  }
  return out;
}






