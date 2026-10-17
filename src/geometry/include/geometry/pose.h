#pragma once

#include <QVector>

#include <optional>

#include "mathtypes.h"

class Pose
{
public:
  Pose() = default;

  static std::optional<Pose> fromFrame(const V6d& frame);
  static std::optional<Pose> fromTransform(const M4d& tf);
  static std::optional<Pose> fromAxes(const V3d& t, const V3d& b, const V3d& n, const V3d& origin);
  static std::optional<Pose> fromRotAndOrigin(const M3d& rot, const V3d& origin);

  const V6d& frame() const noexcept;
  const M4d& transform() const noexcept;
  M3d rot() const noexcept;

  const V3d& t() const noexcept;
  const V3d& b() const noexcept;
  const V3d& n() const noexcept;
  const V3d& origin() const noexcept;

private:
  V6d m_frame{V6d::Zero()};
  M4d m_tf{M4d::Identity()};

  V3d m_t{V3d::UnitX()};
  V3d m_b{V3d::UnitY()};
  V3d m_n{V3d::UnitZ()};
  V3d m_origin{V3d::Zero()};
};

QVector<V6d> poses2Frames(const QVector<Pose>& poses);
QVector<M4d> poses2Transforms(const QVector<Pose>& poses);
