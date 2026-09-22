#pragma once

#include <QObject>
#include <QString>
#include <QUrl>

class SceneModel;

class SceneObject final : public QObject
{
  Q_OBJECT
  Q_PROPERTY(QString objectId READ objectId CONSTANT)
  Q_PROPERTY(QString name READ name NOTIFY nameChanged)
  Q_PROPERTY(Kind kind READ kind CONSTANT)
  Q_PROPERTY(Classification classification READ classification CONSTANT)
  Q_PROPERTY(bool visible READ visible NOTIFY visibleChanged)
  Q_PROPERTY(QUrl sourceUrl READ sourceUrl CONSTANT)
  Q_PROPERTY(QObject* geometry READ geometry CONSTANT)

public:
  enum Kind { Plane, Cylinder, Cone, Edge, ScanPath, MachiningPath };
  Q_ENUM(Kind)
  enum Classification { Unclassified, Rough, Precise };
  Q_ENUM(Classification)

  QString objectId() const { return m_objectId; }
  QString name() const { return m_name; }
  Kind kind() const { return m_kind; }
  Classification classification() const { return m_classification; }
  bool visible() const { return m_visible; }
  QUrl sourceUrl() const { return m_sourceUrl; }
  QObject* geometry() const { return m_geometry; }

signals:
  void nameChanged();
  void visibleChanged();

private:
  friend class SceneModel;
  SceneObject(const QString& objectId, const QString& name, Kind kind,
              Classification classification, const QUrl& sourceUrl, QObject* parent);
  void setName(const QString& name);
  void setVisible(bool visible);

  const QString m_objectId;
  QString m_name;
  const Kind m_kind;
  const Classification m_classification;
  bool m_visible = true;
  const QUrl m_sourceUrl;
  QObject* m_geometry = nullptr;
};
