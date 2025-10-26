#ifndef PLCMESSAGEMANAGER_H
#define PLCMESSAGEMANAGER_H

#include <QByteArray>
#include <QDataStream>
#include <QString>
#include <QDateTime>
#include <QDataStream>
#include <QBuffer>

struct Header {
  quint16 magic = 0xAA55; // 0xAA55
  quint8  ver = 0x01;     // 0x01
  quint8  type;           // 0x10 REQ, 0x11 RESP_OK, 0x12 RESP_ERR
  quint8  tid;            // transaction id
  quint8  len;            // payload length

  QByteArray toByteArray() {
    return
  }
};

enum Type : quint8 {
  REQ      = 0xF1,
  RESP_OK  = 0x0D,
  RESP_ERR = 0xEE
};

enum CMD : quint8 {
  READ_IO   = 0x0F,
  READ_REG  = 0xF0,
  WRITE_IO  = 0x3C,
  WRITE_REG = 0xC3,
  WRITE_RAW = 0xA5,
  SNAPSHOT  = 0x5A,
  NONE      = 0x00
};

enum DEV : quint8 {
  X = 0x58,
  Y = 0x59,
  D = 0x44,
};

struct Message {
  Header     header;       // decoded header
  QByteArray payload;      // exactly header.len bytes
  int        consumed = 0; // total bytes consumed (header + payload)
};

/*
struct ParseResult {
  ParseKind kind;  // parser verdict
  Message   msg;   // valid if kind==Good
  int       drop;  // bytes to drop if kind==Drop
  QString   note;  // human-readable reason (e.g., "MAGIC mismatch")
};
*/

struct ParseResult {

};

struct BuildResult {
  bool status;
  QByteArray data;
  QString note;
};

// ----------------------------------------------------------------

class PlcMessageManager : public QObject
{

  Q_OBJECT

public:
  explicit PlcMessageManager(QObject* parent=nullptr);

  BuildResult buildReq(const QVariantMap& req, quint16 tid) const;
  QVariantMap parseMessage(const QByteArray& message) const;

private:
  BuildResult buildReqPayload(const QVariantMap& req) const;
  QByteArray buildHeader(Type type, quint16 tid, quint16 len) const;

  BuildResult parseHeader(const QByteArray& header) const;
  QVariantMap parseRespOk(quint8 cmd, const QByteArray& body) const;
  QVariantMap parseRespErr(const QByteArray& payload) const;

  const quint16 MAGIC = 0xAA55;
  const quint8 VER = 0x01;


  DEV str2dev(const QString& strDev) const {
    const QHash<QString, DEV> hash = {
      {"X", DEV::X},
      {"Y", DEV::Y},
      {"D", DEV::D}
    };
    return hash.value(strDev);
  }

  CMD str2cmd(const QString& strCmd) const {
    const QHash<QString, CMD> hash = {
      {"READ_IO",   READ_IO},
      {"READ_REG",  READ_REG},
      {"WRITE_IO",  WRITE_IO},
      {"WRITE_REG", WRITE_REG},
      {"WRITE_RAW", WRITE_RAW},
      {"SNAPSHOT",  SNAPSHOT},
      {"NONE",      NONE}
    };
    return hash.value(strCmd);
  }
};

#endif // PLCMESSAGEMANAGER_H
