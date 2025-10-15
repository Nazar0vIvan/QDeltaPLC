#include "socketrsi.h"

SocketRSI::SocketRSI(const QString& name, QObject *parent) : QUdpSocket{parent}
{
  setObjectName(name);

  connect(this, &SocketRSI::logMessage,  Logger::instance(), &Logger::push);
  connect(this, &SocketRSI::readyRead, this, &SocketRSI::onReadyRead);
  connect(this, &SocketRSI::errorOccurred, this, &SocketRSI::onErrorOccurred);
  connect(this, &SocketRSI::stateChanged,  this, &SocketRSI::onStateChanged);
}

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
  QHostAddress la = QHostAddress(config.value("localAddress").toString());
  qint16 lp = config.value("localPort").toUInt();
  QHostAddress pa = QHostAddress(config.value("peerAddress").toString());
  qint16 pp = config.value("peerPort").toUInt();


  if (!bind(la,lp,QAbstractSocket::ReuseAddressHint)) {
    emit logMessage({"binding failed", 0, objectName()});
    return;
  }

  setLocalAddress(la); setLocalPort(lp); setPeerAddress(pa); setPeerPort(pp);
  emit logMessage({stateToString(state()) + "<br/>" +
                  "[local address]: " + la.toString() + "<br/>" +
                  "[local port]: " + QString::number(lp) + "<br/>" +
                  "[peer address]: " + pa.toString() + "<br/>" +
                  "[peer port]: " + QString::number(pp) + "<br/>" +
                  "----------",
                  1, objectName()});
}

void SocketRSI::unbind()
{
  close();
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
    default: return "UnconnectedState";
  }
}

void SocketRSI::onErrorOccurred(QAbstractSocket::SocketError socketError) {
  emit logMessage({this->errorString(), 0, objectName()});
}

void SocketRSI::onStateChanged(QAbstractSocket::SocketState state) {
  emit logMessage({stateToString(state), 2, objectName()});
}

void SocketRSI::onReadyRead()
{

}




