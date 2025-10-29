#ifndef PLCMESSAGEMANAGER_H
#define PLCMESSAGEMANAGER_H

#include <QQmlEngine>
#include <QByteArray>
#include <QDataStream>
#include <QString>
#include <QDateTime>
#include <QIODevice>
#include <QVariant>
#include <QVariantMap>
#include <QMetaType>
#include <QList>



class PlcMessageManager : public QObject
{

  Q_OBJECT

public:
  explicit PlcMessageManager(QObject* parent=nullptr);

  enum MessageError : quint8 {
    BAD_MAGIC = 0xE0, // 224
    BAD_VER   = 0xE1, // 225
    BAD_TYPE  = 0xE2, // 226
    BAD_TID   = 0xE3, // 227
    BAD_MLEN  = 0xE4, // 228
    BAD_PLEN  = 0xE5, // 229
    BAD_CMD   = 0xE6, // 230
    BAD_DEV   = 0xE7, // 231
    BAD_MOD   = 0xE8, // 232
    BAD_ADDR  = 0xE9, // 233
    BAD_COUNT = 0xEA, // 234
    BAD_AND   = 0xEB, // 235
    BAD_OR    = 0xEC, // 236
    BAD_DATA  = 0xED, // 237
    BAD_RAW   = 0xEE, // 238
    BAD_RESP  = 0xEF, // 239
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
    quint16 magic = 0xAA55;
    quint8  ver = 0x01;
    quint8  type;
    quint8  tid;
    quint8  len;
  };

  struct ParseResult {
    QVariant data; // QByteArray, Header, QVariantMap
    MessageError error = MessageError::NOERR;
    QVariant note = 0;

    bool ok() const { return error == NOERR; }
  };

  ParseResult buildReq(const QVariantMap& req, quint8 tid) const;
  ParseResult parseResp(const QByteArray& message, quint8 exp_tid) const;

private:
  ParseResult buildReqPayload(const QVariantMap& req) const;
  QByteArray buildHeader(Type type, quint8 tid, quint8 len) const;

  ParseResult parseHeader(const QByteArray& headerBytesIn, quint8 exp_tid) const;
  ParseResult parseRespOk(const QByteArray& payload, quint8 tid) const;
  ParseResult parseRespErr(const QByteArray& payload) const;

  bool isValidType(quint8 type) const;
  bool isValidCmd(quint8 cmd) const;
  bool isValidDev(quint16 dev) const;
  bool isValidMod(quint8 module) const;

  QVariantList byteToBitVariantList(quint8 value) const;

  static constexpr quint16 MAGIC = 0xAA55;
  static constexpr quint8  VER   = 0x01;
  static constexpr int HEADER_SIZE = 6; // bytes actually written
  static constexpr int RESP_SIZE = 32;  // resp size in bytes

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

Q_DECLARE_METATYPE(PlcMessageManager::Header)

#endif // PLCMESSAGEMANAGER_H
