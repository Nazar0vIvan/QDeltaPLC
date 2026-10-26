#pragma once

#include "geometry/mathtypes.h"

#include <QObject>
#include <QList>
#include <QString>

#include <optional>

// Tool configuration only; CAD shapes and flange attachment belong to the viewport.
class SceneEndEffectors : public QObject
{
  Q_OBJECT
  Q_PROPERTY(QString measuringName READ measuringName CONSTANT)
  Q_PROPERTY(double rubyBallDiameter READ rubyBallDiameter CONSTANT)
  Q_PROPERTY(double stylusLength READ stylusLength CONSTANT)
  Q_PROPERTY(QList<double> measuringTcp READ measuringTcp NOTIFY measuringTcpChanged)
  Q_PROPERTY(QList<double> spindleTcp READ spindleTcp NOTIFY spindleTcpChanged)
  Q_PROPERTY(QList<double> spindleTcpInCollet READ spindleTcpInCollet NOTIFY spindleTcpChanged)
  Q_PROPERTY(QList<double> spindleColletPose READ spindleColletPose NOTIFY spindleColletChanged)
  Q_PROPERTY(Tool activeTool READ activeTool NOTIFY activeToolChanged)

public:
  enum Tool { Measuring, Spindle };
  Q_ENUM(Tool)

  explicit SceneEndEffectors(QObject* parent = nullptr);

  QString measuringName() const { return QStringLiteral("WP-500 V6"); }
  double rubyBallDiameter() const { return 3.0; } // mm
  double stylusLength() const { return 37.2; } // mm
  // Ruby ball centre relative to the flange, XYZABC as spindleTcp; TOOL_DATA[5] until Apply.
  const QList<double>& measuringTcp() const { return m_measuringTcp; }
  const std::optional<M4d>& measuringTcpFrame() const { return m_measuringTcpFrame; }
  // Flange-relative X/Y/Z in mm, A/B/C in degrees (Z-Y-X composition).
  // Empty until explicitly configured; the screenshot values are not a TCP transform.
  const QList<double>& spindleTcp() const { return m_spindleTcp; }
  const std::optional<M4d>& spindleTcpFrame() const { return m_spindleTcpFrame; }
  // ER-relative XYZABC in the same mm/degrees convention as spindleTcp.
  QList<double> spindleTcpInCollet() const;
  // Flange-relative ER XYZABC and frame, initialized from the hardcoded collet transform.
  QList<double> spindleColletPose() const;
  const std::optional<M4d>& spindleColletFrame() const { return m_spindleColletFrame; }
  Tool activeTool() const { return m_activeTool; }

  // Commit flange-relative ER and ER-relative TCP together before notifying observers.
  Q_INVOKABLE bool applySpindlePoses(const QList<double>& colletPose, const QList<double>& tcpInCollet);
  // Rejects anything but six finite values; an unchanged pose emits nothing.
  Q_INVOKABLE bool applyMeasuringTcp(const QList<double>& pose);
  Q_INVOKABLE bool setActiveTool(Tool tool);

signals:
  void spindleTcpChanged();
  void spindleColletChanged();
  void activeToolChanged();
  void measuringTcpChanged();

private:
  bool setSpindleTcp(const QList<double>& pose);

  QList<double> m_spindleTcp;
  std::optional<M4d> m_spindleTcpFrame;
  std::optional<M4d> m_spindleColletFrame;
  // Controller TOOL_DATA[5] from the CMM measurement of the probe in its flange adapter.
  QList<double> m_measuringTcp{-0.048, -0.097, 127.0, 0.0, 0.0, 0.0};
  std::optional<M4d> m_measuringTcpFrame;
  Tool m_activeTool = Measuring;
};
