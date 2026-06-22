#include "socketstateutils.h"

#include <QMetaEnum>

namespace {

template <typename Enum>
QString enumKey(Enum value)
{
  const QMetaEnum metaEnum = QMetaEnum::fromType<Enum>();
  const char* key = metaEnum.valueToKey(static_cast<int>(value));

  if (!key) {
    return QStringLiteral("Unknown(%1)").arg(static_cast<int>(value));
  }

  return QString::fromLatin1(key);
}

} // namespace

QString socketStateName(QAbstractSocket::SocketState state)
{
  return enumKey(state);
}

QString socketErrorName(QAbstractSocket::SocketError error)
{
  return enumKey(error);
}
