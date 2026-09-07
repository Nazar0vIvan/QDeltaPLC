#pragma once

#include <QHash>
#include <QObject>
#include <QString>
#include <QStringList>

class AbstractSocketRunner;

// The only object QML needs in order to reach a device. QML asks for a runner
// by key; adding a device requires no new QML registration and no new context
// property.
//
// DeviceHub is the parent of every runner it holds. QQmlEngine gives an object
// returned from a Q_INVOKABLE JavaScriptOwnership when that object has no
// parent, which would let the QML garbage collector delete the runner.
class DeviceHub : public QObject
{
  Q_OBJECT
  Q_PROPERTY(QStringList keys READ keys CONSTANT)

public:
  explicit DeviceHub(QObject* parent = nullptr);

  // Takes ownership of runner.
  void add(const QString& key, AbstractSocketRunner* runner);

  Q_INVOKABLE AbstractSocketRunner* device(const QString& key) const;

  QStringList keys() const;

public slots:
  void startAll();
  void stopAll();

private:
  QHash<QString, AbstractSocketRunner*> m_devs;
};
