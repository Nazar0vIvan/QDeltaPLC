#include "plcmessagemanager.h"

PlcMessageManager::PlcMessageManager(QObject* parent) : QObject(parent) {}

// PUBLIC

PlcMessageManager::ParseResult PlcMessageManager::buildReq(const QVariantMap& req, quint8 tid) const {
  ParseResult payloadRes = buildReqPayload(req);
  if (!payloadRes.ok()) {
    return payloadRes;
  }
  QByteArray payload = payloadRes.data.toByteArray();
  QByteArray header = buildHeader(Type::REQ, tid, static_cast<quint8>(payload.size()));
  return { header + payload };
}

PlcMessageManager::ParseResult PlcMessageManager::parseMessage(const QByteArray& resp, quint8 exp_tid) const
{
  if (resp.size() < HEADER_SIZE)
    return { QVariantMap(), BAD_MLEN, resp.size() };

  ParseResult hdrRes = parseHeader(resp.left(HEADER_SIZE), exp_tid);
  if (!hdrRes.ok())
    return hdrRes;

  Header header = hdrRes.data.value<Header>();

  if (resp.size() != RESP_SIZE)
    return { QVariantMap(), BAD_PLEN, resp.size()};

  QByteArray payload = resp.mid(HEADER_SIZE, header.len);
  switch (header.type) {
    case Type::RESP_OK:
      return parseRespOk(payload, exp_tid, header.len);
    case Type::RESP_ERR:
      return parseRespErr(payload);
    case Type::CHG:
      return parseStateChange(payload, header.len);
    default:
      return { QVariantMap(), BAD_RESP, header.type };
  }
}

// PRIVATE

PlcMessageManager::ParseResult PlcMessageManager::buildReqPayload(const QVariantMap& req) const {

  QByteArray payload;
  QDataStream ds(&payload, QIODevice::WriteOnly);
  ds.setByteOrder(QDataStream::BigEndian);

  if(!req.value("cmd").canConvert<quint8>()) {
    return { 0, BAD_CMD, {} };
  }

  quint8 cmd = req.value("cmd").toUInt();
  if (!isValidCmd(cmd))
    return {0, BAD_CMD, cmd};

  const quint8 FLAGS = 0x00; // reserved = 0
  ds << cmd << FLAGS;

  switch (cmd) {
    case CMD::READ_IO: {
      if(!req.value("dev").isNull() && !req.value("dev").canConvert<quint16>())
          return { QVariant(), BAD_DEV, {} };
      quint16 dev = static_cast<quint16>(req.value("dev").toUInt());
      if (!isValidDev(dev))
        return { QVariant(), BAD_DEV, dev };

      if(!req.value("module").isNull() && !req.value("module").canConvert<quint8>())
        return { QVariant(), BAD_MOD, {} };
      quint8 module = static_cast<quint8>(quint8(req.value("module").toUInt()));
      if (!isValidMod(module))
        return { QVariant(), BAD_MOD, module };

      ds << dev <<  quint8() << module;
      break;
    }
    case CMD::READ_REG: {
      if(!req.value("addr").isNull() && !req.value("addr").canConvert<quint16>())
          return { QVariant(), BAD_ADDR, {}} ;
      quint16 addr = static_cast<quint16>(req.value("addr").toUInt());

      ds << static_cast<quint16>(DEV::D) << addr;
      break;
    }
    case CMD::WRITE_IO: {
      if(!req.value("module").isNull() && !req.value("module").canConvert<quint8>())
        return { QVariant(), BAD_MOD, {} };
      quint8 module = static_cast<quint8>(req.value("module").toUInt());
      if (!isValidMod(module))
        return { QVariant(), BAD_MOD, module };

      if(!req.value("andMask").isNull() && !req.value("andMask").canConvert<quint8>())
        return { QVariant(), BAD_AND, {} };
      quint8 andMask = static_cast<quint8>(req.value("andMask").toUInt());

      if(!req.value("orMask").isNull() && !req.value("orMask").canConvert<quint8>())
        return { QVariant(), BAD_OR, {} };
      quint8 orMask = static_cast<quint8>(req.value("orMask").toUInt());

      ds << static_cast<quint16>(DEV::Y) << quint8() << module << andMask << orMask;
      break;
    }
    case CMD::WRITE_REG: {
      if(!req.value("addr").isNull() && !req.value("addr").canConvert<quint16>())
        return { QVariant(), BAD_ADDR, {} };
      quint16 addr = static_cast<quint16>(req.value("addr").toUInt());

      if(!req.value("value").isNull() && !req.value("value").canConvert<quint16>())
        return { QVariant(), BAD_DATA, {} };
      quint16 value = static_cast<quint16>(req.value("value").toUInt());

      ds << static_cast<quint16>(DEV::D) << addr << value;
      break;
    }
    case CMD::WRITE_RAW: {
      if(!req.value("raw").isNull() && !req.value("raw").canConvert<QByteArray>())
        return { QVariant(), BAD_RAW, {} };
      QByteArray raw = req.value("raw").toByteArray();
      ds.writeRawData(raw.constData(), raw.size());
      break;
    }
    case CMD::SNAPSHOT: {
      break;
    }
    case CMD::SET_VAR: {
      if(!req.value("var").isNull() && !req.value("var").canConvert<quint8>())
        return { QVariant(), BAD_VAR, {} };
      quint8 var = static_cast<quint8>(req.value("var").toUInt());
      if(!req.value("attr").isNull() && !req.value("attr").canConvert<quint8>())
        return { QVariant(), BAD_VAR, {} };
      quint8 attr = static_cast<quint8>(req.value("attr").toUInt());
      ds << var << attr;
    }
    default:
      break;
  }
  return { payload };
}

QByteArray PlcMessageManager::buildHeader(Type type, quint8 tid, quint8 len) const {
  QByteArray out;
  QDataStream ds(&out, QIODevice::WriteOnly);
  ds.setByteOrder(QDataStream::BigEndian);
  ds << MAGIC << quint8(VER) << quint8(type) << quint8(tid) << quint8(len);
  return out;
}

PlcMessageManager::ParseResult PlcMessageManager::parseHeader(const QByteArray& headerBytesIn, quint8 exp_tid) const
{
  if (headerBytesIn.size() < HEADER_SIZE) {
    return { QVariant(), BAD_MLEN, headerBytesIn.size() };
  }

  QByteArray headerBytes = headerBytesIn.left(HEADER_SIZE);
  QDataStream in(&headerBytes, QIODevice::ReadOnly);
  in.setByteOrder(QDataStream::BigEndian);

  Header h;
  in >> h.magic >> h.ver >> h.type >> h.tid >> h.len;

  if (h.magic != MAGIC)
    return {QVariant::fromValue(h), BAD_MAGIC, h.magic};

  if (h.ver != VER)
    return {QVariant::fromValue(h), BAD_VER, h.ver};

  if (!isValidType(h.type))
    return {QVariant::fromValue(h), BAD_TYPE, h.type};

  if (h.type != Type::CHG && h.tid != exp_tid)
    return { QVariant::fromValue(h), BAD_TID, h.tid };

  return { QVariant::fromValue(h) };
}

PlcMessageManager::ParseResult PlcMessageManager::parseRespOk(const QByteArray& payload, quint8 tid, quint8 paylen) const
{
  QDataStream ds(payload);
  ds.setByteOrder(QDataStream::BigEndian);
  quint8 cmd, status;
  ds >> cmd >> status;

  if (!isValidCmd(cmd))
    return { QVariant(), BAD_CMD, cmd };

  QVariantMap out;
  out["type"]   = Type::RESP_OK;
  out["tid"]    = tid;
  out["cmd"]    = cmd;
  out["status"] = status;

  switch (cmd) {
    case CMD::READ_IO: {
      quint16 dev; quint8 module, state;
      ds >> dev >> module >> state;
      if (!isValidDev(dev))
        return {QVariantMap(), BAD_DEV, dev};
      if (!isValidMod(module))
        return { QVariant(), BAD_MOD, module };
      out["dev"]    = DEV::Y;
      out["module"] = module;
      out["state"]  = byteToBits(state);
      return { out };
    }
    case CMD::READ_REG: {
      quint16 dev, addr, value;
      ds >> dev >> addr >> value;
      if (!isValidDev(dev))
        return {QVariantMap(), BAD_DEV, dev};
      out["dev"]   = DEV::Y;
      out["addr"]  = addr;
      out["value"] = value;
      return { out };
    }
    case CMD::WRITE_IO: {
      quint16 dev; quint8 module, state;
      ds >> dev >> module >> state;
      if (!isValidDev(dev))
        return {QVariantMap(), BAD_DEV, dev};
      out["module"] = module;
      out["state"]  = byteToBits(state);
      return { out };
    }
    case CMD::WRITE_REG: {
      quint16 dev, addr, value;
      ds >> dev >> addr >> value;
      if (!isValidDev(dev))
        return {QVariantMap(), BAD_DEV, dev};
      out["dev"]   = DEV::Y;
      out["addr"]  = addr;
      out["value"] = value;
      return { out };
    }
    case CMD::WRITE_RAW: {
      out["value"] = payload.mid(2, paylen-2);
      return { out };
    }
    case CMD::SNAPSHOT: {
      quint8 x1, y1, x2, y2;
      ds >> x1 >> y1 >> x2 >> y2;
      out["x1"] = byteToBits(x1);
      out["y1"] = byteToBits(y1);
      out["x2"] = byteToBits(x2);
      out["y2"] = byteToBits(y2);
      return { out };
    }
    case CMD::SET_VAR: {
      quint8 var, attr;
      ds >> var >> attr;
      if (!isValidVar(var))
        return {QVariantMap(), BAD_VAR, var};
      out["var"]  = var;
      out["attr"] = attr;
    }
    default:
      return { QVariant(), BAD_CMD, cmd };
  }
}

PlcMessageManager::ParseResult PlcMessageManager::parseRespErr(const QByteArray& payload) const
{
  QDataStream ds(payload);
  ds.setByteOrder(QDataStream::BigEndian);
  quint8 cmd, err; quint16 code;
  ds >> cmd >> err >> code;

  QVariantMap out;
  out["cmd"]  = cmd;
  out["err"]  = err;
  out["code"] = code;
  return { out };
}

PlcMessageManager::ParseResult PlcMessageManager::parseStateChange(const QByteArray& payload, quint8 paylen) const
{
  if (paylen != payload.size())
    return { QVariant(), BAD_PLEN, paylen };

  QDataStream ds(payload);
  ds.setByteOrder(QDataStream::BigEndian);

  quint8 chg;
  ds >> chg;

  QVariantMap out;
  out["type"] = Type::CHG;
  out["chg"] = chg;
  switch (chg) {
    case IOs: {
      quint8 x1, y1, x2, y2;
      ds >> x1 >> y1 >> x2 >> y2;
      out["x1"] = byteToBits(x1);
      out["y1"] = byteToBits(y1);
      out["x2"] = byteToBits(x2);
      out["y2"] = byteToBits(y2);
      return { out };
    }
    case AUT_EXT: {
      quint8 state;
      ds >> state;
      out["state"] = bool(state);
      return { out };
    }
    default:
      return { QVariant(), BAD_CHG, chg };
  }
}

bool PlcMessageManager::isValidType(quint8 type) const {
  return type == Type::REQ      ||
         type == Type::RESP_OK  ||
         type == Type::RESP_ERR ||
         type == Type::CHG;
}

bool PlcMessageManager::isValidCmd(quint8 cmd) const {
  return cmd == CMD::READ_IO ||
         cmd == CMD::READ_REG ||
         cmd == CMD::WRITE_IO ||
         cmd == CMD::WRITE_REG ||
         cmd == CMD::WRITE_RAW ||
         cmd == CMD::SNAPSHOT ||
         cmd == CMD::SET_VAR;
}

bool PlcMessageManager::isValidMod(quint8 module) const {
  return module == 1 || module == 2;
}

bool PlcMessageManager::isValidDev(quint16 dev) const {
  return dev == DEV::X ||
         dev == DEV::Y ||
         dev == DEV::D;
}

bool PlcMessageManager::isValidVar(quint8 var) const {
  return var == VAR_TYPE::START_CELL ||
         var == VAR_TYPE::SFY_OK;
}

QVariantList PlcMessageManager::byteToBits(quint8 value) const {
  QVariantList bits; bits.reserve(8);
  for (int i = 0; i < 8; ++i)
    bits.append( ((value >> i) & 0x01) != 0 );
  return bits;
}









