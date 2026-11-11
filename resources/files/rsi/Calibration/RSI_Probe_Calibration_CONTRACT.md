# RSI_Probe_Calibration run: controller/PC contract

The probe sphere calibration session on the robot: HOME, then the steps of the calibration path
generated in the Calibration submode (114 calibration and 44 verification touches, 485 steps),
executed by KRL as PTP, LIN and CIRC moves, except that every approach to the sphere is a search
under RSI that stops at the WP-500 touch (`$IN[8]`). The plan behind it is
`plans/PROBE_SPHERE_RSI_CALIBRATION_EXECPLAN.md`.

Controller files (in the repository the SRC, DAT and this contract are in
`resources/files/rsi/Calibration/`, the Ethernet configuration and `FILE_UTILS/` in
`resources/files/rsi/`):
- `RSI_Probe_Calibration.src` and `RSI_Probe_Calibration.dat` (module `RSI_Probe_Calibration`) with
  the local functions `SEARCH()` and `READ_JOB()`.
- `RSI_Probe_Calibration.rsi`, `.rsi.xml` and `.rsi.diagram`: the RSI Visual context below, loaded
  by `RSI_CREATE("RSI_Probe_Calibration.rsi")`. Copy them from the RSI Visual export; do not
  hand-edit them.
- `RSI_EthernetConfig.xml`: shared with `RSI_Chamfer_Hole` and `RSI_Probe_Check`.
- `FILE_UTILS/REMOUNT_SHARE.src`, `READ_POS.src` and `READ_FRAME.src`: the helpers of
  `RSI_Chamfer_Hole`.

PC side: "RSI Sampling" in Measuring mode, Calibration submode, calls
`SceneMachiningPreparation::prepareCalibration` (`src/application/scene/scenemachiningpreparation.cpp`),
which runs `sampleProbeCalibrationRsi` (`src/trajectories/probe/probersistream.cpp`) on the saved
calibration path, writes `C:\RoboCrapJobs\calibration_data.txt` and installs one correction stream
per touch in `RsiDevice` (`src/devices/rsi/rsidevice.cpp`).

## Frames and motion

KRL: `$BASE=$NULLFRAME` (BASE = WORLD; `$ROBROOT` is zero in the archived machine data),
`$TOOL=ToolFrame` from the job file (the measuring flange-to-TCP transform of the sampled path, not a
`TOOL_DATA` entry), `$IPO_MODE=#BASE`, `$ORI_TYPE=#CONSTANT` (the probe orientation is fixed within
each pass), `$VEL.CP`/`$ACC.CP` from the job file (the Generate transfer feed and acceleration, by
default 0.01 m/s and 0.02 m/s²), `$VEL_AXIS[1..6]` and `$ACC_AXIS[1..6]` = 20 % for every PTP of the
session. The first `PTP HOME Vel= 100 % DEFAULT` runs before these assignments; the final HOME step
is the plain `PTP XHOME`, so it keeps 20 %. XYZ are mm; ABC are principal Z-Y-X Euler angles in
degrees. POSCORR1 uses RefCorrSys = World; a search stream carries only X/Y/Z increments.

Each search starts at the touch's start pose, 3 mm outside the nominal contact on the outward
sphere normal, and the PC streams the approach direction at the plan's search feed (1 mm/s: 1250
increments of 0.004 mm, 5 mm in total, then 50 zero cycles). The contact is expected after about
750 cycles; 2 mm beyond the nominal contact are allowed for the uncertainty of the contact position.

## RSI Visual context `RSI_Probe_Calibration`

Start from a copy of the `RSI_Probe_Check` context (File > Save as) and change only what the table
marks as new.

| Object | Parameters | Connections |
| --- | --- | --- |
| ETHERNET1 | ConfigFile `RSI_EthernetConfig.xml`, Timeout 10, Precision 6 | In1 ← SEN_PINT1.Out1, In2 ← SEN_PINT2.Out1, In3 ← SEN_PINT3.Out1 (new), In4 ← DIGIN1.Out1 (new) |
| SEN_PINT1 | Index 1 (`$SEN_PINT[1]`, Start) | Out1 → ETHERNET1.In1 |
| SEN_PINT2 | Index 2 (`$SEN_PINT[2]`, Result) | Out1 → ETHERNET1.In2 |
| SEN_PINT3 (new) | Index 3 (`$SEN_PINT[3]`, touch number) | Out1 → ETHERNET1.In3 |
| POSCORR1 | RefCorrSys World; large X, Y, Z and rotation limits (values set by the user in RSI Visual, not recorded) | CorrX..CorrC ← ETHERNET1.Out1..Out6 |
| DIGIN1 | first input 8, one bit (`$IN[8]`) | Out1 → OR1.In2 and → ETHERNET1.In4 (new) |
| OR1 | 2 inputs | In1 ← ETHERNET1.Out7 (PC `Stop`), In2 ← DIGIN1.Out1; Out1 → STOP1.In1 |
| STOP1 | Mode ExitMoveCorr | In1 ← OR1.Out1 |
| MAP2SEN_PINT1 | Index 2 | In1 ← ETHERNET1.Out8 (PC `Result`) |

STOP1 ends `RSI_MOVECORR` at the rising edge of OR1: `$IN[8]` switching on (the touch) or the PC's
Stop after the stream and the Result handshake (a missed touch). In `#RELATIVE` mode POSCORR1
applies each cycle's increment and compares only the summed correction with its limits, so with
limits far above the session's summed search travel it makes no difference whether the sum starts
from zero at every `RSI_ON` (user decision, 2026-11-11). The limits then bound nothing; one search
moves at most the 5 mm of its stream.

## Signals

ETHERNET1 exchanges these signals with the PC at 172.31.1.47:5555 (sensor type ImFree), over the
direct cable described in `RSI_Chamfer_Hole_CONTRACT.md`. The controller also sends RIst, AIPos
(the actual axis angles A1–A6, six decimals with Precision 6), MACur and Delay.

| Direction | Port | XML | Connection |
| --- | --- | --- | --- |
| Controller → PC | In1 | `Start`, BOOL | SEN_PINT1 reads `$SEN_PINT[1]` |
| Controller → PC | In2 | `Result`, LONG | SEN_PINT2 reads `$SEN_PINT[2]` |
| Controller → PC | In3 | `TouchIndex`, LONG | SEN_PINT3 reads `$SEN_PINT[3]` |
| Controller → PC | In4 | `Touch`, BOOL | DIGIN1 reads `$IN[8]` |
| PC → controller | Out1..6 | `RKorr.X/Y/Z/A/B/C`, DOUBLE, HOLDON 0 | POSCORR1 CorrX..CorrC |
| PC → controller | Out7 | `Stop`, BOOL, HOLDON 1 | OR1.In1 |
| PC → controller | Out8 | `Result`, LONG, HOLDON 1 | MAP2SEN_PINT1 writes `$SEN_PINT[2]` |

KRL clears `$SEN_PINT[1..3]` at the start of the program. Each search reads `$POS_ACT` (which
triggers an advance run stop, so the robot stands at the search start), ends the program if `$IN[8]`
is already TRUE, writes the touch number to `$SEN_PINT[3]`, calls `RSI_ON(#RELATIVE)`, waits
`WAIT SEC 0.012` with `$SEN_PINT[1]=0` (one 12 ms interpolation cycle, the wait resolution of
KSS 8.3), sets `$SEN_PINT[1]=1`, calls `RSI_MOVECORR()`, reads `$IN[8]`, clears `$SEN_PINT[1]` and
calls `RSI_OFF()`. Without the wait the controller sends no Start = 0 packet between `RSI_ON` and
`$SEN_PINT[1]=1` (`RSI_Probe_Check` capture, 2026-11-11). Value assignments and `IF` conditions run in
the advance run (KUKA Robot Programming 1, "Computer advance run"); the `$POS_ACT` read makes the
`$IN[8]` check independent of how far the advance run got with `$ADVANCE=0`.

## PC behavior

`RsiDevice` follows "PC behavior" of `RSI_Chamfer_Hole_CONTRACT.md` with a table of streams instead
of one stream:
- The table has 159 entries for this program: index 0 is empty, index t holds the search stream of
  touch t (1–158). The console reports "RSI correction streams installed: 159".
- `TouchIndex` is read as a decimal index; a missing element or `not linked:<n>` (an unconnected
  input, as in the other two programs) reads as 0; any other text makes the packet invalid.
- On the first Start = 1 packet after Idle the device plays `streams[TouchIndex]`; a missing or
  empty stream gives zero correction, Stop 1 and Result 0, as an empty table does.
- A Start = 1 packet whose `TouchIndex` differs from the running stream's index ends the run first,
  as a Start = 0 packet would, so a stream never continues into another touch.

## calibration_data.txt

RSI Sampling in the Calibration submode writes `C:\RoboCrapJobs\calibration_data.txt`, read by KRL
as `/RoboCrapJobs/calibration_data.txt` after `REMOUNT_SHARE()`. Version 1, CRLF rows, `;`
separators, six-decimal floats and decimal integer S/T:

1. `1`: format version.
2. ToolFrame: flange-to-TCP XYZABC, without S/T.
3. `$VEL.CP` in m/s: the Generate transfer feed (mm/s) / 1000.
4. `$ACC.CP` in m/s²: the Generate transfer acceleration (mm/s²) / 1000.
5. The step count M, 1–600 (485 for the current plan).
6. M steps, each a row `kind;touch` followed by:
   - kind 1 PTP and kind 2 LIN: one row `X;Y;Z;A;B;C;S;T`, the target;
   - kind 3 CIRC: the auxiliary point row, then the target row, both with the target's S/T;
   - kind 4 SEARCH: no further row; touch is the global touch number 1–158;
   - kind 5 HOME: no further row.

   touch is 0 for every kind except 4.

Every target is the saved animation TCP of the step's end point, reproduced within 0.0001 mm and
0.000001 in the rotation entries after six-decimal rounding; S/T come from its saved joints. The
CIRC auxiliary point lies on the start shell (sphere of radius contact distance + 3 mm) halfway
along the arc, with the target's orientation.

`READ_JOB()` requires version 1 and a step count 1–600; for every step a kind 1–5 and `#DATA_OK`,
and S in 0–7 / T in 0–63 for every POS row (`READ_POS`). Kind and touch are reset to 0 before each
row is read, so a value from the previous job cannot survive a partial conversion; a touch number
without a stream (0 or above 158) makes the PC answer Stop, which ends that search as missed. Every
input failure returns after HOME and before `RSI_CREATE`. The file is closed before `READ_JOB`
returns; the next run's `REMOUNT_SHARE` closes a file left open.

The PC deletes the file whenever it holds no calibration job: at application start, at every
Sampling request (chamfer, calibration or test LIN), when the calibration path is removed or
regenerated, and at shutdown. KRL then stops after HOME.

## KRL flow and failures

After HOME and `READ_JOB`, the program executes steps 1 to M, or 1 to `StepLimit` when the DAT value
`StepLimit` is between 1 and M−1 (set it on the smartPAD; 0 executes all steps):
- `$IN[8]` already TRUE at a search start (the rising-edge STOP could not detect the touch): the
  program ends without moving.
- A search that ends without `$IN[8]` (missed touch) or with a failed `RSI_ON`/`RSI_OFF`: LIN back to
  the search start recorded from `$POS_ACT`, then the program ends. The retract therefore does not
  depend on the job's next step.
- When `StepLimit` ends the loop right after a search, the program executes the same LIN back to
  the search start and ends.
- Neither end moves PTP near the sphere. The robot then stands about 3 mm from the sphere: jog it
  clear in T1 before the next start, because the program begins with `PTP HOME`.

The search steps of every touch number are listed in the plan file (`search_step`), for choosing
`StepLimit` (a search step plus 1 includes its retract). For the current plan: steps 1 and 2 are the
entry PTP and the descent, touch 1 searches at step 3, and each further touch adds transfer, search
and retract; `StepLimit` = 10 executes touches 1–3, `StepLimit` = 58 the whole first pass (touch 19
searches at step 57).

## Plan, packet and touch files

All beside the executable in `records/`, sharing the Sampling's UTC identifier `<id>`:
- `rsi_calibration_plan_<id>.csv` (written by Sampling): one row per touch number with its search
  step, purpose, group, repetition, touch within the pass, tilt, roll, normal, nominal start and
  contact poses and the S/T at the search start. A failure to write it fails the Sampling.
- `rsi_packets_<id>.csv` and `rsi_packets_<id>_partN.csv` (written by `RsiDevice`): the packet
  capture of `RSI_Chamfer_Hole_CONTRACT.md`. It is saved after every search, because the controller
  sends no packets between `RSI_OFF` and the next `RSI_ON`. It is the record of the touches.
- `rsi_touches_<id>.csv` (written by `python -I python/rsi_touches.py <records directory> <id>`):
  one row per search with a detection: `touch_index`, the IPOC and the cycle count since the search's
  first packet ((IPOC − first IPOC) / 4) of the first packet with `Touch` 1, its AIPos A1–A6 and the
  AIPos of the packet before it. The script reports searches without detection, touch numbers
  searched more than once and captures with dropped rows.

## Verified on the controller

Runs of 2026-11-11 with `StepLimit` = 10 (plan id `20261111_150029_843`, touches 1–3) and 58
(`20261111_152601_386`, the first pass G1, touches 1–19), user-authorized:
- The SRC and DAT compile and run, including the 600-element DAT arrays, `$POS_ACT` into E6POS,
  the PTP/LIN/CIRC steps with `$ORI_TYPE=#CONSTANT` and the `StepLimit` end with `LIN SearchStart`.
- All 22 searches stopped on `$IN[8]`; `Touch` arrives as 0/1 through DIGIN1.Out1, which also feeds
  OR1.
- Every search is preceded by exactly 3 Start = 0 packets (the 12 ms wait), and each search played
  its own stream.
- Detection 781–846 cycles after the search start; `python/rsi_touches.py` found every touch.

The context uses large POSCORR1 limits (user, 2026-11-11), so these runs do not show whether
POSCORR1 starts every search from zero; with those limits it does not matter.

## Not yet verified on the controller

- That `$VEL_AXIS`/`$ACC_AXIS` = 20 % are kept through the RSI calls for the PTP moves between
  passes (the first pass has no such PTP after a search).
- The 2 mm of search beyond the nominal contact are not checked for reachability by the animation.
- Braking distance after ExitMoveCorr at 1 mm/s.
- A missed search and its `LIN SearchStart` exit (no search missed so far).
