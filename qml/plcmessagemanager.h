#ifndef PLCMESSAGEMANAGER_H
#define PLCMESSAGEMANAGER_H

#include <QByteArray>
#include <QDataStream>
#include <QString>
#include <QDateTime>
#include <QDataStream>
#include <QIODevice>
#include <QVariant>

class PlcMessageManager : public QObject
{

  Q_OBJECT

public:
  explicit PlcMessageManager(QObject* parent=nullptr);

  enum MessageError : quint8 {
    BAD_MAGIC = 0xE0,
    BAD_VER   = 0xE1,
    BAD_TYPE  = 0xE2,
    BAD_TID   = 0xE3,
    BAD_LEN   = 0xE4,
    BAD_CMD   = 0xE5,
    BAD_DEV   = 0xE6,
    BAD_MOD   = 0xE7,
    BAD_ADDR  = 0xE8,
    BAD_COUNT = 0xE9,
    BAD_AND   = 0xEA,
    BAD_OR    = 0xEB,
    BAD_DATA  = 0xEC,
    BAD_RAW   = 0xED,
    NOERR     = 0x00
  };
  Q_ENUM(MessageError)

  enum Type : quint8 {
    REQ      = 0xF1,
    RESP_OK  = 0x0D,
    RESP_ERR = 0xEE
  };
  Q_ENUM(Type)

  enum CMD : quint8 {
    READ_IO   = 0x0F,
    READ_REG  = 0xF0,
    WRITE_IO  = 0x3C,
    WRITE_REG = 0xC3,
    WRITE_RAW = 0xA5,
    SNAPSHOT  = 0x5A,
    NOCMD     = 0x00
  };
  Q_ENUM(CMD)

  enum DEV : quint16 {
    X = 0x0058,
    Y = 0x0059,
    D = 0x0044,
  };
  Q_ENUM(DEV)

  struct Header {
    quint16 magic = 0xAA55; // 0xAA55
    quint8  ver = 0x01;     // 0x01
    quint8  type;           // 0x10 REQ, 0x11 RESP_OK, 0x12 RESP_ERR
    quint8  tid;            // transaction id
    quint8  len;            // payload length
  };

  struct ParseBytes {
    QByteArray data;
    MessageError error = MessageError::NOERR;
    QVariant note = 0;
  };

  struct ParseHeader {
    Header header;
    MessageError error = MessageError::NOERR;
    QVariant note = 0;
  };

  struct ParseResp {
    QVariantMap data;
    MessageError error = MessageError::NOERR;
    QVariant note = 0;
  };

  /*
  struct Message {
    Header     header;       // decoded header
    QByteArray payload;      // exactly header.len bytes
    int        consumed = 0; // total bytes consumed (header + payload)
  };
  */

  ParseBytes buildReq(const QVariantMap& req, quint16 tid) const;
  ParseResp parseMessage(const QByteArray& message, quint8 exp_tid) const;

private:
  ParseBytes buildReqPayload(const QVariantMap& req) const;
  QByteArray buildHeader(Type type, quint8 tid, quint8 len) const;

  ParseHeader parseHeader(const QByteArray& header, quint8 exp_tid) const;
  ParseResp parseRespOk(quint8 cmd, const QByteArray& body) const;
  ParseResp parseRespErr(const QByteArray& payload) const;

  bool isValidType(quint8 type) const;
  bool isValidCmd(quint8 cmd) const;
  bool isValidDev(quint16 dev) const;
  bool isValidMod(quint8 module) const;

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
      {"NOCMD",     NOCMD}
    };
    return hash.value(strCmd);
  }


};

#endif // PLCMESSAGEMANAGER_H
