#include "plcmessagemanager.h"

#include <cmath>
#include <limits>
#include <optional>

namespace {

template<typename T>
std::optional<T> readUInt(const QVariantMap& req, const QString& key)
{
  const auto it = req.constFind(key);
  if (it == req.cend() || it->isNull()) return std::nullopt;

  const QMetaType type = it->metaType();
  const int id = type.id();

  const bool isNum =
     type.flags().testFlag(QMetaType::IsEnumeration)
     || id == QMetaType::Char
     || id == QMetaType::SChar
     || id == QMetaType::UChar
     || id == QMetaType::Short
     || id == QMetaType::UShort
     || id == QMetaType::Int
     || id == QMetaType::UInt
     || id == QMetaType::Long
     || id == QMetaType::ULong
     || id == QMetaType::LongLong
     || id == QMetaType::ULongLong
     || id == QMetaType::Float
     || id == QMetaType::Double;

  if (!isNum) return std::nullopt;

  bool ok = false;
  const double val = it->toDouble(&ok);

  if (!ok || !std::isfinite(val))
    return std::nullopt;
  if (val < 0.0 || std::trunc(val) != val)
    return std::nullopt;
  if (val > static_cast<double>(std::numeric_limits<T>::max()))
    return std::nullopt;

  return static_cast<T>(val);
}

std::optional<QByteArray> readBytes(const QVariantMap& req, const QString& key)
{
  const auto it = req.constFind(key);
  if (it == req.cend() || it->isNull()) return std::nullopt;
  if (!it->canConvert<QByteArray>()) return std::nullopt;

  return it->toByteArray();
}

} // namespace

PlcMessageManager::PlcMessageManager(QObject* parent) : QObject(parent) {}

// PUBLIC

PlcMessageManager::ParseResult PlcMessageManager::buildReq(const QVariantMap& req, quint8 tid) const {
  const ParseResult payloadRes = buildReqPayload(req);
  if (!payloadRes.ok()) {
    return payloadRes;
  }
  const QByteArray payload = payloadRes.data.toByteArray();
  const QByteArray header = buildHeader(Type::REQ, tid, static_cast<quint8>(payload.size()));
  return { header + payload };
}

PlcMessageManager::ParseResult PlcMessageManager::parseMessage(const QByteArray& resp) const
{
  if (resp.size() < HEADER_SIZE)
    return { QVariantMap(), BAD_MLEN, resp.size() };

	ParseResult hdrRes = parseHeader(resp.left(HEADER_SIZE));
  if (!hdrRes.ok())
    return hdrRes;

  Header header = hdrRes.data.value<Header>();

  if (resp.size() != RESP_SIZE)
    return { QVariantMap(), BAD_PLEN, resp.size()};

	QByteArray payload = resp.mid(HEADER_SIZE, header.len);
	switch (header.type) {
		case Type::RESP_OK:
			return parseRespOk(payload, header.tid, header.len);
		case Type::RESP_ERR:
			return parseRespErr(payload, header.tid);
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

  const auto cmd = readUInt<quint8>(req, "cmd");
  if (!cmd || !isValidCmd(*cmd))
    return {QVariant(), BAD_CMD, req.value("cmd")};

  constexpr quint8 flags = 0x00;
  ds << *cmd << flags;

  switch (*cmd) {
    case CMD::READ_IO: {
      const auto dev = readUInt<quint16>(req, "dev");
      if (!dev || !isValidDev(*dev))
        return {QVariant(), BAD_DEV, req.value("dev")};

      const auto module = readUInt<quint8>(req, "module");
      if (!module || !isValidMod(*module))
        return {QVariant(), BAD_MOD, req.value("module")};

      ds << *dev << quint8() << *module;
      break;
    }
    case CMD::READ_REG: {
      const auto addr = readUInt<quint16>(req, "addr");
      if (!addr)
        return {QVariant(), BAD_ADDR, req.value("addr")};

      ds << static_cast<quint16>(DEV::D) << *addr;
      break;
    }
    case CMD::WRITE_IO: {
      const auto module = readUInt<quint8>(req, "module");
      if (!module || !isValidMod(*module))
        return {QVariant(), BAD_MOD, req.value("module")};

      const auto andMask = readUInt<quint8>(req, "andMask");
      if (!andMask)
        return {QVariant(), BAD_AND, req.value("andMask")};

      const auto orMask = readUInt<quint8>(req, "orMask");
      if (!orMask)
        return {QVariant(), BAD_OR, req.value("orMask")};

      ds << static_cast<quint16>(DEV::Y)
         << quint8()
         << *module
         << *andMask
         << *orMask;
      break;
    }
    case CMD::WRITE_REG: {
      const auto addr = readUInt<quint16>(req, "addr");
      if (!addr)
        return {QVariant(), BAD_ADDR, req.value("addr")};

      const auto value = readUInt<quint16>(req, "value");
      if (!value)
        return {QVariant(), BAD_DATA, req.value("value")};

      ds << static_cast<quint16>(DEV::D)
         << *addr
         << *value;
      break;
    }
    case CMD::WRITE_RAW: {
      const auto raw = readBytes(req, "raw");
      if (!raw)
        return {QVariant(), BAD_RAW, req.value("raw")};

      ds.writeRawData(raw->constData(), raw->size());
      break;
    }
    case CMD::SNAPSHOT: {
      break;
    }
    case CMD::SET_VAR: {
      const auto var = readUInt<quint8>(req, "var");
      if (!var || !isValidVar(*var))
        return {QVariant(), BAD_VAR, req.value("var")};

      const auto attr = readUInt<quint8>(req, "attr");
      if (!attr)
        return {QVariant(), BAD_ATTR, req.value("attr")};

      ds << *var << *attr;
			break;
    }
		default: break;
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

PlcMessageManager::ParseResult PlcMessageManager::parseHeader(const QByteArray& headerBytesIn) const
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
			return { out };
    }
    default:
      return { QVariant(), BAD_CMD, cmd };
  }
}

PlcMessageManager::ParseResult PlcMessageManager::parseRespErr(const QByteArray& payload, quint8 tid) const
{
	QDataStream ds(payload);
	ds.setByteOrder(QDataStream::BigEndian);
	quint8 cmd, err; quint16 code;
	ds >> cmd >> err >> code;

	QVariantMap out;
	out["type"] = Type::RESP_ERR;
	out["tid"]  = tid;
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
    case CELL_STATE: {
      quint8 state;
      ds >> state;
			out["cellState"] = state;
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









