#pragma once

#include <QByteArray>
#include <QHostAddress>

#include <array>
#include <optional>

struct RsiTxFrame {
  quint64 ipoc = 0;
  bool shouldStop = false;
  std::array<double, 6> corr{};
};

namespace RsiProtocol {

struct Response {
  quint64 ipoc = 0;
  std::optional<std::array<double, 6>> pose;
};

std::optional<Response> decode(const QByteArray& data);
QByteArray encode(const RsiTxFrame& frame);
bool acceptsSender(const QHostAddress& configuredAddress, quint16 learnedPort,
                   const QHostAddress& senderAddress, quint16 senderPort);

// Shared receive gate: the motion callback is invoked only for a fully validated
// packet. A zero learned port means this connection has not accepted a peer yet.
template<typename NextFrame>
std::optional<QByteArray> replyForDatagram(
    const QByteArray& data, const QHostAddress& configuredAddress, quint16& learnedPort,
    const QHostAddress& senderAddress, quint16 senderPort, NextFrame nextFrame)
{
  if (!acceptsSender(configuredAddress, learnedPort, senderAddress, senderPort))
    return std::nullopt;
  const auto response = decode(data);
  if (!response) return std::nullopt;
  learnedPort = senderPort;
  return encode(nextFrame(response->ipoc));
}

} // namespace RsiProtocol
