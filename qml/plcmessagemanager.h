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
  quint16 tid;            // transaction id
  quint16 len;            // payload length
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

inline constexpr quint16 MAGIC = 0xAA55;
inline constexpr quint8  VER   = 0x01;

struct Message {
  Header     header;       // decoded header
  QByteArray payload;      // exactly header.len bytes
  int        consumed = 0; // total bytes consumed (header + payload)
};

enum class ParseKind { NeedMore, Good, Drop };

/*
struct ParseResult {
  ParseKind kind;  // parser verdict
  Message   msg;   // valid if kind==Good
  int       drop;  // bytes to drop if kind==Drop
  QString   note;  // human-readable reason (e.g., "MAGIC mismatch")
};
*/

struct PendingReq {
  QVariantMap req; // original QML map
  QDateTime   ts;  // send time
};

struct BuiltFrame {
  quint16    tid;   // assigned TID
  QByteArray bytes; // full frame (header + payload), CRC omitted by design
};

// ----------------------------------------------------------------

class PlcMessageManager : public QObject
{

  Q_OBJECT

public:
  explicit PlcMessageManager(QObject* parent=nullptr);

  QByteArray buildReq(const QVariantMap& req, quint16 tid) const;
  QVariantMap parseMessage(const QByteArray& message) const;

private:
  QByteArray buildReqPayload(const QVariantMap& req) const;
  bool parseHeader(const QByteArray& first8, Header& h) const;
  QByteArray makeHeader(Type type, quint16 tid, quint16 len) const;

  QVariantMap parseRespOk(quint8 cmd, const QByteArray& body) const;
  QVariantMap parseRespErr(const QByteArray& payload) const;
};

#endif // PLCMESSAGEMANAGER_H
