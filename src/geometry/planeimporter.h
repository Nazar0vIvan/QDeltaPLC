#pragma once

#include <QObject>
#include <QUrl>
#include <QVariantList>

class QThread;

class PlaneImporter : public QObject
{
  Q_OBJECT
  Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)

public:
  explicit PlaneImporter(QObject* parent = nullptr);
  ~PlaneImporter() override;

  bool busy() const { return m_worker != nullptr; }
  Q_INVOKABLE void load(const QUrl& sourceUrl);

signals:
  void busyChanged();
  void loaded(const QUrl& sourceUrl, const QString& name, const QVariantList& coefficients);
  void failed(const QString& message);

private:
  QThread* m_worker = nullptr;
};
