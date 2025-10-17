#include "socketrsi.h"

SocketRSI::SocketRSI(const QString& name, QObject *parent) : QUdpSocket{parent}
{
  setObjectName(name);

  connect(this, &SocketRSI::logMessage,  Logger::instance(), &Logger::push);
  connect(this, &SocketRSI::readyRead, this, &SocketRSI::onReadyRead);
  connect(this, &SocketRSI::errorOccurred, this, &SocketRSI::onErrorOccurred);
  connect(this, &SocketRSI::stateChanged,  this, &SocketRSI::onStateChanged);
}

// Q_INVOKABLE

void SocketRSI::startStreaming()
{

}

void SocketRSI::stopStreaming()
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
  m_pp = config.value("peerPort").toUInt();

  emit logMessage({QString("Socket configured:<br/>"
                   "&nbsp;&nbsp;Local: &nbsp;%1:%2<br/>"
                   "&nbsp;&nbsp;Peer: &nbsp;&nbsp;%3:%4<br/>").
                   arg(m_la.toString()).arg(m_lp).arg(m_pa.toString()).arg(m_pp),
                   1, objectName()});
}

// PUBLIC SLOTS

void SocketRSI::onReadyRead()
{

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




