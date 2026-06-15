#include "pose.h"

Pose Pose::fromFrame(const V6d& frame)
{
  Pose pose;

  const V3d origin{frame(0), frame(1), frame(2)};
  const M3d rot = euler2rot(frame(3), frame(4), frame(5));

  pose.setFromRotation(rot, origin);
  pose.m_frame = frame;

  return pose;
}

std::optional<Pose> Pose::fromTransform(const M4d& tf)
{
  const M3d rawRot = tf.block<3, 3>(0, 0);

  const auto rot = vec2rot(rawRot.col(0), rawRot.col(1), rawRot.col(2));

  if (!rot) return std::nullopt;

  Pose pose;
  pose.setFromRotation(*rot, tf.block<3, 1>(0, 3));

  return pose;
}

std::optional<Pose> Pose::fromAxes(const V3d& t, const V3d& b, const V3d& n, const V3d& origin)
{
  const auto rot = vec2rot(t, b, n);

  if (!rot) return std::nullopt;

  Pose pose;
  pose.setFromRotation(*rot, origin);

  return pose;
}

const V6d& Pose::frame() const noexcept
{
  return m_frame;
}

const M4d& Pose::tf() const noexcept
{
  return m_tf;
}

M3d Pose::rot() const noexcept
{
  return m_tf.block<3, 3>(0, 0);
}

const V3d& Pose::t() const noexcept
{
  return m_t;
}

const V3d& Pose::b() const noexcept
{
  return m_b;
}

const V3d& Pose::n() const noexcept
{
  return m_n;
}

const V3d& Pose::origin() const noexcept
{
  return m_origin;
}

void Pose::setFromRotation(const M3d& rot, const V3d& origin)
{
  m_origin = origin;

  m_t = rot.col(0);
  m_b = rot.col(1);
  m_n = rot.col(2);

  m_tf = transform(rot, origin);

  const EulerSolution euler = rot2euler(rot);

  m_frame << origin.x(),
             origin.y(),
             origin.z(),
             euler.A1,
             euler.B1,
             euler.C1;
}

QVector<V6d> poses2Frames(const QVector<Pose>& poses)
{
  QVector<V6d> frames;
  frames.reserve(poses.size());

  for (const Pose& pose : poses) {
    frames.push_back(pose.frame());
  }

  return frames;
}

QVector<M4d> poses2Transforms(const QVector<Pose>& poses)
{
  QVector<M4d> transforms;
  transforms.reserve(poses.size());

  for (const Pose& pose : poses) {
    transforms.push_back(pose.tf());
  }

  return transforms;
}




