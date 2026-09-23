#include "3d/robot/robotpreviewstate.h"
#include "3d/math/utils.h"

#include <QCoreApplication>
#include <QDebug>

#include <cmath>
#include <limits>
#include <stdexcept>

namespace {

using namespace RoboCrap3D;

void require(bool condition, const char* message)
{
  if (!condition) throw std::runtime_error(message);
}

void requireTransform(const M4d& actual, const M4d& expected)
{
  const M4d delta = expected.Inverted().Multiplied(actual);
  require(V3d{delta.TranslationPart()}.Magnitude() <= GeomConst::PosEps,
          "Transform position changed");
  for (int row = 1; row <= 3; ++row) {
    for (int col = 1; col <= 3; ++col) {
      require(std::abs(delta.Value(row, col) - (row == col ? 1.0 : 0.0)) <= GeomConst::RotEps,
              "Transform orientation changed");
    }
  }
}

void requireUnchanged(const RobotPose& actual, const RobotPose& committed)
{
  require(actual.joints == committed.joints, "Committed joints changed during calculation");
  require(actual.flange == committed.flange, "Committed flange changed during calculation");
  for (std::size_t i = 0; i < LinkCount; ++i) {
    for (int row = 1; row <= 3; ++row) {
      for (int col = 1; col <= 4; ++col) {
        require(actual.transforms[i].Value(row, col) == committed.transforms[i].Value(row, col),
                "Committed link transform changed during calculation");
      }
    }
  }
}

M4d fromFrame(const V6d& frame)
{
  return makeTransform(euler2rot(frame[3], frame[4], frame[5]),
                       V3d{frame[0], frame[1], frame[2]});
}

void homeAndConventions(const Kr10Model& model)
{
  require(model.qHome == V6d{0, -90, 90, 0, 0, 0}, "Fixture home joints changed");
  require(model.pHome == V6d{890, 0, 1080, 0, 90, 0}, "Fixture home frame changed");
  RobotPreviewState state(model);
  QString error;
  require(state.initialize(error), qPrintable(error));
  require(state.pose().joints == model.qHome, "Initialization did not commit home joints");
  requireTransform(state.pose().transforms.back(), fromFrame(model.pHome));
  requireTransform(fromFrame(state.pose().flange), fromFrame(model.pHome));

  // Explicit noncommuting rotation matrix for Rz(90) * Ry(30) * Rx(90).
  const double c = std::sqrt(3.0) / 2.0;
  const M3d expected{0, 0, 1, c, 0.5, 0, -0.5, c, 0};
  requireTransform(fromFrame(V6d{11, 22, 33, 90, 30, 90}),
                   makeTransform(expected, V3d{11, 22, 33}));

  Kr10Kinematics solver(model);
  require(solver.isValid(), "Fixture has invalid kinematic dimensions");
  require(!solver.solveIK(solver.solveFK(model.qHome).back(), model.qHome),
          "Home wrist singularity must be rejected by IK");
  const RobotPose committed = state.pose();
  require(!state.inverse(model.pHome, error), "Preview accepted singular home IK");
  require(!error.isEmpty(), "Singular IK must report an error");
  requireUnchanged(state.pose(), committed);

  Kr10Model inconsistent = model;
  inconsistent.pHome[0] += 1.0;
  RobotPreviewState invalidHome(inconsistent);
  const RobotPose before = invalidHome.pose();
  error.clear();
  require(!invalidHome.initialize(error), "Inconsistent home fixture was accepted");
  require(!error.isEmpty(), "Invalid home must report an error");
  requireUnchanged(invalidHome.pose(), before);
}

void roundTripsAndBranches(const Kr10Model& model)
{
  struct Case { const char* name; V6d joints; uint8_t status; uint8_t turn; };
  const Case cases[] = {
    {"basic, negative elbow, positive wrist", {20, -60, 60, 20, 40, 30}, 2, 2},
    {"basic, negative elbow, negative wrist", {20, -60, 60, -160, -40, 210}, 6, 26},
    {"basic, positive elbow", {-30, -20, -60, 35, 50, -45}, 0, 39},
    {"overhead, negative elbow", {25, -160, 90, 20, 40, 30}, 3, 2},
    {"nearest equivalent joint turn", {20, -60, 60, 20, 40, 330}, 2, 2}
  };
  Kr10Kinematics solver(model);
  for (const auto& test : cases) {
    qInfo() << "Round trip:" << test.name;
    const M4d target = solver.solveFK(test.joints).back();
    V6d seed = test.joints;
    seed[0] += 1.0;
    seed[5] -= 1.0;
    const auto solution = solver.solveIK(target, seed);
    require(solution.has_value(), "Representative IK failed");
    require(solution->status == test.status, "IK branch status changed");
    require(solution->turn == test.turn, "IK joint sign mask changed");
    for (std::size_t i = 0; i < DofCount; ++i) {
      require(std::abs(solution->q[i] - test.joints[i]) < 1e-6,
              "IK changed branch or nearest equivalent turn");
      require(solution->q[i] >= model.joints[i].qMin && solution->q[i] <= model.joints[i].qMax,
              "IK returned a joint outside limits");
    }
    requireTransform(solver.solveFK(solution->q).back(), target);

    RobotPreviewState state(model);
    QString error;
    require(state.initialize(error), qPrintable(error));
    const auto pending = state.forward(seed, error);
    require(pending.has_value(), qPrintable(error));
    state.commit(*pending);
    const RobotPose committed = state.pose();
    const auto targetPose = state.forward(test.joints, error);
    require(targetPose.has_value(), qPrintable(error));
    const auto inverse = state.inverse(targetPose->flange, error);
    require(inverse.has_value(), qPrintable(error));
    requireTransform(inverse->transforms.back(), target);
    requireUnchanged(state.pose(), committed);
  }

  // One target admits both wrist configurations; the seed selects which is kept.
  const M4d target = solver.solveFK(cases[0].joints).back();
  requireTransform(solver.solveFK(cases[1].joints).back(), target);
  const auto flipped = solver.solveIK(target, cases[1].joints);
  require(flipped.has_value() && flipped->status == 6, "Seed did not select negative wrist");
  require(std::abs(flipped->q[4] + 40.0) < 1e-6, "Wrist branch was silently switched");

  // Constrain a copy so this otherwise reachable target cannot use its selected q1.
  Kr10Model limited = model;
  limited.joints[0].qMax = 10.0;
  Kr10Kinematics limitedSolver(limited);
  require(!limitedSolver.solveIK(target, cases[0].joints), "IK ignored a joint limit");
}

void limitsAndFailedCalculations(const Kr10Model& model)
{
  const V6d expectedMin{-170, -185, -137, -185, -120, -350};
  const V6d expectedMax{170, 65, 163, 185, 120, 350};
  RobotPreviewState state(model);
  QString error;
  require(state.initialize(error), qPrintable(error));
  const auto initial = state.forward(V6d{20, -60, 60, 20, 40, 30}, error);
  require(initial.has_value(), qPrintable(error));
  state.commit(*initial);
  const RobotPose committed = state.pose();

  for (std::size_t i = 0; i < DofCount; ++i) {
    require(model.joints[i].qMin == expectedMin[i] && model.joints[i].qMax == expectedMax[i],
            "Fixture joint limits changed");
    for (double boundary : {expectedMin[i], expectedMax[i]}) {
      V6d joints = committed.joints;
      joints[i] = boundary;
      require(state.forward(joints, error).has_value(), "Inclusive joint boundary rejected");
      requireUnchanged(state.pose(), committed);
    }
    for (double invalid : {expectedMin[i] - 0.001, expectedMax[i] + 0.001,
                           std::numeric_limits<double>::quiet_NaN(),
                           std::numeric_limits<double>::infinity()}) {
      V6d joints = committed.joints;
      joints[i] = invalid;
      error.clear();
      require(!state.forward(joints, error), "Invalid forward input accepted");
      require(!error.isEmpty(), "Invalid forward input must report an error");
      requireUnchanged(state.pose(), committed);
    }
    for (double invalid : {std::numeric_limits<double>::quiet_NaN(),
                           std::numeric_limits<double>::infinity()}) {
      V6d flange = committed.flange;
      flange[i] = invalid;
      error.clear();
      require(!state.inverse(flange, error), "Nonfinite inverse input accepted");
      require(!error.isEmpty(), "Invalid inverse input must report an error");
      requireUnchanged(state.pose(), committed);
    }
  }
  V6d unreachable = committed.flange;
  unreachable[0] = 1e6;
  error.clear();
  require(!state.inverse(unreachable, error), "Unreachable target accepted");
  require(!error.isEmpty(), "Unreachable target must report an error");
  requireUnchanged(state.pose(), committed);
}

} // namespace

int main(int argc, char** argv)
{
  QCoreApplication app(argc, argv);
  try {
    const auto model = Kr10Model::fromJson(
        QStringLiteral(ROBOCRAP_SOURCE_DIR "/resources/json/kr10.json"));
    require(model.has_value(), "Cannot load resources/json/kr10.json fixture");
    homeAndConventions(*model);
    roundTripsAndBranches(*model);
    limitsAndFailedCalculations(*model);
    qInfo() << "Robot numeric regressions passed";
    return 0;
  } catch (const std::exception& error) {
    qCritical() << error.what();
    return 1;
  }
}
