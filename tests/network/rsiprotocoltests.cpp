#include "network/rsi/rsiprotocol.h"

#include <QCoreApplication>
#include <QDebug>
#include <QFile>

#include <stdexcept>

namespace {

void require(bool condition, const char* message)
{
  if (!condition) throw std::runtime_error(message);
}

void protocolChecks()
{
  QFile fixture(QStringLiteral(ROBOCRAP_SOURCE_DIR "/resources/files/fromKRC.xml"));
  require(fixture.open(QIODevice::ReadOnly), "Cannot read fromKRC.xml");
  const auto example = RsiProtocol::decode(fixture.readAll());
  require(example && example->ipoc == 123645634563ULL && !example->pose,
          "Example without RIst must be accepted");
  const auto pose = RsiProtocol::decode(
      "<Rob><RIst X='1' Y='2' Z='3' A='4' B='5' C='6'/><IPOC>00042</IPOC></Rob>");
  require(pose && pose->pose && pose->ipoc == 42
              && *pose->pose == std::array<double, 6>{1, 2, 3, 4, 5, 6},
          "Cartesian order or numeric IPOC changed");
  require(RsiProtocol::decode("<Rob><IPOC>0</IPOC></Rob>").has_value(), "Zero IPOC rejected");
  require(RsiProtocol::decode("<Rob><IPOC>18446744073709551615</IPOC></Rob>").has_value(),
          "Maximum quint64 IPOC rejected");
  require(RsiProtocol::decode("<Rob><Extension><Value/></Extension><IPOC>1</IPOC></Rob>").has_value(),
          "Well-formed unconsumed extension rejected");

  const QByteArray invalid[] = {
    "", "<Rob/>", "<Other><IPOC>1</IPOC></Other>",
    "<Rob><IPOC/></Rob>", "<Rob><IPOC>abc</IPOC></Rob>",
    "<Rob><IPOC>-1</IPOC></Rob>", "<Rob><IPOC>1.2</IPOC></Rob>",
    "<Rob><IPOC>18446744073709551616</IPOC></Rob>",
    "<Rob><IPOC>1</IPOC><IPOC>2</IPOC></Rob>",
    "<Rob><IPOC>1</IPOC>", "<Rob><IPOC>1</IPOC></Rob>garbage",
    "<Rob><IPOC>1</IPOC></Rob><Rob/>",
    "<Rob><IPOC>1<X/></IPOC></Rob>",
    "<Rob><IPOC>1</IPOC><Unknown></Rob>",
    "<Rob><RIst X='1'/><IPOC>1</IPOC></Rob>",
    "<Rob><RIst X='nan' Y='2' Z='3' A='4' B='5' C='6'/><IPOC>1</IPOC></Rob>",
    "<Rob><RIst X='1e999' Y='2' Z='3' A='4' B='5' C='6'/><IPOC>1</IPOC></Rob>",
    "<Rob><AIPos A1='bad' A2='2' A3='3' A4='4' A5='5' A6='6'/><IPOC>1</IPOC></Rob>",
    "<Rob><MACur A1='1' A2='2' A3='3' A4='4' A5='5'/><IPOC>1</IPOC></Rob>"
  };
  for (const auto& packet : invalid) {
    require(!RsiProtocol::decode(packet), packet.constData());
    quint16 port = 0;
    bool advanced = false;
    const QHostAddress peer(QStringLiteral("127.0.0.1"));
    require(!RsiProtocol::replyForDatagram(packet, peer, port, peer, 5000,
        [&](quint64) { advanced = true; return RsiTxFrame{}; }),
        "Invalid packet produced a reply");
    require(!advanced && port == 0, "Invalid packet advanced motion or established peer");
  }

  RsiTxFrame frame;
  frame.ipoc = 42;
  frame.corr = {1, -2, 3.5, 4, 5, 6};
  const QByteArray expected =
      "<Sen Type=\"ImFree\"><RKorr X=\"1\" Y=\"-2\" Z=\"3.5\" A=\"4\" B=\"5\" C=\"6\"/>"
      "<IPOC>42</IPOC></Sen>";
  require(RsiProtocol::encode(frame) == expected, "Reply wire format changed");
  frame.shouldStop = true;
  require(RsiProtocol::encode(frame) == expected, "Unexpected new stop wire field");
}

void receiveGateChecks()
{
  const QHostAddress configured(QStringLiteral("127.0.0.1"));
  const QHostAddress foreign(QStringLiteral("127.0.0.2"));
  const QByteArray valid = "<Rob><IPOC>7</IPOC></Rob>";
  quint16 learnedPort = 0;
  const std::array<double, 3> offsets{10, 20, 30};
  std::size_t offsetIndex = 0;
  // This is the same gate used by RsiDevice::onReadyRead around makeTxFrame/tickMotion.
  auto receive = [&](const QByteArray& data, const QHostAddress& sender, quint16 port) {
    return RsiProtocol::replyForDatagram(data, configured, learnedPort, sender, port,
        [&](quint64 ipoc) {
          require(offsetIndex < offsets.size(), "Too many motion callbacks");
          RsiTxFrame frame;
          frame.ipoc = ipoc;
          frame.corr[0] = offsets[offsetIndex++];
          return frame;
        });
  };
  require(!receive(valid, foreign, 5000), "Foreign first sender accepted");
  require(!receive(valid, configured, 0), "Zero sender port accepted");
  require(!receive("<Rob><IPOC>7</IPOC>", configured, 5001), "Truncated first packet accepted");
  require(offsetIndex == 0 && learnedPort == 0, "Rejected packet consumed offset or learned peer");
  const auto first = receive(valid, configured, 5002);
  require(first && first->contains("X=\"10\"") && first->contains("<IPOC>7</IPOC>"),
          "First accepted packet skipped first offset");
  require(offsetIndex == 1 && learnedPort == 5002, "Valid peer was not learned");
  require(!receive(valid, configured, 5003), "Changed peer port accepted");
  require(!receive(valid, foreign, 5002), "Foreign established sender accepted");
  require(!receive("<Rob/>", configured, 5002), "Missing IPOC accepted");
  require(offsetIndex == 1 && learnedPort == 5002, "Rejected packet mutated established state");
  const auto second = receive(valid, configured, 5002);
  require(second && second->contains("X=\"20\"") && offsetIndex == 2,
          "Rejected packets consumed offsets or duplicate IPOC was newly prohibited");
  learnedPort = 0; // Same reset performed by connect/disconnect.
  require(receive(valid, configured, 5004).has_value() && learnedPort == 5004,
          "Peer port could not be relearned after reset");
}

} // namespace

int main(int argc, char** argv)
{
  QCoreApplication app(argc, argv);
  try {
    protocolChecks();
    receiveGateChecks();
    qInfo() << "RSI protocol regressions passed";
    return 0;
  } catch (const std::exception& error) {
    qCritical() << error.what();
    return 1;
  }
}
