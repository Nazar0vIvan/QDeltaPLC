# Offline robot numeric regressions

`robotnumerictests` compiles the production KR10 model parser, math helpers,
kinematics and preview state directly. It uses `resources/json/kr10.json` from the
source directory recorded at configure time. Keep that source fixture available.
It creates only a QCoreApplication: no viewer, CAD loading, QML engine or devices.

The cases pin the home joints/frame, degree-based XYZABC and Z-Y-X conventions,
representative basic/overhead shoulder and positive/negative elbow/wrist branches,
status and joint-sign masks, nearest equivalent turns, FK/IK round trips,
inclusive joint limits, invalid/nonfinite input, unreachable and singular IK, and
unchanged committed joints, flange and all seven link transforms after calculations.
An inconsistent home frame must also fail initialization without committing.
These are calculation tests; they do not verify OccController presentation rollback.

Build verification requires explicit authorization. Nothing was built or run when
step 1 was implemented. After authorization, select a matching existing build using
AGENTS.md (check its cache first), with the matching Qt/MinGW and the configured
OCCT SDK `bin` directories on PATH. The root project still requires its usual SDKs;
the option does not provide a standalone configure path. For an existing compatible
build with hot reload OFF:

```powershell
cmake -S . -B "$buildDir" -DROBOCRAP_BUILD_ARCHITECTURE_TESTS=ON
cmake --build "$buildDir" --target robotnumerictests --parallel
& "$buildDir/robotnumerictests.exe"
```

Expect `Robot numeric regressions passed` and exit code zero. Failures return one
with a diagnostic. The option defaults to OFF and adds no test framework dependency
or CTest registration. OCCT runtime DLLs are resolved from PATH; this target does
not deploy robot CAD. Adjust the executable path for multi-configuration builds.
Do not use stale binaries as evidence for changed sources.

Existing scene regressions remain separately gated by ROBOCRAP_BUILD_SCENE_TESTS;
see tests/qml/README.md and ARCHITECTURE_EXECPLAN.md for their acceptance commands.
