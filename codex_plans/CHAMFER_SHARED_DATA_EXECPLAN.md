# Export shared KRL transfer data (step 9)

This living ExecPlan follows PLANS.md.

## Purpose / Big Picture

RSI Sampling will publish the accepted prepared job's staging and clear-end frames and saved transfer settings to C:/RoboCrapJobs/shared_data.txt. KRL reads three XYZABC rows and then speed and acceleration, with no keys or headers. The file does not contain the central RSI correction stream. Start Machining remains disabled.

## Progress

- [x] (2026-10-09) Inspect preparation, encoder, saved motion, QML status, KRL and local KUKA manual.
- [x] (2026-10-09) Implement copied-input payload construction, accepted-result atomic publication and status.
- [x] (2026-10-09) Update KRL scalar reader and transfer assignments; document exact contract.
- [x] (2026-10-09) Review source, new files and whitespace; record verification limitations.

## Context and Orientation

src/scene/scenemachiningpreparation.cpp runs PreparedChamferJob::create in a future and rejects results when selection, calibration, availability or correction limits changed. src/pathgeneration/chamfer/rsiposeencoder.h retains the initial XYZABC and serialized increments. The new scene/chamfershareddata helper constructs file bytes on that worker. Only the GUI coordinator may publish after its existing current-input check. QFile/QSaveFile remain in the Scene layer, outside numerical geometry and ControlIO. qml/Main.qml displays separate export status through the existing wrapping Label. resources/files/rsi/RSI_Hole.src uses externally available READ_FRAME and CLOSE_FILES functions supplied by the user.

## Plan of Work

First add a cohesive worker return value containing the prepared result, file bytes and export error. Build three rows from staging-in XYZ with initial encoded ABC, exact initial encoded XYZABC, and terminal initial-plus-cumulative encoded corrections. Check these against saved staging and analytical endpoints. Convert the saved transferFeed and transferAcceleration by dividing by 1000. Use C-locale max_digits10 strings and CRLF. Require encodingReady, including configured correction limits, before producing a publishable payload.

Next retain existing future stale-result rejection. After accepting a job, write the small payload using QSaveFile, without direct-write fallback. Create C:/RoboCrapJobs when necessary. Expose sharedDataExported and sharedDataStatus through preparedChanged, clear them with the selected prepared job, and preserve valid sampling if export fails. No file is written by the worker; no live shared-folder writes occur during implementation.

Finally extend the KRL reader with two synchronous krl_fscanf calls using %f, check both states and positive values against $VEL_MA.CP and $ACC_MA.CP, and assign the two CP settings immediately before each LIN. Preserve existing user edits, BASE_DATA[1], tool/load 6, HOME and RSI flow. PsOut continues to use Ps XYZ and LeadEnd ABC.

## Decision Log

Decision (2026-10-09): publish automatically after RSI Sampling acceptance, with independent export status. This uses an existing explicit user action and the immutable job, avoids another button and prevents a stale worker from writing a selected replacement job's file.

Decision (2026-10-09): retain the existing three-frame order and derive PsOut in KRL. Its orientation is already determined by the third frame; a fourth frame would be redundant. Do not add identity keys or tokens because the user requested positional numeric data only. The file cannot prove freshness or controller readiness; later nominal execution must check its session independently.

## Surprises & Discoveries

The user already changed the live KRL template to shared_data.txt and BASE_DATA[1]. Preserve these. The local KUKA System Variables 8.1, 8.2 and 8.3 manual, pages 14 and 112, defines CP acceleration in m/s squared and velocity in m/s, bounded by machine data. No installed maximum is assumed. The Windows sandbox cannot start processes; user authorized fallback access after an accidental denial.

## Concrete Steps

Work in D:/Qt/QtProjects/RoboCrap. Add src/scene/chamfershareddata.h/.cpp and their RoboCrapScene CMake entries. Update coordinator future type, status and completion publication, QML status, KRL SRC/DAT and contract, and AGENTS.md. Run git diff --check and inspect all new files. Do not add project tests, run a build or connect hardware.

## Validation and Acceptance

Source inspection must show exactly three six-value semicolon rows followed by two scalar rows. A saved 10 mm/s and 20 mm/s squared transfer produces numeric values 0.01 and 0.02. The initial Euler branch must be identical to encoding.initialCoordinates; the terminal branch must equal that initial vector plus accumulated serialized increments. KRL reads in the same order, closes the file even on read failures and rejects bad settings before PTP Ps. Failure or stale preparation must not replace the file; an accepted successful export must report its path.

When separately authorized, build robocrap and run its QML lint target, then exercise RSI Sampling disconnected from hardware with a valid correction profile and inspect the five rows and failure status for a locked destination. KRL compilation and installed BASE/TOOL agreement require controller-side verification; do not claim them here. An existing file remains on invalidation or failed export and must not be treated as a current executable job. Controller REAL precision and Euler branch at actual entry remain session-validation concerns.

## Idempotence and Recovery

Repeated successful sampling atomically replaces the same file. A failed write preserves the prior complete file and reports failure; regenerate/retry sampling after correcting the cause. No deletion, deployment or hardware action is part of this task.

## Interfaces and Dependencies

ChamferSharedDataResult contains PreparedChamferJobResult prepared, QByteArray contents and QString error. prepareChamferSharedData(source, limits) returns it. writeChamferSharedData(contents) returns an empty QString on success or an error. Existing queued preparedJobReady delivery and job identity remain unchanged. No new dependency or public API removal.

## Outcomes & Retrospective

Implemented the worker payload, accepted-result atomic writer, QML export status and matching KRL reader/assignments. Source review traced the five rows, encoded Euler branches, unit conversions, CMake ownership, stale-result rejection and status reset. git diff --check passed; new source, KRL and plan files were separately checked for trailing whitespace. No build, QML lint, runtime, controller compilation or shared-folder publication was executed. Nominal RSI session integration remains pending. Failed export or invalidation leaves the previous disk file intact, explicitly documented because the requested numeric format carries no identity token.

Revision 2026-10-09: define positional format, accepted-result publication and controller rate validation for step 9.
