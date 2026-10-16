#include "trajectories/chamferarclength.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <utility>

namespace {

constexpr int InitialIntervals = 16;
constexpr int MaximumDepth = 20;
constexpr qsizetype MaximumKnots = 200000;
constexpr int MaximumEvaluations = 800000;
using Polynomial = std::array<double, 6>;

struct PolynomialValue
{
  double value = 0.0;
  double first = 0.0;
  double second = 0.0;
};

PolynomialValue evaluatePolynomial(const Polynomial& coefficients, double x)
{
  PolynomialValue result{coefficients.back(), 0.0, 0.0};
  for (int index = 4; index >= 0; --index) {
    result.second = result.second * x + 2.0 * result.first;
    result.first = result.first * x + result.value;
    result.value = result.value * x + coefficients[index];
  }
  return result;
}

double polynomialProgress(const Polynomial& coefficients, double x)
{
  double value = coefficients.back();
  for (int index = 4; index >= 0; --index) value = value * x + coefficients[index];
  return value;
}

// Positive Bernstein coefficients imply positivity over the complete interval.
// Subdivision resolves conservative negative coefficients without root solving.
bool positiveBernstein(const std::array<double, 5>& coefficients, int depth)
{
  const auto extrema = std::minmax_element(coefficients.cbegin(), coefficients.cend());
  if (*extrema.first > 0.0) return true;
  if (*extrema.second <= 0.0 || depth == 12) return false;
  auto work = coefficients;
  std::array<double, 5> left{}, right{};
  left[0] = work[0];
  right[4] = work[4];
  for (int level = 1; level <= 4; ++level) {
    for (int index = 0; index <= 4 - level; ++index)
      work[index] = 0.5 * work[index] + 0.5 * work[index + 1];
    left[level] = work[0];
    right[4 - level] = work[4 - level];
  }
  return positiveBernstein(left, depth + 1) && positiveBernstein(right, depth + 1);
}

bool increasingPolynomial(const Polynomial& coefficients)
{
  for (double value : coefficients)
    if (!std::isfinite(value)) return false;
  const double d0 = coefficients[1];
  const double d1 = 2.0 * coefficients[2];
  const double d2 = 3.0 * coefficients[3];
  const double d3 = 4.0 * coefficients[4];
  const double d4 = 5.0 * coefficients[5];
  const std::array<double, 5> bernstein{d0, d0 + d1 / 4.0,
      d0 + d1 / 2.0 + d2 / 6.0, d0 + 0.75 * d1 + 0.5 * d2 + d3 / 4.0,
      d0 + d1 + d2 + d3 + d4};
  for (double value : bernstein)
    if (!std::isfinite(value)) return false;
  return positiveBernstein(bernstein, 0);
}

struct ArcPoint
{
  double progress = 0.0;
  ChamferArcDifferential differential;
};

// Normalize distance and progress locally to avoid subtracting large coefficients.
Polynomial inversePolynomial(const ArcPoint& first, const ArcPoint& last, double length)
{
  const double width = last.progress - first.progress;
  const double firstScale = length / first.differential.speed;
  const double lastScale = length / last.differential.speed;
  const double d0 = firstScale / width;
  const double d1 = lastScale / width;
  const double q0 = -(first.differential.speedDerivative / first.differential.speed)
      * firstScale * firstScale / width;
  const double q1 = -(last.differential.speedDerivative / last.differential.speed)
      * lastScale * lastScale / width;
  const double a = 1.0 - d0 - 0.5 * q0;
  const double b = d1 - d0 - q0;
  const double c = q1 - q0;
  return {0.0, d0, 0.5 * q0, 10.0 * a - 4.0 * b + 0.5 * c,
          -15.0 * a + 7.0 * b - c, 6.0 * a - 3.0 * b + 0.5 * c};
}

struct Integral
{
  double length = 0.0;
  double error = 0.0;
};

struct ArcInterval
{
  ArcPoint first;
  ArcPoint last;
  Integral integral;
  double length = 0.0; // Stored distance width, including cumulative rounding.
};

struct ArcTableResult
{
  QVector<ChamferArcLengthKnot> knots;
  QVector<Polynomial> polynomials;
  QString error;
  int actualEvaluations = 0;
  int logicalEvaluations = 0;
};

class ArcTableBuilder
{
public:
  // Path and phase identify the subcurve; parameters specify numerical accuracy.
  ArcTableBuilder(const ChamferPath& path, ChamferPhase phase,
                  const ChamferArcLengthParameters& parameters)
    : m_path(path), m_phase(phase), m_parameters(parameters) {}

  ArcTableResult run()
  {
    auto first = point(0.0);
    if (!first) return std::move(m_result);
    m_result.knots.append({0.0, 0.0});
    for (int index = 1; index <= InitialIntervals; ++index) {
      const auto last = point(static_cast<double>(index) / InitialIntervals);
      if (!last || !appendInterval(*first, *last, 0)) return std::move(m_result);
      first = last;
    }
    if (m_result.knots.last().distance <= GeomConst::Eps)
      m_result.error = QStringLiteral("TCP curve length is degenerate.");
    return std::move(m_result);
  }

private:
  bool chargeEvaluations(int count)
  {
    const int remaining = MaximumEvaluations - m_result.logicalEvaluations;
    m_result.logicalEvaluations += std::min(count, remaining);
    if (count <= remaining) return true;
    m_result.error = QStringLiteral("Arc-length differential evaluation limit reached.");
    return false;
  }

  std::optional<ArcPoint> point(double progress)
  {
    if (!chargeEvaluations(1)) return std::nullopt;
    ++m_result.actualEvaluations;
    const auto differential = m_path.phaseArcDifferential(m_phase, progress);
    if (!differential) {
      m_result.error = QStringLiteral("Invalid analytical TCP differential during arc-length calculation.");
      return std::nullopt;
    }
    return ArcPoint{progress, *differential};
  }

  std::optional<Integral> integrate(double start, double end)
  {
    if (start == end) return Integral{};
    // Embedded Gauss-7 / Kronrod-15 rule on [-1,1], positive nodes plus zero.
    static constexpr std::array<double, 8> nodes{
        0.9914553711208126392, 0.9491079123427585245, 0.8648644233597690728,
        0.7415311855993944399, 0.5860872354676911303, 0.4058451513773971669,
        0.2077849550078984676, 0.0};
    static constexpr std::array<double, 8> kronrod{
        0.0229353220105292250, 0.0630920926299785533, 0.1047900103222501838,
        0.1406532597155259187, 0.1690047266392679028, 0.1903505780647854099,
        0.2044329400752988924, 0.2094821410847278280};
    static constexpr std::array<double, 4> gauss{
        0.1294849661688696933, 0.2797053914892766679,
        0.3818300505051189449, 0.4179591836734693878};
    const double half = 0.5 * (end - start);
    const double middle = start + half;
    const auto center = point(middle);
    if (!center) return std::nullopt;
    double fine = kronrod[7] * center->differential.speed;
    double coarse = gauss[3] * center->differential.speed;
    std::array<double, 7> low{}, high{};
    for (std::size_t index = 0; index < 7; ++index) {
      const auto left = point(middle - half * nodes[index]);
      const auto right = point(middle + half * nodes[index]);
      if (!left || !right) return std::nullopt;
      low[index] = left->differential.speed;
      high[index] = right->differential.speed;
      const double sum = low[index] + high[index];
      fine += kronrod[index] * sum;
      if (index % 2 == 1) coarse += gauss[index / 2] * sum;
    }
    const double mean = 0.5 * fine;
    double deviation = kronrod[7] * std::abs(center->differential.speed - mean);
    for (std::size_t index = 0; index < 7; ++index)
      deviation += kronrod[index] * (std::abs(low[index] - mean) + std::abs(high[index] - mean));
    const double length = half * fine;
    deviation *= half;
    double error = std::abs(half * (fine - coarse));
    if (deviation > 0.0 && error > 0.0)
      error = deviation * std::min(1.0, std::pow(200.0 * error / deviation, 1.5));
    error = std::max(error, 50.0 * std::numeric_limits<double>::epsilon() * length);
    if (!std::isfinite(length) || length <= 0.0 || !std::isfinite(error)) {
      m_result.error = QStringLiteral("Arc-length integral exceeds the numerical range.");
      return std::nullopt;
    }
    return Integral{length, error};
  }

  bool accurateInverse(const ArcInterval& interval, const Polynomial& polynomial)
  {
    const auto& first = interval.first;
    const auto& last = interval.last;
    const double length = interval.length;
    const double width = last.progress - first.progress;
    if (!increasingPolynomial(polynomial)) return false;
    for (double x : {0.0, 0.1, 0.25, 0.4, 0.5, 0.6, 0.75, 0.9, 1.0}) {
      const auto value = evaluatePolynomial(polynomial, x);
      // Pin only mathematical endpoints, not failed interior evaluations.
      const double progress = x == 0.0 ? first.progress
          : x == 1.0 ? last.progress : first.progress + width * value.value;
      if (!std::isfinite(progress) || progress < first.progress || progress > last.progress)
        return false;
      std::optional<ArcPoint> actual;
      std::optional<Integral> distance;
      if (x == 0.0 || x == 1.0) {
        if (chargeEvaluations(1)) actual = x == 0.0 ? first : last;
        if (x == 0.0) distance = Integral{};
        else if (chargeEvaluations(15)) distance = interval.integral;
      } else {
        actual = point(progress);
        distance = integrate(first.progress, progress);
      }
      if (!actual || !distance) return false;
      const double derivative = (width / length) * value.first;
      const double second = (width / length) * value.second / length;
      const double speed = actual->differential.speed;
      const double ratio = speed * derivative;
      const double ratioDerivative = actual->differential.speedDerivative * derivative * derivative
          + speed * second;
      // The partial integral is a separate numerical check, not an exact bound.
      if (!std::isfinite(ratio) || !std::isfinite(ratioDerivative)
          || derivative <= 0.0
          || distance->error > m_parameters.lengthTolerance * width
          || std::abs(distance->length - x * length) + distance->error > m_parameters.lookupTolerance
          || std::abs(ratio - 1.0) > m_parameters.relativeSpeedTolerance
          || std::abs(ratioDerivative) > m_parameters.speedDerivativeTolerance)
        return false;
    }
    return true;
  }

  // Endpoints specify the subcurve; depth bounds both integration and inverse refinement.
  bool appendInterval(const ArcPoint& first, const ArcPoint& last, int depth)
  {
    if (m_result.knots.size() >= MaximumKnots) {
      m_result.error = QStringLiteral("Arc-length map size limit reached.");
      return false;
    }
    const double width = last.progress - first.progress;
    const auto integral = integrate(first.progress, last.progress);
    if (!integral) return false;
    const double startDistance = m_result.knots.last().distance;
    const double endDistance = startDistance + integral->length;
    const double length = endDistance - startDistance;
    if (!std::isfinite(endDistance) || length <= 0.0 || width <= 0.0) {
      m_result.error = QStringLiteral("Arc-length interval cannot be represented distinctly.");
      return false;
    }
    // Coefficients use the stored distance width, including cumulative rounding.
    const auto polynomial = inversePolynomial(first, last, length);
    const bool converged = integral->error + std::abs(length - integral->length)
            <= m_parameters.lengthTolerance * width
        && accurateInverse({first, last, *integral, length}, polynomial);
    if (!m_result.error.isEmpty()) return false;
    if (!converged) {
      const double middleProgress = first.progress + 0.5 * width;
      if (depth >= MaximumDepth || middleProgress <= first.progress || middleProgress >= last.progress) {
        m_result.error = QStringLiteral("Quintic arc-length refinement did not reach the requested accuracy.");
        return false;
      }
      const auto middle = point(middleProgress);
      return middle && appendInterval(first, *middle, depth + 1)
          && appendInterval(*middle, last, depth + 1);
    }
    m_result.knots.append({last.progress, endDistance});
    m_result.polynomials.append(polynomial);
    return true;
  }

  const ChamferPath& m_path;
  const ChamferPhase m_phase;
  const ChamferArcLengthParameters& m_parameters;
  ArcTableResult m_result;
};

bool distanceBefore(const ChamferArcLengthKnot& knot, double distance)
{
  return knot.distance < distance;
}

bool positiveFinite(double value)
{
  return std::isfinite(value) && value > 0.0;
}

} // namespace

ChamferArcLength::ChamferArcLength(QVector<ChamferArcLengthKnot> knots,
                                 QVector<std::array<double, 6>> polynomials)
  : m_knots(std::move(knots)), m_polynomials(std::move(polynomials)) {}

ChamferArcLengthResult ChamferArcLength::create(const ChamferPath& path, ChamferPhase phase,
                                              const ChamferArcLengthParameters& parameters)
{
  if (phase != ChamferPhase::LeadIn && phase != ChamferPhase::Machining
      && phase != ChamferPhase::LeadOut)
    return {{}, QStringLiteral("Invalid chamfer phase for arc-length calculation.")};
  if (!positiveFinite(parameters.lengthTolerance) || !positiveFinite(parameters.lookupTolerance)
      || !positiveFinite(parameters.relativeSpeedTolerance)
      || !positiveFinite(parameters.speedDerivativeTolerance))
    return {{}, QStringLiteral("Arc-length tolerances must be finite and positive.")};
  auto result = ArcTableBuilder(path, phase, parameters).run();
  if (!result.error.isEmpty())
    return {{}, QStringLiteral("Cannot measure chamfer phase %1: %2")
                    .arg(static_cast<int>(phase)).arg(result.error),
            result.actualEvaluations, result.logicalEvaluations};
  return {ChamferArcLength(std::move(result.knots), std::move(result.polynomials)), {},
          result.actualEvaluations, result.logicalEvaluations};
}

std::optional<double> ChamferArcLength::progressAtDistance(double distance) const
{
  if (!std::isfinite(distance) || distance < 0.0 || distance > length()) return std::nullopt;
  if (distance == 0.0) return 0.0;
  if (distance == length()) return 1.0;
  const auto upper = std::lower_bound(m_knots.cbegin(), m_knots.cend(), distance, distanceBefore);
  if (upper->distance == distance) return upper->progress;
  const auto& lower = *(upper - 1);
  const auto& polynomial = m_polynomials[upper - m_knots.cbegin() - 1];
  const double fraction = (distance - lower.distance) / (upper->distance - lower.distance);
  const double progress = lower.progress + (upper->progress - lower.progress)
      * polynomialProgress(polynomial, fraction);
  if (!std::isfinite(progress) || progress < lower.progress || progress > upper->progress)
    return std::nullopt;
  return progress;
}
