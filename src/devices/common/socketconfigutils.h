#pragma once

#include <QVariant>

#include <optional>

// Configured ports share this rule; socket binding and peer discovery do not.
inline std::optional<quint16> parseSocketPort(const QVariant& value)
{
  bool ok = false;
  const uint port = value.toUInt(&ok);
  if (!ok || port == 0 || port > 65535) return std::nullopt;
  return static_cast<quint16>(port);
}
