# Busy indicator over the OCCT scene

This document records a working way to draw a Qt Quick item (a `QxBusyIndicator`) over the
native OCCT view on Windows, and the facts that forced each part of it. It was developed and
verified by running the application on 2026-10-18.

Status: **not in the current code.** On 2026-10-19 the indicator was moved to the status bar,
because about 50 lines of overlay code were judged too much for a loading indicator. The
complete overlay code is reproduced below; it is also in commit `2a8fa4f`. Two related changes stayed in the code:
`OccViewport::setRobot` draws immediately (see "Readiness after the first frame") and
`QxBusyIndicator` uses a render-thread `RotationAnimator`.

## Requirements

- The indicator spins in the centre of the 3D viewport while the scene loads at startup, and
  optionally while a trajectory is calculated.
- It has a transparent background: only the bars are visible.
- It stays centred while the layout settles, the main window moves and the split view changes.
- It is never drawn over other applications, and it does not appear when the application closes.
- It shows no layout shift in the main window (AGENTS.md "Mandatory interface stability").

## Facts that shape the solution

### 1. No Qt Quick item can draw over the OCCT view

`RobotViewport.qml` embeds `OccController.viewWindow` (an `OccViewWindow`, a `QWindow` that
OpenGL renders into) with `WindowContainer`. The embedded window is a native child window of
the main window. Qt Quick paints all its items (including `Popup` and items with a high `z`)
into the main window's surface, and Windows always draws a native child window over its
parent's surface. The Qt 6.8 `WindowContainer` documentation states this: sibling or child
items "will not automatically overlap the embedded window", and recommends putting such items
inside their own dedicated child window.

Result: the indicator needs its own native window.

### 2. A child window cannot be transparent on Windows

Following the documented workaround, the indicator was put into a `Window` embedded by a second
`WindowContainer` (a sibling declared after the view's container, so it is stacked above it).
It was correctly on top, but:

- `color: "transparent"` produced a **white square**. Qt's Windows backend makes only top-level
  windows layered (alpha-blended); a child window's transparent pixels have nothing to blend with.
- Filling the window with the empty view's grey (`#4D4D4D`, sampled from a screenshot) hid the
  square but the window **flashed white, then grey**, several times at startup. Each show or
  re-expose displays the native surface before Qt Quick renders the first frame into it, and the
  viewport is laid out several times while starting (it begins 0 px high, see the
  `setGeometry: Unable to set geometry 398x-2+226+123` warning in the startup log).

Result: transparency requires a top-level window.

### 3. A top-level window must be shown after the main window is activated

A frameless, transparent `Qt.Tool` window declared inside the viewport item (so its
`transientParent` is the main window) rendered correctly (`frameSwapped` fired), but the user
could not see it: it was behind the main window. When the application was minimized, it kept
spinning over the desktop, which an owned window would not do. Its `visible` became true as soon
as the main window became visible, before the main window was activated and brought forward.

- `Qt.WindowStaysOnTopHint` (`HWND_TOPMOST`, set in `WindowCreationData::initialize` of the Qt
  Windows plugin) made it visible, but it then floated over **every** application, including
  when the user clicked another window right after start.
- Hiding it when the main window is inactive did not fix the topmost case, because the click
  happens while the GUI thread is blocked (fact 4) and QML cannot react.
- Working solution: **no** stays-on-top flag; `visible` additionally requires
  `transientParent.active`, so the overlay is shown only after the main window is active and
  is created on top of it. Other applications cover it normally.

### 4. The GUI thread is blocked during the visible loading phase

Startup sequence, from a log of the running application (times in ms, `Date.now() % 100000`):

| Time | Event |
| --- | --- |
| 72562 | Overlay created; `loading: true`, `ready: false`, main window not visible yet |
| 72597 | Main window visible, overlay becomes visible |
| 72934 | Overlay renders its first frame; `loading` turns false (BREP parsing finished) |
| 73684 | `ready` turns true |

- `OccController.loading` is true only while `CadLoadWorker` parses the BREP files in a thread.
  `main.cpp` starts that before QML is loaded, so `loading` can end before the window appears.
  The visible "grey" phase is OCCT viewport initialization and robot installation on the
  **GUI thread**, while `loading` is already false. The condition must therefore be
  "until `ready` first becomes true (or `errorString` is set)", not `loading`.
- While the GUI thread is blocked, QML bindings, handlers and `Timer`s do not run, and a
  `NumberAnimation` (GUI thread) stops. `QxBusyIndicator` rotates its ring with a
  `RotationAnimator`, which runs on the scene graph render thread and keeps turning.

### 5. Readiness after the first frame

`OccViewport::setRobot` used to call `requestRender()`, which only starts a 0 ms single-shot
timer. `OccViewWindow::attachRobot` emits `readyChanged` right after `setRobot` returns, so
`ready` was reported (and the overlay hidden) before the robot was drawn. `setRobot` now calls
`flushRender()` directly, after `fitInitialCamera()`. This change was kept in the code:

```cpp
ViewportOperationResult OccViewport::setRobot(const RobotVisualModel& visuals,
                                              const CadLoadResult& shapes,
                                              const DrawingPose& pose)
{
  if (!isValid()) return {false, QStringLiteral("Robot installation: viewport is unavailable.")};
  const auto result = m_robot.load(visuals, shapes, pose);
  if (result.accepted) fitInitialCamera();
  // Draw now rather than on the next event-loop turn, so the robot is on screen
  // when readiness (which hides the loading overlay) is reported after this call.
  flushRender();
  return result;
}
```

### 6. Closing the application clears `ready`

When QML is destroyed, `RobotViewport`'s `Component.onDestruction` calls
`OccController::detachViewWindow`, which releases the surface; `OccViewWindow::releaseSurface`
emits `readyChanged` and `ready` becomes false. A condition "not ready" briefly showed the
spinner on close. A latch (`robotShown`), set once `ready` first becomes true, restricts the
startup condition to startup.

### 7. Centring on the viewport

- The overlay's `x`/`y` are screen coordinates. The view window's `x`/`y` are relative to the
  main window's client area, and the main window's `x`/`y` are its client-area screen
  position. Log of one run: main window `x = 157`, view window `x = 226`, viewport item
  `mapToGlobal(0, 0).x = 383 = 157 + 226`; likewise `121 + 123 = 244` for `y`.
- During startup the split view moves the viewport about 96 px left **without changing its
  size**, so size-change handlers miss it, and a periodic `Timer` never fired because the GUI
  thread was blocked right after the overlay appeared. The centre was off by about 48 px
  (half of that move).
- `WindowContainer` moves the view window whenever the viewport item moves, and `QWindow`
  `x`/`y`/`width`/`height` are notifiable properties. Binding the overlay position to the main
  window position plus the view window geometry updates it on every move, with no handlers.
- `visible` also waits for `container.window.height > 0`, so the overlay first appears after
  the view window has been laid out (it is 0 px high at first).

## Complete overlay code

`RobotViewport.qml` as it was when the overlay worked (`QxBusyIndicator` is in
`qml/Modules/Components`; `Main.qml` set `busy: App.Machining.calculating` on `RobotViewport`):

```qml
import QtQuick

import Components 1.0
import Styles 1.0
import RoboCrap.Viewport3D 1.0 as Viewport3D

Item {
  id: root

  // Shows the busy indicator over the scene, in addition to the startup scene loading.
  property bool busy: false

  Rectangle {
    anchors.fill: parent

    color: Colors.background.dp01
    border.color: Colors.background.dp12
    border.width: Metrics.w1

    WindowContainer {
      id: container

      anchors.fill: parent
      anchors.margins: Metrics.w1

      focus: false
      activeFocusOnTab: false
      window: Viewport3D.OccController.viewWindow

      Component.onDestruction: {
        Viewport3D.OccController.detachViewWindow(container.window)
        container.window = null
      }
    }
  }

  // The OCCT view is a native child window that covers Qt Quick items, and Windows cannot make
  // child windows transparent, so the indicator is a transparent top-level window (transient for
  // the main window). It is not stays-on-top, which would float it over every application.
  Window {
    id: loadingOverlay

    // Set once the robot is shown; closing the view clears `ready` again, which is not loading.
    property bool robotShown: false
    // On-screen viewport centre; the view window is positioned in the main window's client area.
    readonly property point viewportCenter: loadingOverlay.transientParent && container.window
        ? Qt.point(loadingOverlay.transientParent.x + container.window.x + container.window.width / 2,
                   loadingOverlay.transientParent.y + container.window.y + container.window.height / 2)
        : Qt.point(0, 0)

    flags: Qt.Tool | Qt.FramelessWindowHint | Qt.WindowDoesNotAcceptFocus | Qt.WindowTransparentForInput
    color: "transparent"
    x: loadingOverlay.viewportCenter.x - loadingOverlay.width / 2
    y: loadingOverlay.viewportCenter.y - loadingOverlay.height / 2
    width: indicator.implicitWidth
    height: indicator.implicitHeight
    // While busy, and from startup until the robot is first shown (OccController.loading covers
    // only CAD parsing); only once the view window is laid out and the main window is active,
    // so the indicator appears at the centre and activation does not bring the main window over it.
    visible: (root.busy
              || !loadingOverlay.robotShown && Viewport3D.OccController.errorString.length === 0)
             && container.window?.height > 0 && (loadingOverlay.transientParent?.active ?? false)

    Connections {
      target: Viewport3D.OccController

      function onReadyChanged() { if (Viewport3D.OccController.ready) loadingOverlay.robotShown = true }
    }

    QxBusyIndicator {
      id: indicator

      anchors.fill: parent
      diameter: 48
      running: loadingOverlay.visible
    }
  }
}
```

Notes on the code:

- `transientParent` is not set explicitly: a `Window` declared inside an `Item` automatically
  becomes transient for the item's window (Qt 6.8 `Window` documentation), which also delays
  showing it until that window is shown.
- `Qt.WindowTransparentForInput` lets clicks reach the scene; `Qt.WindowDoesNotAcceptFocus`
  keeps the overlay from taking activation from the main window.
- `Qt.FramelessWindowHint` is required for a transparent window colour (Qt `Window.color`
  documentation).

## Approaches tried and their results

| Approach | Result |
| --- | --- |
| Item over the `WindowContainer` | Hidden under the native view (fact 1) |
| Top-level window, `visible: OccController.loading` | Hidden before the grey phase started (fact 4) |
| Top-level window shown with the main window | Behind the main window; spun over the desktop when minimized (fact 3) |
| Child window in a second `WindowContainer`, transparent | White square (fact 2) |
| Child window filled with `#4D4D4D` | Correct, but flashed white/grey at startup (fact 2) |
| Top-level window with `Qt.WindowStaysOnTopHint` | Visible, but over every application (fact 3) |
| Re-centring on resize, window move or a 100 ms `Timer` | Off-centre by about 48 px (fact 7) |
| Final code above | Transparent, centred, shown only while the application is active |

## Checks when reusing it

- Start the application and watch the grey phase: the spinner must be visible and centred.
- Click another application during startup: the spinner must not stay above it.
- Minimize during startup: the spinner must disappear with the application.
- Close the application: the spinner must not flash.
- Drag the split-view handles and move the main window while it is visible.
- Verify on the target GPU that the window background is transparent, not black or white.
