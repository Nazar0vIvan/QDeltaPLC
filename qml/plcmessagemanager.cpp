#include "plcmessagemanager.h"

PlcMessageManager::PlcMessageManager(QObject* parent) : QObject(parent) {
  QVariantMap req = {
    {"raw", "yUix"},
    {"module", 2}
  };
}

PlcMessageManager::ParseBytes PlcMessageManager::buildReq(const QVariantMap& req, quint16 tid) const {
  ParseBytes buildResult = buildReqPayload(req);
  if (!buildResult.error) {
    return {0, buildResult.error, buildResult.note};
  }
  QByteArray payload = buildResult.data;
  QByteArray header = buildHeader(Type::REQ, tid, payload.size());
  return { header + payload };
}

PlcMessageManager::ParseResp PlcMessageManager::parseMessage(const QByteArray& message, quint8 exp_tid) const
{
  ParseHeader prHeader = parseHeader(message.left(8), exp_tid);
  if (prHeader.error)
    return { QVariantMap(),  prHeader.error, prHeader.note };

  Header header = prHeader.header;

  const int total = 8 + header.len;
  if (message.size() != total) {
    return { QVariantMap(),  MessageError::BAD_LEN, QByteArray(1, message.size())};
  }

  QByteArray payload = message.mid(8, header.len);
  QDataStream ds(payload);
  ds.setByteOrder(QDataStream::BigEndian);

  switch (header.type) {
    case Type::RESP_OK: {
      quint8 cmd; quint8 status; QByteArray body;
      ds >> cmd >> status >> body;
      ParseResp resp = parseRespOk(cmd, body);
      resp["type"]   = "RESP_OK";
      resp["cmd"]    = cmd;
      resp["status"] = status;
      resp["tid"]    = int(h.tid);
      return resp;
      break;
    }
    default:
      break;
  }



  }

  if (h.type == Type::RESP_ERR) {
    ParseResp resp = parseRespErr(payload);
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

PlcMessageManager::ParseBytes PlcMessageManager::buildReqPayload(const QVariantMap& req) const {

  QByteArray payload;
  QDataStream ds(&payload, QIODevice::WriteOnly);
  ds.setByteOrder(QDataStream::BigEndian);

  if(!req.value("cmd").canConvert<quint8>()) {
    return {0, MessageError::BAD_CMD, 0};
  }

  quint8 cmd = req.value("cmd").toUInt();
  if (!isValidCmd(cmd))
    return {0, MessageError::BAD_CMD, QByteArray(1, cmd)};

  const quint8 FLAGS = 0x00; // reserved = 0
  ds << quint8(cmd) << quint8(FLAGS);

  switch (cmd) {
    case CMD::READ_IO: {
      if(!req.value("dev").isNull() && !req.value("dev").canConvert<quint16>())
        return {0, MessageError::BAD_DEV, 0};
      quint16 dev = quint16(req.value("dev").toUInt());
      if (!isValidDev(dev))
        return {0, MessageError::BAD_DEV, QByteArray(2, dev)};

      if(!req.value("module").isNull() && !req.value("module").canConvert<quint8>())
        return {0, MessageError::BAD_MOD, 0};
      quint16 module = quint16(req.value("module").toUInt());

      ds << dev <<  module;
      break;
    }
    case CMD::READ_REG: {
      if(!req.value("addr").isNull() && !req.value("addr").canConvert<quint16>())
        return {0, MessageError::BAD_ADDR, 0};
      quint16 addr = quint16(req.value("dev").toUInt());

      if(!req.value("count").isNull() && !req.value("count").canConvert<quint16>())
        return {0, MessageError::BAD_COUNT, 0};
      quint16 count = quint16(req.value("count").toUInt());
      ds << quint8(DEV::D) << addr << count;
      break;
    }
    case CMD::WRITE_IO: {
      if(!req.value("module").isNull() && !req.value("module").canConvert<quint8>())
        return {0, MessageError::BAD_MOD, 0};
      quint8 module = quint8(req.value("module").toUInt());
      if (!isValidMod(module))
        return {0, MessageError::BAD_MOD, 0};

      if(!req.value("andMask").isNull() && !req.value("andMask").canConvert<quint8>())
        return {0, MessageError::BAD_AND, 0};
      quint8 andMask = quint8(req.value("andMask").toUInt());

      if(!req.value("orMask").isNull() && !req.value("orMask").canConvert<quint8>())
        return {0, MessageError::BAD_OR, 0};
      quint8 orMask = quint8(req.value("orMask").toUInt());

      ds << quint8(DEV::Y) << module << andMask << orMask;
      break;
    }
    case CMD::WRITE_REG: {
      if(!req.value("addr").isNull() && !req.value("addr").canConvert<quint16>())
        return {0, MessageError::BAD_ADDR, 0};
      quint16 addr = quint16(req.value("addr").toUInt());

      if(!req.value("data").isNull() && !req.value("data").canConvert<quint16>())
        return {0, MessageError::BAD_DATA, 0};

      quint16 data = quint16(req.value("data").toUInt());

      ds << addr << data;
      break;
    }
    case CMD::WRITE_RAW: {
      if(!req.value("raw").isNull() && !req.value("raw").canConvert<QByteArray>())
        return {0, MessageError::BAD_RAW, 0};
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
  return { payload };
}

PlcMessageManager::ParseHeader PlcMessageManager::parseHeader(const QByteArray& first8, quint8 exp_tid) const
{
  QDataStream in(const_cast<QByteArray&>(first8));
  in.setByteOrder(QDataStream::BigEndian);
  Header h;
  in >> h.magic >> h.ver >> h.type >> h.tid >> h.len;
  
  if (h.magic != MAGIC)
    return {h, MessageError::BAD_MAGIC, QVariant::fromValue(h.magic)};
  
  if (h.ver != VER)
    return {h, MessageError::BAD_VER, h.ver};

  if (!isValidType(h.type))
    return {h, MessageError::BAD_TYPE, h.type};
  
  if (h.tid != exp_tid)
    return {h, MessageError::BAD_TID, h.tid};

  return { h };
}

QByteArray PlcMessageManager::buildHeader(Type type, quint8 tid, quint8 len) const {
  QByteArray out;
  QDataStream ds(&out, QIODevice::WriteOnly);
  ds.setByteOrder(QDataStream::BigEndian);
  ds << MAGIC << quint8(VER) << quint8(type) << quint8(tid) << quint8(len);
  return out;
}

PlcMessageManager::ParseResp PlcMessageManager::parseRespOk(quint8 cmd, const QByteArray& body) const
{
  QDataStream ds(body);
  ds.setByteOrder(QDataStream::BigEndian);
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

/*
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
*/

bool PlcMessageManager::isValidType(quint8 type) const
{
  return type == Type::REQ || type == Type::RESP_OK || type == Type::RESP_ERR;
}

bool PlcMessageManager::isValidCmd(quint8 cmd) const
{
  return cmd == CMD::READ_IO ||
         cmd == CMD::READ_REG ||
         cmd == CMD::WRITE_IO ||
         cmd == CMD::WRITE_REG ||
         cmd == CMD::WRITE_RAW ||
         cmd == CMD::SNAPSHOT;
}

bool PlcMessageManager::isValidMod(quint8 module) const
{
  return module == 1 || module == 2;
}

bool PlcMessageManager::isValidDev(quint16 dev) const
{
  return dev == DEV::X ||
         dev == DEV::Y ||
         dev == DEV::D;
}









