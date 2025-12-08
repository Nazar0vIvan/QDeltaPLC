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

  enum MessageError : quint8 { // E, F
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
    BAD_CHG   = 0xF1, // 240
    BAD_VAR   = 0xF2, // 241
    NOERR     = 0x00
  };
  Q_ENUM(MessageError)

  enum Type : quint8 { // A
    REQ      = 0xA0,
    RESP_OK  = 0xA3,
    RESP_ERR = 0xAC,
    CHG      = 0xAF
  };
  Q_ENUM(Type)

  enum CMD : quint8 { // B
    READ_IO   = 0xB0,
    READ_REG  = 0xB3,
    WRITE_IO  = 0xB5,
    WRITE_REG = 0xB6,
    WRITE_RAW = 0xB9,
    SNAPSHOT  = 0xBA,
    SET_VAR   = 0xBC,
    NOCMD     = 0xBF
  };
  Q_ENUM(CMD)

  enum VAR_TYPE : quint8 { // D
    START_CELL = 0xC0,
    EXT_START  = 0xC3,
    PGNO       = 0xC5,
    PGNO_OK    = 0xC6,
    CONTINUE   = 0xC9,
    SFY_OK     = 0xCA,
    EXIT_CELL  = 0xCC,
  };
  Q_ENUM(VAR_TYPE);

  enum CHG_TYPE : quint8 { // C
    IOs      = 0xC0,
    AUT_EXT  = 0xC3,
    APPL_RUN = 0xC5
  };
  Q_ENUM(CHG_TYPE)

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
  ParseResult parseMessage(const QByteArray& message, quint8 exp_tid) const;

private:
  ParseResult buildReqPayload(const QVariantMap& req) const;
  QByteArray buildHeader(Type type, quint8 tid, quint8 len) const;

  ParseResult parseHeader(const QByteArray& headerBytesIn, quint8 exp_tid) const;
  ParseResult parseRespOk(const QByteArray& payload, quint8 tid, quint8 paylen) const;
  ParseResult parseRespErr(const QByteArray& payload) const;
  ParseResult parseStateChange(const QByteArray &payload, quint8 paylen) const;

  bool isValidType(quint8 type) const;
  bool isValidCmd(quint8 cmd) const;
  bool isValidDev(quint16 dev) const;
  bool isValidMod(quint8 module) const;
  bool isValidVar(quint8 var) const;

  QVariantList byteToBits(quint8 value) const;

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
