# Offline scene tests

The scene tests use the real RoboCrap.Backend C++ module and the source workspace panels.
They exercise shared object identity, collection updates, rename/visibility notifications,
selection, invalid imports, ID collisions, repeated imports, and the sample JSON through
PlaneImporter. The runner does not create hardware devices or an OCCT viewport.

Build verification must be explicitly requested under AGENTS.md. With that authorization,
select a valid matching Qt/MinGW build as described there, then enable the optional runner:

```powershell
cmake -S . -B "$buildDir" -DROBOCRAP_BUILD_SCENE_TESTS=ON
cmake --build "$buildDir" --target sceneobjecttests boundedplanetests --parallel
& "$buildDir/boundedplanetests.exe"
& "$buildDir/sceneobjecttests.exe" -platform offscreen -import "$PWD/qml/Modules" -o -,txt
```

Run from the repository root with the selected kit's Qt and MinGW `bin` directories on PATH.
For a multi-configuration generator, use the executable inside its selected configuration
directory. The first command is for an already configured valid build; follow AGENTS.md
when configuring a new build. The option defaults to OFF, so normal builds gain no test
dependency. QuickTest is required only when the option is enabled. No CTest registration
is added.

Expect every test to pass with no binding, type, or import errors. The runner embeds the
application icons, and the JSON importer test reads resources/json/rough-plane-sample.json
from the source tree. A stock qmltestrunner alone no longer suffices: the backend is a static
C++ module and must be linked into the runner.

The suite creates a fresh SceneModel for each test. Models own and destroy their children;
tests must not replace the objects list or call destroy() on an imported SceneObject.

The boundedplanetests executable checks the actual C++ fitter and rectangle construction:
orthonormal right-handed axes, projected sample containment, original sample retention,
center and dimensions, point order, noisy data, horizontal and steep planes, and rejected
degenerate/nonfinite input. Expect `Bounded plane regressions passed` and exit code zero.
The QML importer test checks bounded Properties and retained geometry after another import.
It calls load(url, scene) and consumes loaded(object); no QML-side insertion is needed.
The C++ runner also checks shared coefficient storage and destination destruction before
worker completion. QML cases cover failed imports and missing destinations.

The rectangle origin is its center. U is the projection of world X onto the plane, except
when abs(normal.x) >= 0.9, where world Y is used. V = normal cross U. Corners are
origin +/- U*width/2 +/- V*height/2. This encloses projected points in that frame; it is
not a minimum-area rectangle. Dimensions preserve input units. The sample JSON should
produce origin (0,0,10), width approximately 222.287572, height 189.177788, and 5 points.
Original points remain in `PlaneGeometry::boundedPlane()->points` in input order.
