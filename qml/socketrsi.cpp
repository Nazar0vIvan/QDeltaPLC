#include "socketrsi.h"

SocketRSI::SocketRSI(const QString& name, QObject *parent) : QUdpSocket{parent}
{
  connect(this, &SocketRSI::logMessage,  Logger::instance(), &Logger::push);
}

void SocketRSI::parseConfigFile(const QUrl &url)
{
  QString filePath = url.toLocalFile();
  QFile configFile(filePath);

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
  QString rsiLocalAddress = configElements.namedItem("IP_NUMBER").toElement().text();
  int rsiLocalPort = configElements.namedItem("PORT").toElement().text().toInt();

  QString rsiOpenModeText = configElements.namedItem("ONLYSEND").toElement().text();
  QIODeviceBase::OpenModeFlag rsiOpenMode = rsiOpenModeText == "TRUE" ? QIODeviceBase::ReadOnly : "FALSE" ? QIODeviceBase::ReadWrite : QIODeviceBase::NotOpen;

  QString pattern("[0-9]{1,3}\\.[0-9]{1,3}\\.[0-9]{1,3}\\.[0-9]{1,3}");
  QRegularExpression re(pattern);
  QRegularExpressionMatch match = re.match(rsiLocalAddress);

  if(!rsiLocalPort || !match.hasMatch()){
    emit logMessage({"Invalid IP or port: " + QString::number(rsiLocalPort) + ":" + rsiLocalAddress, 0, objectName()});
    return;
  }



  setLocalAddress(QHostAddress(rsiLocalAddress));
  setLocalPort(rsiLocalPort);
  setOpenMode(rsiOpenMode);

}
