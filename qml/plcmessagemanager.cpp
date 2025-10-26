#include "plcmessagemanager.h"

PlcMessageManager::PlcMessageManager(QObject* parent) : QObject(parent) {
  QVariantMap req = {
    {"raw", "yUix"},
    {"module", 2}
  };

  QByteArray body;
  QDataStream ds(&body, QIODevice::WriteOnly);
  const QByteArray raw = req.value("raw").toByteArray();
  ds.writeRawData(raw.constData(), raw.size());

  const CMD cmd = str2cmd("boo");
  // qDebug() << body.toHex(' ');
}

BuildResult PlcMessageManager::buildReq(const QVariantMap& req, quint16 tid) const {
  BuildResult brPayload = buildReqPayload(req);
  if (!brPayload.status) {
    return {false, 0, brPayload.note};
  }

  BuildResult brHeader = buildHeader(Type::REQ, tid, brPayload.data.size());
  if (!brHeader.status) {
    return {false, 0, brHeader.note}
  }

  return {true, brHeader.data + brPayload.data, ""};
}

QVariantMap PlcMessageManager::parseMessage(const QByteArray& message) const {
  if (message.size() < 8)
    return {{"type", "PARSE_ERR"}, {"error", "SHORT_HEADER"}, {"detail", QString::number(message.size())}};

  if (!parseHeader(message.left(8)))
    return {{"type", "PARSE_ERR"}, {"error", "HEADER_READ_FAILED"}};

  if (h.magic != MAGIC)
    return {{"type", "PARSE_ERR"}, {"error", "BAD_MAGIC"}, {"detail", QString("0x%1").arg(h.magic,4,16,QChar('0'))}};

  if (h.ver != VER)
    return {{"type", "PARSE_ERR"}, {"error", "BAD_VER"}, {"detail", QString::number(h.ver)}};

  const int total = 8 + int(h.len);
  if (message.size() != total)
    return {{"type", "PARSE_ERR"}, {"error", "LEN_MISMATCH"}, {"detail", QString("expected %1, got %2").arg(total).arg(message.size())}};

  const QByteArray payload = message.mid(8, h.len);

  if (h.type == Type::RESP_OK) {
    if (payload.size() < 2)
      return {{"type", "PARSE_ERR"}, {"error", "RESP_OK_TOO_SHORT"}};

    const quint8 cmd   = quint8(payload[0]);
    const quint8 status  = quint8(payload[1]);
    const QByteArray body = payload.mid(2);

    QVariantMap resp = parseRespOk(cmd, body);
    resp["type"]   = "RESP_OK";
    resp["cmd"]    = cmd;
    resp["status"] = status;
    resp["tid"]    = int(h.tid);
    return resp;
  }

  if (h.type == Type::RESP_ERR) {
    QVariantMap resp = parseRespErr(payload);
    resp["type"] = "RESP_ERR";
    resp["tid"]  = int(h.tid);
    return resp;
  }

  if (h.type == Type::REQ) {
    // Optional: parse incoming REQ if you ever need to (sniffer/loopback).
    // For now, treat as unsupported for this path.
    return {{"type", "PARSE_ERR"}, {"error", "UNEXPECTED_TYPE_REQ"}};
  }

  return {{"type", "PARSE_ERR"}, {"error", "UNKNOWN_TYPE"}, {"detail", QString::number(h.type)}};
}

// PRIVATE

BuildResult PlcMessageManager::buildReqPayload(const QVariantMap& req) const {

  QByteArray payload;
  QDataStream ds(&payload, QIODevice::WriteOnly);
  ds.setByteOrder(QDataStream::BigEndian);

  CMD cmd = str2cmd(req.value("cmd").toString());
  const quint8 FLAGS = 0x00; // reserved = 0
  ds << quint8(cmd) << quint8(FLAGS);

  switch (cmd) {
    case CMD::READ_IO: {
      ds << req.value("dev").toString().toLatin1();
      ds << quint8(req.value("module").toUInt());
      break;
    }
    case CMD::READ_REG: {
      ds << quint8(DEV::D);
      ds << quint16(req.value("addr").toUInt());
      ds << quint16(req.value("count").toUInt());
      break;
    }
    case CMD::WRITE_IO: {
      ds << quint8(DEV::Y);
      ds << quint8(req.value("mod").toUInt());
      ds << quint8(req.value("and").toUInt());
      ds << quint8(req.value("or").toUInt());
      break;
    }
    case CMD::WRITE_REG: {
      ds << quint16(req.value("addr").toUInt());
      ds << quint16(req.value("data").toUInt());
      break;
    }
    case CMD::WRITE_RAW: {
      const QByteArray raw = req.value("raw").toByteArray();
      ds.writeRawData(raw.constData(), raw.size());
      break;
    }
    case CMD::SNAPSHOT: {
      break;
    }
    default:
      break;
  }
  return {true, payload, ""};
}

BuildResult PlcMessageManager::parseHeader(const QByteArray& first8) const {
  QByteArray header;
  QDataStream ds(const_cast<QByteArray&>(first8)); // safe: operating on a local copy/slice
  ds.setByteOrder(QDataStream::BigEndian);


  ds >> h.magic >> h.ver >> h.type >> h.tid >> h.len;
  return {true, header, ""};
}

QByteArray PlcMessageManager::buildHeader(Type type, quint16 tid, quint16 len) const {
  QByteArray out;
  out.reserve(8);
  QDataStream ds(&out, QIODevice::WriteOnly);
  ds.setByteOrder(QDataStream::BigEndian);
  ds << MAGIC << quint8(VER) << quint8(type) << quint16(tid) << quint16(len);
  // qDebug() << "HEADER: " << out.toHex(' ') << ", size = " << out.size();
  return out;
}

QVariantMap PlcMessageManager::parseRespOk(quint8 cmd, const QByteArray& body) const {
  QBuffer b;
  b.setData(body);
  b.open(QIODevice::ReadOnly);

  QDataStream ds(&b);
  ds.setByteOrder(QDataStream::LittleEndian);

  QVariantMap m;

  switch (cmd) {
    case CMD::READ_IO: {
        if (body.size() != 3)
          return {{"type","PARSE_ERR"},{"error","READ_IO_BODY_SIZE"}};
        quint8 dev, mod, data;
        ds >> dev >> mod >> data;
        if (dev != DEV::X && dev != DEV::Y)
          return {{"type","PARSE_ERR"},{"error","READ_IO_BAD_DEV"}};
        m["cmd"]  = "READ_IO";
        m["dev"]  = (dev == DEV::Y) ? "Y" : "X";
        m["mod"]  = int(mod);
        m["data"] = int(data);
        return m;
      }
    case CMD::READ_REG: {
        if (body.size() < 4)
          return {{"type","PARSE_ERR"},{"error","READ_REG_BODY_SHORT"}};
        quint8 dev, rsv;
        quint16 count;
        ds >> dev >> rsv >> count;
        const int expect = 4 + count*2;
        if (body.size() != expect)
          return {{"type","PARSE_ERR"},{"error","READ_REG_COUNT_MISMATCH"}};
        QVariantList regs;
        regs.reserve(count);
        for (quint16 i = 0; i < count; i++) {
          quint16 w;
          ds >> w;
          regs << int(w);
        }
        m["cmd"]   = "READ_REG";
        m["count"] = int(count);
        m["data"]  = regs;
        return m;
      }

    case CMD::WRITE_IO: {
        if (body.size() != 3)
          return {{"type","PARSE_ERR"},{"error","WRITE_IO_BODY_SIZE"}};
        quint8 dev, mod, newdata;
        ds >> dev >> mod >> newdata;
        m["cmd"]     = "WRITE_IO";
        m["mod"]     = int(mod);
        m["newdata"] = int(newdata);
        return m;
      }

    case CMD::WRITE_REG: {
        if (body.size() != 4)
          return {{"type","PARSE_ERR"},{"error","WRITE_REG_BODY_SIZE"}};
        quint16 addr, count;
        ds >> addr >> count;
        m["cmd"]   = "WRITE_REG";
        m["addr"]  = int(addr);
        m["count"] = int(count);
        return m;
      }

    case CMD::SNAPSHOT: {
        if (body.size() != 4)
          return {{"type","PARSE_ERR"},{"error","SNAPSHOT_BODY_SIZE"}};
        quint8 di1, do1, di2, do2;
        ds >> di1 >> do1 >> di2 >> do2;
        m["cmd"] = "SNAPSHOT";
        m["di1"] = int(di1);
        m["do1"] = int(do1);
        m["di2"] = int(di2);
        m["do2"] = int(do2);
        return m;
      }
    default:
      return {{"type","PARSE_ERR"},{"error","UNKNOWN_CMD"},{"detail",QString::number(cmd)}};
  }
}

QVariantMap PlcMessageManager::parseRespErr(const QByteArray& payload) const {
  if (payload.size() != 4)
    return {{"type","PARSE_ERR"},{"error","RESP_ERR_BODY_SIZE"}};

  QBuffer b;
  b.setData(payload);
  b.open(QIODevice::ReadOnly);

  QDataStream ds(&b);
  ds.setByteOrder(QDataStream::LittleEndian);
  quint8 cmd, err;
  quint16 code;
  ds >> cmd >> err >> code;

  return { {"cmd", cmd}, {"err", err}, {"code", code} };
}









