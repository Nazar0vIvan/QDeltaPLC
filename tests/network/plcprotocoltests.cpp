#include "network/plc/plcdevice.h"

#include <QCoreApplication>
#include <QDebug>
#include <stdexcept>

namespace {
using M = PlcMessageManager;

void require(bool ok, const char* message)
{
  if (!ok) throw std::runtime_error(message);
}

QByteArray frame(quint8 type, quint8 tid, const QByteArray& payload)
{
  QByteArray result;
  QDataStream stream(&result, QIODevice::WriteOnly);
  stream.setByteOrder(QDataStream::BigEndian);
  stream << quint16(0xAA55) << quint8(1) << type << tid << quint8(payload.size());
  result.append(payload);
  result.resize(M::RESP_SIZE, '\0');
  return result;
}

QByteArray ioPayload(quint8 cmd, quint16 dev, quint8 module)
{
  QByteArray result;
  QDataStream stream(&result, QIODevice::WriteOnly);
  stream.setByteOrder(QDataStream::BigEndian);
  stream << cmd << quint8(0) << dev << module << quint8(0x55);
  return result;
}
}

// Narrow friend access exercises actual device buffering/matching without networking.
class PlcProtocolTests
{
public:
  static void run()
  {
    M manager;
    for (auto dev : {M::X, M::Y, M::D}) {
      for (auto cmd : {M::READ_IO, M::WRITE_IO}) {
        const auto parsed = manager.parseMessage(frame(M::RESP_OK, 2, ioPayload(cmd, dev, 1)));
        require(parsed.ok() && parsed.data.toMap().value("dev").toUInt() == uint(dev),
                "IO device identity was lost");
      }
      for (auto cmd : {M::READ_REG, M::WRITE_REG}) {
        QByteArray payload;
        QDataStream stream(&payload, QIODevice::WriteOnly);
        stream.setByteOrder(QDataStream::BigEndian);
        stream << quint8(cmd) << quint8(0) << quint16(dev) << quint16(123) << quint16(456);
        const auto parsed = manager.parseMessage(frame(M::RESP_OK, 2, payload));
        require(parsed.ok() && parsed.data.toMap().value("dev").toUInt() == uint(dev),
                "Register device identity was lost");
      }
    }

    int publications = 0;
    QVariantHash published;
    PlcDevice device(QStringLiteral("offline-plc"));
    device.start(); // Creates an unconnected socket only; never calls connect().
    struct Stop { PlcDevice& device; ~Stop() { device.stop(); } } stop{device};
    const QVariantMap request{{"cmd", M::WRITE_IO}, {"module", 1}, {"andMask", 255}, {"orMask", 1}};
    require(manager.buildReq(request, 2).data.toByteArray()
                == QByteArray::fromHex("aa5501a00208b50000590001ff01"), "Request framing changed");
    require(PlcDevice::swapBytes(QByteArray::fromHex("123456")) == QByteArray::fromHex("341256"),
            "Word byte swapping changed");

    for (int i = 0; i < 256; ++i) {
      const auto tid = device.availableTid();
      require(tid.has_value() && !device.m_pend.contains(*tid), "Outstanding id reused");
      device.m_nextTid = *tid;
      device.m_pend.insert(*tid, request);
    }
    require(!device.availableTid(), "Transaction exhaustion was not rejected");
    device.writeMessage(request);
    require(device.m_pend.size() == 256, "Exhaustion altered pending requests");
    device.m_pend.remove(255);
    require(device.availableTid() == std::optional<quint8>(255), "Allocator missed free id after wrap");
    device.m_pend.clear();
    device.m_nextTid = 255;
    require(device.availableTid() == std::optional<quint8>(0), "Transaction wrap changed");

    // Failed unconnected socket write must remove its pending entry.
    device.writeMessage(request);
    require(device.m_pend.isEmpty(), "Failed send left a pending request");

    QObject::connect(&device, &AbstractDevice::stateReady, &device,
                     [&](const QVariantHash& data) { ++publications; published = data; });
    device.m_pend.insert(7, request);
    auto feed = [&](quint8 tid, quint8 cmd, quint16 dev, quint8 module) {
      device.processIncoming(PlcDevice::swapBytes(frame(M::RESP_OK, tid, ioPayload(cmd, dev, module))));
    };
    feed(8, M::WRITE_IO, M::Y, 1);
    feed(7, M::READ_IO, M::Y, 1);
    feed(7, M::WRITE_IO, M::X, 1);
    feed(7, M::WRITE_IO, M::Y, 2);
    QByteArray malformed = frame(M::RESP_OK, 7, ioPayload(M::WRITE_IO, M::Y, 1));
    malformed[0] = 0;
    device.processIncoming(PlcDevice::swapBytes(malformed));
    require(publications == 0 && device.m_pend.contains(7), "Unrelated reply consumed request or published");
    const QByteArray valid = PlcDevice::swapBytes(frame(M::RESP_OK, 7, ioPayload(M::WRITE_IO, M::Y, 1)));
    for (qsizetype i = 0; i < valid.size() - 1; ++i) {
      device.processIncoming(valid.mid(i, 1));
      require(publications == 0 && device.m_pend.contains(7), "Partial frame was processed");
    }
    device.processIncoming(valid.right(1));
    require(publications == 1 && published.contains("y1") && device.m_pend.isEmpty(),
            "Complete matching frame was not published");
    device.processIncoming(valid);
    require(publications == 1, "Duplicate completed response published");

    device.m_pend.insert(9, {{"cmd", M::READ_IO}, {"dev", M::X}, {"module", 2}});
    feed(9, M::READ_IO, M::Y, 2);
    require(device.m_pend.contains(9), "Wrong read device matched");
    feed(9, M::READ_IO, M::X, 2);
    require(!device.m_pend.contains(9) && publications == 1,
            "Matching read rejected or unsupported publication added");

    for (auto cmd : {M::READ_REG, M::WRITE_REG}) {
      device.m_pend.insert(10, {{"cmd", cmd}, {"addr", 123}, {"value", 456}});
      QVariantMap reply{{"type", M::RESP_OK}, {"tid", 10}, {"cmd", cmd}, {"dev", M::D}, {"addr", 124}, {"value", 456}};
      require(!device.matchResponse(reply), "Wrong register address matched");
      reply["addr"] = 123;
      reply["dev"] = M::Y;
      require(!device.matchResponse(reply), "Wrong register device matched");
      reply["dev"] = M::D;
      if (cmd == M::WRITE_REG) {
        reply["value"] = 457;
        require(!device.matchResponse(reply), "Wrong write value matched");
        reply["value"] = 456;
      }
      require(device.matchResponse(reply), "Matching register reply rejected");
    }
    device.m_pend.insert(11, {{"cmd", M::SET_VAR}, {"var", M::SFY_OK}, {"attr", 1}});
    QVariantMap variable{{"type", M::RESP_OK}, {"tid", 11}, {"cmd", M::SET_VAR}, {"var", M::START_CELL}, {"attr", 1}};
    require(!device.matchResponse(variable), "Wrong variable matched");
    variable["var"] = M::SFY_OK;
    variable["attr"] = 0;
    require(!device.matchResponse(variable), "Wrong variable attribute matched");
    variable["attr"] = 1;
    require(device.matchResponse(variable), "Matching variable reply rejected");

    device.m_pend.insert(12, request);
    device.processIncoming(PlcDevice::swapBytes(frame(M::RESP_ERR, 12, QByteArray::fromHex("b0e70000"))));
    require(device.m_pend.contains(12), "Wrong error command consumed request");
    device.processIncoming(PlcDevice::swapBytes(frame(M::RESP_ERR, 12, QByteArray::fromHex("b5e70000"))));
    require(!device.m_pend.contains(12) && publications == 1, "Matching error reply published or stayed pending");

    // Coalesced unsolicited changes still work without pending requests.
    const auto change = PlcDevice::swapBytes(frame(M::CHG, 0, QByteArray::fromHex("d001020304")));
    device.processIncoming(change + change);
    require(publications == 3, "Coalesced change notifications were lost");
    device.m_pend.insert(13, request);
    device.processIncoming(valid.left(9));
    device.disconnect();
    require(device.m_rx.isEmpty() && device.m_pend.isEmpty(), "Explicit disconnect retained transaction data");
    device.m_pend.insert(14, request);
    device.m_rx = valid.left(3);
    device.onStateChanged(QAbstractSocket::UnconnectedState);
    require(device.m_rx.isEmpty() && device.m_pend.isEmpty(), "Remote disconnect retained transaction data");
  }
};

int main(int argc, char** argv)
{
  QCoreApplication app(argc, argv);
  try {
    PlcProtocolTests::run();
    qInfo() << "PLC protocol regressions passed";
    return 0;
  } catch (const std::exception& error) {
    qCritical() << error.what();
    return 1;
  }
}
