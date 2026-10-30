# RSI_Probe_Lin test run: controller/PC contract

A hard-coded RSI LIN for testing the WP-500 stop: from HOME and a PTP to `TestStart`, the PC streams a constant
+0.004 mm per 4 ms cycle along WORLD X (1 mm/s) for 100 mm, followed by 50 zero cycles. The RSI
context stops the motion when `$IN[11]` (WP-500 touch) switches on. After the stop or the
completed 100 mm the program only ends: no retract, no return to HOME.

Controller files:
- `RSI_Probe_Lin.src` and `RSI_Probe_Lin.dat` (module `RSI_Probe_Lin`).
- `RSI_Probe_Lin.rsi`, `.rsi.xml` and `.rsi.diagram`: the RSI Visual context described below,
  loaded by `RSI_CREATE("RSI_Probe_Lin.rsi")`. Copy them from the RSI Visual export.
- `RSI_EthernetConfig.xml`: unchanged, shared with `RSI_Chamfer_Hole`.

PC side: the "RSI test LIN" button on the Network page calls
`SceneMachiningPreparation::prepareTestLin` (`src/application/scene/scenemachiningpreparation.cpp`),
which replaces any held stream, deletes `C:\RoboCrapJobs\shared_data.txt` and installs the test
increments in `RsiDevice` through the existing `correctionsReady` link. `RsiDevice` behaves as in
`RSI_Chamfer_Hole_CONTRACT.md` ("PC behavior"); packet captures are named
`rsi_packets_test_lin_<UTC>.csv`. A later RSI Sampling replaces the test stream.

## Frames and motion

KRL: `$BASE=$NULLFRAME`, `$TOOL=TOOL_DATA[5]` (WP-500, X −0.048, Y −0.097, Z 127 mm, A=B=C=0),
`$IPO_MODE=#BASE`. HOME (A1…A6 = 0, −90, 90, 0, 0, 0 deg) has A5 = 0, a wrist singularity in which
A4 and A6 are collinear; the first run, which started the RSI motion there, stopped with an
excessive motor current error. The program therefore moves PTP from HOME to `TestStart`
(A1…A6 = 0, −90, 90, 0, 35, 0 deg, in the DAT). There the TCP is at about X 979.6, Y −0.1,
Z 961.3 mm with the stylus pointing forward and 35° downward; the motion moves it to
X ≈ 1079.6 mm, where A5 is about 35.7° (scratch model estimate, not controller values).
The constant speed has no ramp; the step from standstill to 1 mm/s is the same as in the
calibration Approach segment.

## RSI Visual context `RSI_Probe_Lin`

Start from a copy of the `RSI_Chamfer_Hole` context (File > Save as) and change only DIGIN1, OR1,
the STOP1 input and the POSCORR1 limits.

| Object | Parameters | Connections |
| --- | --- | --- |
| ETHERNET1 | ConfigFile `RSI_EthernetConfig.xml`, Timeout 10, Precision 1 (as in RSI_Chamfer_Hole) | In1 ← SEN_PINT1.Out1, In2 ← SEN_PINT2.Out1 |
| SEN_PINT1 | Index 1 (`$SEN_PINT[1]`, Start) | Out1 → ETHERNET1.In1 |
| SEN_PINT2 | Index 2 (`$SEN_PINT[2]`, Result) | Out1 → ETHERNET1.In2 |
| POSCORR1 | RefCorrSys World; X limits must allow at least +100 mm (e.g. ±110 mm), Y/Z small (e.g. ±5 mm), rotation limit small (e.g. 1°) | CorrX..CorrC ← ETHERNET1.Out1..Out6 |
| DIGIN1 | first input 11, one bit (`$IN[11]`) | Out1 → OR1.In2 |
| OR1 | 2 inputs | In1 ← ETHERNET1.Out7 (PC `Stop`), In2 ← DIGIN1.Out1; Out1 → STOP1.In1 |
| STOP1 | Mode ExitMoveCorr | In1 ← OR1.Out1 |
| MAP2SEN_PINT1 | Index 2 | In1 ← ETHERNET1.Out8 (PC `Result`) |

STOP1 ends `RSI_MOVECORR` at the rising edge of OR1: either the PC's Stop after the streamed
100 mm and Result handshake, or `$IN[11]` switching on. The stop is decided on the controller in
the same cycle; the PC is not involved. Because only a rising edge stops the motion, the SRC
refuses to start when `$IN[11]` is already TRUE.

The RSI 3.3 manual describes DIGIN only as "returns the value of a range of digital inputs $IN"
and STOP as "stops a motion at a positive signal edge"; the parameter names in this table follow
RSI Visual's object properties and must be checked there.

## Touch signal

`$IN[11]` is TRUE while the WP-500 is deflected: the PLC copies X2.0 to Y1.5, which is wired to
EL1809 input 11 (`docs/ARCHITECTURE.md`, "Additional hardware and Delta I/O wiring"). Datasheet
delays on this path are about 15 ms (PLC input filter 10 ms, PLC scan, EL1809 filter 3 ms), that is
about 0.015 mm at 1 mm/s, within the WP-500's 3 mm stylus travel.

## Not yet verified on the controller

- Compilation of the SRC, the DIGIN/OR/STOP parameter names and that STOP1 reacts to OR1.
- Braking distance after ExitMoveCorr at 1 mm/s.
- That the POSCORR1 limits accept the 100 mm X correction.
