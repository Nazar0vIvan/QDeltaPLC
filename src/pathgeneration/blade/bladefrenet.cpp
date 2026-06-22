#include "bladefrenet.h"

#include <cmath>

#include "geometry/utils.h"

namespace {

bool isValidCxCvInput(const QVector<V3d>& cx, const QVector<V3d>& cxNext, double length)
{
  return cx.size() >= 3 &&
         cxNext.size() == cx.size() &&
         std::isfinite(length);
}

} // namespace

std::optional<Pose> getCxCvStartFrenet(const QVector<V3d>& cx, double length, const Pose& frenet)
{
  if (cx.size() < 2 || !std::isfinite(length)) {
    return std::nullopt;
  }

  const auto direction = normalize(cx[0] - cx[1]);
  if (!direction) return std::nullopt;

  const V3d point = cx[0] + length * *direction;

  return Pose::fromAxes(frenet.t(), frenet.b(), frenet.n(), point);
}

std::optional<Pose> getCxCvEndFrenet(const QVector<V3d>& cx, double length, const Pose& frenet)
{
  if (cx.size() < 2 || !std::isfinite(length)) {
    return std::nullopt;
  }

  const int n = cx.size();

  const auto direction = normalize(cx[n - 1] - cx[n - 2]);
  if (!direction) return std::nullopt;

  const V3d point = cx[n - 1] + length * *direction;

  return Pose::fromAxes(frenet.t(), frenet.b(), frenet.n(), point);
}

std::optional<Pose> getCxCvFrenet(const V3d& point, const V3d& coeffs, const V3d& v0)
{
  const auto tangent = normalize(deriv2d(point, coeffs));
  if (!tangent) return std::nullopt;

  const auto radial = normalize(v0 - point);
  if (!radial) return std::nullopt;

  const auto normal = normalize(tangent->cross(*radial));
  if (!normal) return std::nullopt;

  const auto binormal = normalize(normal->cross(*tangent));
  if (!binormal) return std::nullopt;

  return Pose::fromAxes(*tangent, *binormal, *normal, point);
}

std::optional<QVector<Pose>> getCxCvFrenets(const QVector<V3d>& cx, const QVector<V3d>& cxNext, double length)
{
  if (!isValidCxCvInput(cx, cxNext, length)) {
    return std::nullopt;
  }

  const int n = cx.size();

  QVector<Pose> frenets;
  frenets.reserve(n + 2);

  const auto firstCoeffs = polyfit2d(cx[0], cx[1], cx[2]);
  if (!firstCoeffs) return std::nullopt;

  const auto firstFrenet = getCxCvFrenet(cx[0], *firstCoeffs, cxNext[0]);
  if (!firstFrenet) return std::nullopt;

  const auto startFrenet = getCxCvStartFrenet(cx, length, *firstFrenet);
  if (!startFrenet) return std::nullopt;

  frenets.push_back(*startFrenet);
  frenets.push_back(*firstFrenet);

  for (int i = 1; i < n - 1; ++i) {
    const auto coeffs = polyfit2d(cx[i - 1], cx[i], cx[i + 1]);
    if (!coeffs) return std::nullopt;

    const auto frenet = getCxCvFrenet(cx[i], *coeffs, cxNext[i]);
    if (!frenet) return std::nullopt;

    frenets.push_back(*frenet);
  }

  const auto lastCoeffs = polyfit2d(cx[n - 3], cx[n - 2], cx[n - 1]);
  if (!lastCoeffs) return std::nullopt;

  const auto lastFrenet = getCxCvFrenet(cx[n - 1], *lastCoeffs, cxNext[n - 1]);
  if (!lastFrenet) return std::nullopt;

  const auto endFrenet = getCxCvEndFrenet(cx, length, *lastFrenet);
  if (!endFrenet) return std::nullopt;

  frenets.push_back(*lastFrenet);
  frenets.push_back(*endFrenet);

  return frenets;
}