#pragma once

#include "rsiprotocol.h"
#include "pathgeneration/chamfer/chamferjob.h"

// Values shared by the XML, RSI_Hole graph and KRL terminal branch.
enum class RsiHoleResult { None = 0, Completed = 1, Aborted = 2, Faulted = 3 };

struct RsiHoleStatus
{
  int job = 0; // Positive controller INT token; zero means no matched job.
  int sample = 0; // Last submitted target; sample zero is the start/hold pose.
  RsiHoleResult result = RsiHoleResult::None;
};

struct RsiHoleRequest
{
  RsiProtocol::Response robot;
  RsiHoleStatus status;
  bool ready = false; // Graph-confirmed sensor-guided execution gate.
  int correctionStatus = 0; // POSCORR Stat; values greater than one indicate limiting.
};

struct RsiHoleReply
{
  quint64 ipoc = 0;
  RsiPoseCorrection correction; // Preserve the prepared strings exactly.
  RsiHoleStatus status;
  bool stop = false; // ExitMoveCorr edge, separate from the terminal reason.
};

struct RsiHolePacketResult
{
  QByteArray packet;
  QString error;
};

struct RsiHoleDecodeResult
{
  std::optional<RsiHoleRequest> request;
  QString error;
};

// Arithmetic/wire verification is independent of installed monitor readiness.
struct RsiHoleNominalAudit
{
  int correctionCount = 0;
  V6d initialCoordinates = V6d::Zero();
  V6d finalCoordinates = V6d::Zero();
  double maximumPositionError = 0.0; // mm
  double maximumRotationError = 0.0; // Rotation-matrix entry error.
  double savedDuration = 0.0;
  double controllerDuration = 0.0; // Final target occupies one full 4 ms cycle.
  RsiCorrectionLimitStatus limitStatus = RsiCorrectionLimitStatus::Unconfigured;
};

struct RsiHoleNominalAuditResult
{
  std::optional<RsiHoleNominalAudit> audit;
  QString error;
};

namespace RsiHoleProtocol {

RsiPoseCorrection zeroCorrection();
RsiHolePacketResult encode(const RsiHoleReply& reply);
RsiHoleDecodeResult decode(const QByteArray& packet);
// Copied-input preparation work, never an RSI receive-callback operation.
RsiHoleNominalAuditResult auditNominalJob(const PreparedChamferJob& job);

} // namespace RsiHoleProtocol
