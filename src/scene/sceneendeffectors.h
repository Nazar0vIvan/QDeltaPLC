#pragma once

#include "geometry/mathtypes.h"

#include <QObject>
#include <QList>
#include <QString>
#include <QUrl>

#include <optional>

// Tool configuration only; CAD shapes and flange attachment belong to the viewport.
class SceneEndEffectors : public QObject
{
  Q_OBJECT
  Q_PROPERTY(QUrl measuringCadSource READ measuringCadSource NOTIFY measuringCadSourceChanged)
  Q_PROPERTY(QUrl spindleCadSource READ spindleCadSource NOTIFY spindleCadSourceChanged)
  Q_PROPERTY(QString measuringName READ measuringName CONSTANT)
  Q_PROPERTY(double rubyBallDiameter READ rubyBallDiameter CONSTANT)
  Q_PROPERTY(double stylusLength READ stylusLength CONSTANT)
  Q_PROPERTY(QList<double> spindleTcp READ spindleTcp NOTIFY spindleTcpChanged)
  Q_PROPERTY(QList<double> spindleTcpInCollet READ spindleTcpInCollet NOTIFY spindleTcpChanged)
  Q_PROPERTY(QList<double> spindleColletPose READ spindleColletPose NOTIFY spindleColletChanged)
  Q_PROPERTY(Tool activeTool READ activeTool NOTIFY activeToolChanged)

public:
  enum Tool { Measuring, Spindle };
  Q_ENUM(Tool)

  explicit SceneEndEffectors(QObject* parent = nullptr);

  const QUrl& measuringCadSource() const { return m_measuringCadSource; }
  const QUrl& spindleCadSource() const { return m_spindleCadSource; }
  QString measuringName() const { return QStringLiteral("WP-500 V6"); }
  double rubyBallDiameter() const { return 3.0; } // mm
  double stylusLength() const { return 37.2; } // mm
  // Flange-relative X/Y/Z in mm, A/B/C in degrees (Z-Y-X composition).
  // Empty until explicitly configured; the screenshot is not calibration data.
  const QList<double>& spindleTcp() const { return m_spindleTcp; }
  // ER-relative XYZABC in the same mm/degrees convention as spindleTcp.
  QList<double> spindleTcpInCollet() const;
  // Flange-relative ER XYZABC and frame, initialized from the hardcoded collet calibration.
  QList<double> spindleColletPose() const;
  const std::optional<M4d>& spindleColletFrame() const { return m_spindleColletFrame; }
  Tool activeTool() const { return m_activeTool; }

  Q_INVOKABLE bool setMeasuringCadSource(const QUrl& source);
  Q_INVOKABLE bool setSpindleCadSource(const QUrl& source);
  Q_INVOKABLE bool setSpindleTcp(const QList<double>& pose);
  Q_INVOKABLE bool setSpindleTcpInCollet(const QList<double>& pose);
  // Commit flange-relative ER and ER-relative TCP together before notifying observers.
  Q_INVOKABLE bool applySpindlePoses(const QList<double>& colletPose, const QList<double>& tcpInCollet);
  Q_INVOKABLE bool setActiveTool(Tool tool);
  // Robot-base XYZABC poses; empty results mean missing calibration or invalid input.
  QList<double> spindlePoseFromFlange(const QList<double>& flange) const;
  QList<double> flangePoseFromSpindle(const QList<double>& tcp) const;

signals:
  void measuringCadSourceChanged();
  void spindleCadSourceChanged();
  void spindleTcpChanged();
  void spindleColletChanged();
  void activeToolChanged();

private:
  QUrl m_measuringCadSource;
  QUrl m_spindleCadSource;
  QList<double> m_spindleTcp;
  std::optional<M4d> m_spindleColletFrame;
  Tool m_activeTool = Measuring;
};
