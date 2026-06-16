#include "pose.h"
#include "utils.h"

std::optional<Pose> Pose::fromFrame(const V6d& frame)
{
  if (!frame.allFinite()) return std::nullopt;

  const V3d origin{frame(0), frame(1), frame(2)};
  const M3d rot = euler2rot(frame(3), frame(4), frame(5));

  auto pose = fromRotAndOrigin(rot, origin);

  if (!pose) return std::nullopt;

  pose->m_frame = frame;
  return pose;
}

std::optional<Pose> Pose::fromTransform(const M4d& tf)
{
  if (!tf.allFinite()) return std::nullopt;

  if (!nearlyEqual(tf(3, 0), 0.0) || !nearlyEqual(tf(3, 1), 0.0) ||
      !nearlyEqual(tf(3, 2), 0.0) || !nearlyEqual(tf(3, 3), 1.0)) {
    return std::nullopt;
  }

  const M3d rot = tf.block<3, 3>(0, 0);
  const V3d origin = tf.block<3, 1>(0, 3);

  return fromRotAndOrigin(rot, origin);
}
std::optional<Pose> Pose::fromAxes(const V3d& t, const V3d& b, const V3d& n, const V3d& origin)
{
  const auto basis = vecs2basis(t, b, n);

  if (!basis) return std::nullopt;

  const M3d rot = basis2rot(*basis);

  return fromRotAndOrigin(rot, origin);
}

std::optional<Pose> Pose::fromRotAndOrigin(const M3d &rot, const V3d &origin)
{
  if (!rot.allFinite() || !origin.allFinite()) {
    return std::nullopt;
  }

  if (!isBasis(rot.col(0), rot.col(1), rot.col(2))) {
    return std::nullopt;
  }

  Pose pose;

  pose.m_origin = origin;
  pose.m_t = rot.col(0);
  pose.m_b = rot.col(1);
  pose.m_n = rot.col(2);
  pose.m_tf = makeTransform(rot, origin);

  const EulerSolution euler = rot2euler(rot);

  pose.m_frame << origin.x(), origin.y(), origin.z(),
                  euler.A1, euler.B1, euler.C1;

  return pose;
}

const V6d& Pose::frame() const noexcept
{
  return m_frame;
}

const M4d& Pose::transform() const noexcept
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
    transforms.push_back(pose.transform());
  }

  return transforms;
}




