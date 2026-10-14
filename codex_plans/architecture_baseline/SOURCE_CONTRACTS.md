# Architecture step 01: source and configuration baseline

Captured on 2026-10-14 from working-tree HEAD `3709a12ca3726858f2116608f4dd3dc2c01a336d`.
This is a source/configuration record. The user explicitly skipped numerical input and result
capture; no accepted TCP calibration, generated motion, or numerical equivalence is asserted.
No application source, build configuration, or resource was changed for this step.

## Recorded artifacts

`manifest.json` records the initial dirty paths and their SHA256 hashes, tracked text-input
inventory, the excluded RSI files and their hashes, and the executable/cache identity.
`interfaces.json` records current CMake ownership/usage requirements, application imports,
QML foreign declarations, invokables, and string-dispatched device calls with source locations.

`inputs/XP.json`, `inputs/XC.json`, and `inputs/kr10.json` are byte-identical snapshots of the
existing project plane measurements, cylinder measurements, and robot definition. They are
source snapshots, not a newly accepted animation request. XP uses probe radius 0 mm; XC uses
1.5 mm; both use dir 1. Their coordinates and radii are copied without conversion or fitting.
No profile request, before/after report, joint-point dump, or test harness was created.

## Existing executable and configuration

The executable is `build/Felgo_SDK_Desktop_Qt_6_8_3_MinGW_64_bit-Debug/robocrap.exe`.
Its SHA256 is `7C88C80F983A1156865021B1C2161408EB35E93B4FBF244E5CE115D93537A609`,
size 228425183 bytes, modification time 2026-10-14 13:16:03.0737276 UTC.
The existing CMake cache points to this repository, Ninja, Debug,
`C:/Felgo/Tools/mingw1310_64/bin/g++.exe`, and Qt at
`C:/Felgo/Felgo/mingw_64/lib/cmake/Qt6`. USE_HOT_RELOAD is OFF; FAST_QML_BUILD is ON.

The build log contains object entries for the current profiler, chamfer motion/validation,
kinematics, and end-effector code followed by the executable link. Source modification times
precede those build records. Debug symbols contain the current `runPreparation` function and
source layout. This is consistent provenance evidence, not an embedded commit/source-hash proof.
GDB was used only to inspect the executable's symbols/types, without starting the application.
No compiler, linker, CMake configure/build, QML lint, profiler, or application runtime was run.

## Current authored targets

Root `CMakeLists.txt` declares robocrap, its application QML/resources, UI modules, and deployment.
The executable links Backend and its plugin, Scene, Devices, 3D and its plugin, Styles,
Components, and Qt Core/Gui/Quick/Qml/Network/Charts/Xml/Widgets. Both hot-reload branches exist;
the current source default is OFF.

`src/CMakeLists.txt` declares RoboCrapGeometry with geometry, KR10 numerical model/kinematics,
and all path-generation sources. It publicly exports src and Eigen includes and QtCore.
RoboCrapScene contains document/calibration/machining/import/preparation code, publicly
exports src, and publicly links Geometry and QtQml.

RoboCrapDevices currently contains both I/O workers and GUI DeviceHub/DeviceRunner/ProfileModel.
It publicly exports src and links Geometry, QtNetwork, and QtQml. RoboCrapBackend contains
`backendqmltypes.h`, uses URI RoboCrap.Backend, and publicly links Scene and Devices.

`src/3d/CMakeLists.txt` declares RoboCrap3D with URI RoboCrap.Viewport3D. It publicly exports src
and links Geometry and Qt Core/Gui/Qml; Scene and OCCT are private dependencies. It also owns
the full robot model/state and profile CLI. The current target graph is acyclic, but broad src
include exports expose more headers than the link diagram alone describes.

## Application entry points and QML contracts

`src/main.cpp` handles `--profile-chamfer input.json report.json` before constructing devices,
scene objects, or the QML engine. In `src/3d/chamferprofile.cpp`, previewOnly=true returns after
animation generation and before PreparedChamferJob::create. The omitted flag defaults to false.
This branch was inspected, not executed.

Main imports RoboCrap.Backend as Backend, RoboCrap.Viewport3D as Viewport3D, Components, and Styles.
Backend registers Scene, Hub, Machining, MachiningPreparation, EndEffectors, geometry views,
PlaneImporter, DeviceRunner, DeviceProfileModel, and PlcMessage through foreign declarations.
Viewport registers OccController. RDTResponse directly declares QML_VALUE_TYPE(rdtResponse),
with generated foreign metadata forwarded into the application module.

Toolbar Generate Path opens the editor. The panel Generate commits draft settings and invokes
SceneMachining::generate; Cancel restores captured settings. Dry Run, Pause, and Stop use the
existing machining coordinator. Dry Scan only changes UI flags; Scan/Start Machining remain
disabled placeholders. End-effector Apply is separate from generation; accepted TCP starts empty.
The ER display is read-only and Apply passes the original ER values rather than display rounding.

QML device requests use Backend.Hub.device(key), then DeviceRunner::invoke(name, args).
Current call names are connect/disconnect, PLC setOutput, FTS startStreaming/stopStreaming/bias/
startLogRecording/saveLogToDefaultFile, and the dormant KukaPanel's generateTrajectory and
startStreaming/stopStreaming. The invocation signal is queued to the device's I/O thread.

## Generate, calibration, and saved-motion identity

`SceneMachining::sourceParameters` in `src/scene/scenemachining.cpp` returns saved parameters
directly for a machining path. For a raw edge it looks up every source ID and requires a plane
and cylinder still to exist. Numerical parameters contain the ellipse plus cylinder origin/axis;
the plane-presence check remains part of accepted input states.

Generate builds ChamferPath on the GUI thread, copies robot/model/TCP/timing inputs into its
future, and accepts only complete results. `finishCalculation` compares input ID, calibration
revision, availability, and source state before insertion. A replacement is inserted before the
old path is removed; name and visibility are preserved. SceneMachiningData stores an immutable
shared motion, source edge ID, and calibration revision. Removing source geometry does not
destroy a completed motion. Model or effective TCP changes invalidate playback compatibility.

`src/scene/sceneendeffectors.cpp` composes FLANGE_ER * ER_TCP, commits coupled values before
notifications, and returns without notifications for unchanged accepted calibration. The
hard-coded initial ER roll is provisional; this source snapshot does not certify calibration.

## Dry Run and numerical invariants

ChamferMotion::evaluate(seconds) returns joints, FK TCP, and phase. It uses prepared clamped
cubic joint splines for each LIN and the shared central portion, and quintic progress for HOME
PTP phases. It performs no IK, arc integration, or new spline construction during playback.
The outer motion owns ChamferPathParameters; central motion owns the full analytical curve.

SceneMachining retains active playback separately from selected input. Its direct pose signal
currently passes six joints through QList to OccController, which constructs/presents a pose
and commits after AIS-transform acceptance; drawing occurs later. Pause resumes the same motion.
Stop presents time zero/HOME immediately while the preview is available, then ends playback.
Completion is at HOME. Readiness/calibration/mode/path deletion retain their current cleanup.

Preserve exact shared phase endpoints, continuous joint winding, seeded IK branch, fixed
pre-IK animation schedule, model-rated joint speeds, and polynomial position/speed extrema.
Position tolerance is 1e-4 mm; rotation-entry tolerance 1e-6; wrist singularity threshold 1e-9.
Joint-acceleration magnitude rejection remains disabled for preview central/LIN/splines;
finiteness and central XYZ acceleration checks remain, and HOME uses configured acceleration.
The 0.01 mm geometric point criterion does not establish exact TCP/orientation interpolation error.

## Excluded RSI compatibility to preserve

The manifest contains 31 frozen RSI source/header/controller/contract/example records. Their
hashes define the unchanged input set for this step; no RSI operation was executed or modified.

Frozen prepared-job consumers retain ChamferMotion::points, boundaries, robot, parameters,
centralMotion, centralTimeScale, and TCP-returning evaluate(seconds). The Cartesian sampler
reads saved winding seeds/timing/central profiles and uses ChamferSamplingParameters.
Kr10Kinematics::IkSolution q/status/turn and the numerical Eigen types remain compatible.

PreparedChamferJobPtr remains a copied shared_ptr<const PreparedChamferJob> Qt metatype.
SceneMachiningPreparation's queued delivery/invalidation to RsiDevice and its shutdown wait
remain separate from animation. RsiDevice stores prepared metadata but does not execute that
prepared chamfer stream. Correction types still depend on sampler/numerical headers; the legacy
demo still depends on geometry/path code. These dependencies are recorded, not removed.

FTS and RSI share ControlIO; PLC uses GeneralIO. The per-packet FTS signal reaches setForce
directly on ControlIO. DeviceRunner/property maps remain on the GUI thread; device requests and
state delivery remain queued. Stop completes before thread quit/wait, and outstanding Generate,
CAD/preparation/save work retains its existing lifetime rules.

## Scope and verification result

Step 01 is complete for the narrowed scope authorized by the user: source/configuration,
interfaces, immutable input snapshots, executable identity, and excluded RSI compatibility.
Numerical-case capture was explicitly skipped. There are no measured preparation timings,
movement durations, saved-point counts/joint/TCP results, or pass/fail motion cases from this step.
Source facts do not replace future numerical or runtime verification when requested.

Final checks passed: 193 present source/configuration hashes, absence of the deleted input,
31 frozen RSI hashes and clean Git status, preserved pre-existing user edits, three exact JSON
copies, all captured interface source locations, and the executable hash. JSON parsing,
ExecPlan structure, new-file whitespace, and `git diff --check` also passed.
