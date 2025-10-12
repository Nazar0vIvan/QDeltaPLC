#include "socketrsi.h"

SocketRSI::SocketRSI(const QString& name, QObject *parent) : QUdpSocket{parent}
{
  connect(this, &SocketRSI::logMessage,  Logger::instance(), &Logger::push);
  connect(this, &SocketRSI::readyRead, this, &SocketRSI::onReadyRead);
  connect(this, &SocketRSI::errorOccurred, this, &SocketRSI::onErrorOccurred);
  connect(this, &SocketRSI::stateChanged,  this, &SocketRSI::onStateChanged);
}

void SocketRSI::parseConfigFile(const QUrl &url)
{
  QFile configFile(url.toLocalFile());

  if(!configFile.open(QIODevice::ReadOnly)){
    emit logMessage({configFile.errorString(), 0, objectName()});
    return;
  }

  QDomDocument dom;
  QDomDocument::ParseResult parseResult = dom.setContent(&configFile);
  if(!parseResult) {
    emit logMessage({parseResult.errorMessage + ": " + QString::number(parseResult.errorLine) + ":" + QString::number(parseResult.errorColumn), 0, objectName()});
    return;
  }

  QDomElement configElements = dom.documentElement().firstChildElement("CONFIG");
  QHostAddress la = QHostAddress(configElements.namedItem("IP_NUMBER").toElement().text());
  int lp = configElements.namedItem("PORT").toElement().text().toInt();

  QString s_om = configElements.namedItem("ONLYSEND").toElement().text();
  QIODeviceBase::OpenModeFlag om = s_om == "TRUE" ? QIODeviceBase::ReadOnly : "FALSE" ? QIODeviceBase::ReadWrite : QIODeviceBase::NotOpen;

  QString pattern("[0-9]{1,3}\\.[0-9]{1,3}\\.[0-9]{1,3}\\.[0-9]{1,3}");
  QRegularExpression re(pattern);
  QRegularExpressionMatch match = re.match(la.toString());

  if(!lp || !match.hasMatch()){
    emit logMessage({"Invalid IP or port: " + QString::number(lp) + ":" + la.toString(), 0, objectName()});
    return;
  }

  if (!bind(la,lp)) {
    emit logMessage({"binding failed", 0, objectName()});
    return;
  }

  setLocalAddress(la); setLocalPort(lp); setOpenMode(om);
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




