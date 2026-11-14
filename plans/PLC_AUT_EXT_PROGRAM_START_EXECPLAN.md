# Start one of four KRL programs from RoboCrap through the Delta PLC (KUKA Automatic External)

This ExecPlan is a living document. The sections `Progress`, `Surprises & Discoveries`, `Decision Log` and `Outcomes & Retrospective` must be kept up to date as work proceeds. It is maintained in accordance with `PLANS.md` at the repository root.

Five rules override `PLANS.md` for this plan. First, the implementing agent never runs a build, CMake configure, QML lint, the application or any test; when a milestone needs a build or a run, the agent stops, names exactly what the user must build or run, and continues from the user's reported result. Second, the agent never connects to the PLC or the robot, never changes controller or PLC settings, never starts a KRL program and never moves the robot: every ISPSoft, smartPAD, wiring and robot step is done by the user, and every step that can move the robot needs the user's explicit authorization for that step (`AGENTS.md`: "Live PLC/FTS/RSI connections, outputs, sensor bias or motion require explicit authorization"). Third, no git staging or commits unless the user asks. Fourth, no project tests or test harnesses (`AGENTS.md`). Fifth, every `AGENTS.md` rule applies to every C++ and QML line written for this plan, in particular "Mandatory C++ style", "Mandatory code reduction" and "Mandatory interface stability"; the compliance pass and the before/after line counts that `AGENTS.md` requires are recorded in this plan's `Outcomes & Retrospective`.

The design was worked out in `docs/plc_project_snapshot/PLC_AUT_EXT_proposal.md` (study, checked in) with the SFC drawing `docs/plc_project_snapshot/RobotHandler_SFC.drawio`. This plan repeats everything needed and supersedes the proposal where they differ; the proposal is background only. The calibration program used as program 1 is described in `resources/files/rsi/Calibration/RSI_Probe_Calibration_CONTRACT.md` and `plans/PROBE_SPHERE_RSI_CALIBRATION_EXECPLAN.md` (both checked in); this plan does not change it.


## Purpose / Big Picture

Today a KRL program on the KUKA KR C4 controller can only be started by the operator on the smartPAD (the KUKA teach pendant). The PC application RoboCrap talks to a Delta AS332T PLC over TCP, but the PLC program (the ISPSoft project `RoboCrapPLC`) has no working path to the robot: its `RobotHandler` sequence is outdated and the controller's "Automatic External" interface is not configured.

After this plan, with the KR C4 switched to the operating mode Automatic External (AUT EXT), the user selects program 1, 2, 3 or 4 in the RoboCrap PLC panel and presses Run. The PLC switches the drives on, acknowledges messages, starts the KUKA organization program `CELL.SRC` if needed and passes the program number; the robot executes the chosen program. Run is impossible unless the KR C4 is in AUT EXT: the panel disables the button, the PLC rejects the command, and the PLC never drives a KUKA input outside AUT EXT. The panel shows whether the program is running, finished or interrupted, and Resume continues an interrupted program. Program 1 is the existing probe sphere calibration `RSI_Probe_Calibration` (already run by the user in T1); programs 2–4 are placeholders inside `CELL.SRC` that only wait 1, 2 or 3 seconds, so the handshake can be demonstrated without moving the robot. AUT EXT has no 250 mm/s cap as T1 has, so `CELL.SRC` fixes the program override at 10 % for everything it runs and no program motion is programmed at 100 %: the robot never jumps to full speed when started from RoboCrap.


## Progress

- [x] (2026-11-12) Design agreed with the user (proposal and SFC drawing, now in `docs/plc_project_snapshot`).
- [x] (2026-11-13) Milestone 1 (user): ISPSoft project `RoboCrapPLC` backed up; KR C4 archive taken, `R1/cell.src`, `R1/TP/p00.src`, `R1/TP/p00.dat`, `R1/Mada/$machine.dat`, `STEU/Mada/$machine.dat`, `STEU/Mada/$option.dat` copied to `resources/files/KR C4/`; live AUT EXT table recorded in `docs/plc_project_snapshot/AUT_EXT_DIOs_live.md`; no PLC–EtherCAT signal other than the probe (Y1.7 → `$IN[8]`) is wired. P00 read by the agent: program number 0 is accepted (see Decision Log).
- [x] (2026-11-13) Milestone 2, agent part: `resources/files/AutExt/CELL.src` written (program override 10 % via `AutExtOv`, PTP HOME at 20 %, CASE 0 … 3); `RSI_Probe_Calibration.src` first `PTP HOME` lowered from 100 % to 20 % and its contract updated.
- [x] (2026-11-13) Milestone 2, user part (completed: wiring including the spare EL2809 output 2 → X1.1, which stays unassigned; AUT EXT values set; `CELL.src` and `RSI_Probe_Calibration.src` installed; forced Y1.0 … Y1.4 appear on `$IN[1]` … `$IN[5]`; after the BCO run of CELL in T1 `$OUT[3]` `$PERI_RDY`, `$OUT[4]` `$STOPMESS` and `$OUT[8]` `$IN_HOME` are TRUE, all other `$OUT[1..8]` FALSE; POV shows 10 % after the BCO run, so the installed CELL sets the override; ISPSoft online monitoring shows the same states on X1.0 … X1.7, so the EL2809 → X1 wiring works). Milestone 2 accepted.
- [x] (2026-11-13) Milestone 3, ISPSoft edits: all changes entered in `RoboCrapPLC`, compile without errors after three corrections (`CmdRun`, `CmdResume`, `Fail` declared as BOOL instead of WORD; `LastStep` initial value 0 instead of 16#FFFF; magic check rewritten with XOR). `ProbeMap` changed by the user: it now only copies X2.0 to Y1.7 (no `LED_GREEN` coil). Snapshot `docs/plc_project_snapshot/ISPSoft project.md` rewritten to this state.
- [x] (2026-11-13) Milestone 3, download and checks: program downloaded; the old application connects and switches Y2 outputs. Its module 1 outputs are disabled in the panel (Y1.0 … Y1.6 unplugged, Y1.7 display-only), so the PLC's module 1 WRITE_IO refusal was not exercised. The forced `CmdRun` check was skipped by the user, so the PLC-side AUT EXT gate (outputs gated by `AUTEXT_ACT`, result AUTEXT_LOST) is not yet verified; Milestone 5 stage A must check that Y1.0 … Y1.4 stay OFF while the KR C4 is not in AUT EXT. Milestone 3 accepted on these terms.
- [x] (2026-11-13) Milestone 4, agent part: `PlcMessageManager` (new `VAR_TYPE` codes, `robotProgramRequest`, 4-byte `CELL_STATE` published as `robotStep`/`robotProgram`/`robotResult`, unused `CELL` enum removed), `PlcDevice` (request queue with one request in flight, 1 s reply timeout, 2 s SNAPSHOT heartbeat, `runRobotProgram`, `resumeRobot`, `abortRobotStart`, `robotProgramState` from X1, robot keys reset on disconnect), `DeltaPanel.qml` (module 1 KUKA tags, Y1.0 … Y1.4 and Y1.7 display-only), new `RobotStartPanel.qml` registered in `src/application/CMakeLists.txt`. Not built.
- [x] (2026-11-13) Milestone 4, user part: built and run in T1; the panel shows the KUKA tags (X1.2 PERI_RDY, X1.3 STOPMESS, X1.7 IN_HOME ON; X1.0 AUT EXT OFF), Run/Resume/Abort disabled, status "AUT EXT off"; three Disconnect/Connect cycles succeeded; the connection stayed up for an idle minute without reply timeouts. The heartbeat logged `[WRITE] … TX: AA 55 01 A0 xx 02 BA 00` every 2 s; SNAPSHOT requests are no longer logged since (one-line change in `PlcDevice::sendNext`, not yet rebuilt). Milestone 4 accepted.
- [x] (2026-11-13) PLC log for the tests: `PlcDevice` writes `records/plc_io_<yyyyMMdd_HHmmss>.csv` per connection (user request: no manual monitoring of fast signals). Not yet built.
- [x] (2026-11-13) Milestone 5 preparation: `RobotIo` correction and the `PCHandler`/`FB_CmdMonitor` socket fix entered and downloaded; requests are answered again, Y2 switches work, X1.0 follows AUT EXT, Disconnect/Connect works. Remaining anomaly: the first request after every connect (the connect SNAPSHOT) timed out (`PLC reply timeout for transaction 2`, later 43) while all later requests were answered; `PlcDevice::onConnected` no longer sends it (not yet rebuilt).
- [x] (2026-11-13) Repository CELL installed on the controller with the user's WorkVisual project; stage A A1 and A2 passed (log `plc_io_20261113_202709.csv`); A3 was done with the EMERGENCY STOP instead of the STOP key and must be repeated.
- [x] (2026-11-13) Stage A fixes, agent part: the start-sequence state (step, program, result) travels in the IO change frame and in the SNAPSHOT reply, `FB_VarMonitor` and its frame are removed (PC: `PlcMessageManager`, `PlcDevice::publishIo`; PLC text: "FB_IOMonitor body", "FB_RunSnapShot reply", "Stage A fix in ISPSoft"); `CELL.src` clears APPL_RUN in its INIT fold. Not installed, entered or built.
- [x] (2026-11-14) Stage A fixes installed, entered and built; A3 repeated with the STOP key (log `plc_io_20261114_134646.csv`): APPL_RUN stays ON at STOP, Resume ends with TIMEOUT unless the operator first acknowledges "KSS01370 Passive STOP" on the smartPAD, then RESUMED and "finished" (Surprises). Open: the Resume rule for a pending stop message (user decision), then A4, A5.
- [x] (2026-11-14) Final stage A session (log `plc_io_20261114_163255.csv`): A1–A4 passed; A5 (a) and (b) passed, A5 (c) failed (the switch to T1 took longer than the 5 s of ST_PREP), A6 not done. Placeholders changed to `WAIT SEC 5` on the user's request (`CELL.src`, not yet transferred). Next: A5 (c) and A6 alone.
- [x] (2026-11-14) A5 (c) passed (log `plc_io_20261114_165134.csv`): EMERGENCY STOP pressed (STOPMESS = 1 at 16:51:43.975, PERI_RDY = 0 at 44.027); run 2 at 16:52:46.299, PREP 46.525 with the CONF_MESS pulse 46.625–46.824, X1.0 fell at 49.475 (3.2 s after Run, still in ST_PREP) and the same row carries result AUT_EXT_LOST; step IDLE logged at 50.949 (lost frame, repaired by the heartbeat); EXT_START, DRIVES_ON (as far as logged) and the PGNO bits stayed 0. The PLC-side AUT EXT gate is verified. Remaining in stage A: A6.
- [x] (2026-11-14) A6 passed with result 1 (log `plc_io_20261114_170121.csv`, CELL with `WAIT SEC 5` installed): the PLC's `$CONF_MESS` acknowledges the EMERGENCY STOP message (Surprises). Stage A complete; stage B (calibration with motion) needs its own authorization.
- [x] (2026-11-14) Speed scheme changed after the first stage B attempt (LIN at 1 mm/s with POV 10 %): `$OV_PRO` = 100 in AUT EXT, mode-dependent PTP velocity `INIT_PTP_VEL` (new module), Resume blocked after an operating-mode change (PLC `ResumeBlocked`, PC `robotResumeBlocked`). Files: `CELL.src`, `INIT_PTP_VEL.src/.dat`, `RSI_Probe_Calibration.src`, `plcdevice.h/.cpp`, `RobotStartPanel.qml`, PLC `RobotIo`/`FB_RunSetVar`/Main Table (snapshot). Not transferred, entered or built.
- [x] (2026-11-14) Speed scheme installed (`CELL.src`, `INIT_PTP_VEL` with loop variable `I` because `AXIS` is a KRL data type, `RSI_Probe_Calibration.src`, PLC, RoboCrap) and checked by the user (log `plc_io_20261114_190917.csv`): BCO in T1 at T1 speed; program 2 in AUT EXT STARTED at 19:10:22.563, finished 27.556 (5.0 s), POV 100 % on the smartPAD; program 4 stopped with the STOP key at 19:11:24.165, acknowledged 33.062, T1 at 46.868 and AUT EXT again at 52.725: Resume disabled with "Program interrupted · mode changed · restart from CELL".
- [x] (2026-11-14) Stage B run with `StepLimit` = 58 (the user's decision; the whole first pass, touches 1–19), plan id `20261114_161345_844`, PLC log `plc_io_20261114_191324.csv`: run 1 at 19:14:19.206, PGNO bits 0, STARTED 19:14:20.205 (APPL_RUN), IN_HOME OFF 20.831, Y1.7 (probe copy) pulsing for each touch from 19:14:39 to 19:16:22, program finished (APPL_RUN 0, PGNO_REQ 1) at 19:16:23.079, i.e. 2 min 3 s for 58 steps at PTP 5 % and 10 mm/s. `python/rsi_touches.py`: 19 captures, 19 searches, 19 touches, touch numbers 1–19 once each, detection 778–844 cycles after the search start (T1 runs of 2026-11-11: 781–846). With the override at 100 % the search behaves as in T1. Afterwards the user switched to T1 (19:16:49.870) and returned the robot HOME (IN_HOME 19:18:08.409); the smartPAD POV in T1 read 10 %.
- [x] (2026-11-14) Stage B full session, first attempt (`StepLimit` = 0, plan id `20261114_163247_675`, PLC log `plc_io_20261114_193238.csv`) stopped at search 93 of 158 with KSS29002 "Object ETHERNET1 returns error RSITimeout" and KSS00489 "STOP by $CORRECTION function" (Surprises); touches 1–92 recorded (detection 781–870 cycles). Repeated successfully after the PC power settings were fixed (next entries).
- [x] (2026-11-14) Stage B complete: full session (`StepLimit` = 0) after the PC power settings were changed (display off and sleep Never, USB selective suspend Disabled, the RSI adapter's "Allow the computer to turn off this device" cleared and its Energy-Efficient Ethernet Disabled, read back on the PC; the adapter's own SelectiveSuspend stayed Enabled), plan id `20261114_170013_165`, PLC log `plc_io_20261114_195852.csv`. A first Run at 19:59:10.452 ended 0.3 s after STARTED (APPL_RUN 19:59:11.439, PGNO_REQ again 11.742) because no job file existed yet (RSI Sampling followed at 20:00:13; `RSI_Probe_Calibration` returns after HOME without a job, by design). Second Run at 20:00:40.875: STARTED 20:00:41.535, IN_HOME OFF 42.142, back at HOME and finished at 20:17:22.825/22.962 (16 min 41 s). `python/rsi_touches.py`: 158 searches, 158 touches, numbers 1–158 once each, detection 770–863 cycles; largest PC receive gap 10.9 ms (2 of 158 captures above 8 ms), largest reply latency 0.81 ms. Remaining: Milestone 6.
- [x] (2026-11-14) Stage B authorized by the user ("ok, let's start Stage B") for the first run, program 1 with `StepLimit` = 10 (touches 1–3); `StepLimit` = 58 and 0 need the user's further decision.
- [x] (2026-11-14) Resume requires `$STOPMESS` FALSE (user decision): `RobotStartPanel.qml` (Resume rule, two status texts) and `FB_RunSetVar` (snapshot text). Not built, not entered in ISPSoft.
- [x] (2026-11-14) Milestone 5 (user, authorized 2026-11-13 for stage A and 2026-11-14 for stage B; completed, see the entries above): first the `RobotIo` correction (PGNO already in ST_START), then stage A without motion (programs 2–4, stopped CELL, interruption, Resume, Abort, timeout, AUT EXT gate); stage B with motion (program 1 = calibration with `StepLimit`) needs its own authorization.
- [x] (2026-11-13) Milestone 6, hardware part done early because the hardware changed in Milestone 2: `docs/context/HARDWARE_SETUP.md` "KR C4 Automatic External configuration" (new values, CELL, override) and `docs/context/ARCHITECTURE.md` §1 (EtherCAT row, PLC terminal table, `$IN`/`$OUT` wiring, AUT EXT sentence).
- [x] (2026-11-14) Milestone 6: `docs/context/HARDWARE_SETUP.md` (AUT EXT table checked against `AUT_EXT_DIOs_live.md`, the record's path, CELL with override 100 % and `INIT_PTP_VEL`, Run/Resume conditions, Passive STOP, new section "Power settings for RSI"), `docs/context/ARCHITECTURE.md` (§1 PLC drives Y1.0 … Y1.4 and the start sequence, §3.4 `PlcDevice`, §3.6 `RobotStartPanel`, §5.3 queued request, §7 "PLC pending lifetime" replaced by "PLC change frames"), `docs/context/AGENT_CONTRACTS.md` (Devices and protocols: PLC constraints), `RSI_Probe_Calibration_CONTRACT.md` ("AUT EXT start"), `docs/plc_project_snapshot/ISPSoft project.md` (header with the changes after Milestone 3; the bodies were already current). KRL sources in `resources/files/AutExt` (`cell.src`, `INIT_PTP_VEL.src/.dat`) and `resources/files/rsi/Calibration` (`RSI_Probe_Calibration.src`; `.dat` unchanged, `StepLimit` 0) are the installed versions.


## Surprises & Discoveries

- Observation: the live AUT EXT configuration (2026-11-13) equals the one in `docs/context/HARDWARE_SETUP.md`: `$I_O_ACT`, `$EXT_START`, `$CONF_MESS`, `$DRIVES_ON` = 1026 (interface inactive), `$MOVE_ENABLE`, `$DRIVES_OFF` = 1025, PGNO_TYPE 1, PGNO_LENGTH 2, PGNO_FBIT 4001, PGNO_PARITY 0, PGNO_VALID 0, REFLECT_PROG_NR 0; outputs on internal numbers (`$RC_RDY1` 137, `$PERI_RDY` 143, `$I_O_ACTCONF` 140, `$STOPMESS` 144, `$PRO_ACT` 141, PGNO_REQ 33, APPL_RUN 142, `$IN_HOME` 1000, `$ON_PATH` 997).
  Evidence: `docs/plc_project_snapshot/AUT_EXT_DIOs_live.md`; the archived `STEU/Mada/$machine.dat` has the same SIGNAL lines (git diff against the 17 Oct copy, whose old map `$EXT_START $IN[3]`, `$DRIVES_OFF $IN[5]` … is no longer active).
- Observation: `$CHCK_MOVENA` is FALSE, so `$MOVE_ENABLE = $IN[1025]` does not block external start.
  Evidence: `resources/files/KR C4/STEU/Mada/$option.dat` line 17 `BOOL $CHCK_MOVENA=FALSE` (archive of 2026-11-13).
- Observation: P00 (`resources/files/KR C4/R1/TP/p00.src`, `EXT_PGNO`, case `#PGNO_GET` with PGNO_TYPE 1 and PGNO_VALID 0) first waits `WAIT FOR $EXT_START==FALSE`, then sets PGNO_REQ TRUE, then polls until `$EXT_START==TRUE` (a level, not an edge), then sums the PGNO input bits into `PGNO` without any check for 0. `#PGNO_ACKN` sets PGNO_REQ FALSE and APPL_RUN TRUE. APPL_RUN is set FALSE only at the start of the next `EXT_PGNO` call (CELL's next `#PGNO_GET`) and in `INIT_EXT`. `#PGNO_FAULT` shows "ProgramNotAvailable", sets PGNO_REQ FALSE and waits 0.5 s. `INIT_EXT` loops with "OperatorModeWrong" while `$I_O_ACTCONF` is FALSE. `CHK_HOME` stops CELL with "MoveHomeInT1" and HALT if the robot is not at HOME in AUT EXT.
  Consequence: APPL_RUN is an ordinary KRL output; when the KR C4 stops a program (STOP key, E-STOP), APPL_RUN most likely stays TRUE while `$PRO_ACT` falls. KSS Fig. 6-25 … 6-28 draw APPL_RUN falling, which this P00 does not do. Milestone 5 stage A checks which is true; the PC state logic below works for both, the PLC Resume guard assumes the P00 behaviour.
- Observation: the archived `R1/cell.src` already contains a user test: `CASE 1` calls `test_aut_ext()`, `CASE 2` and `CASE 3` are empty examples. It uses `GLOBAL INTERRUPT DECL 3 WHEN $STOPMESS==TRUE DO IR_STOPM()`.
- Observation: the calibration program `RSI_Probe_Calibration` starts with `PTP HOME Vel= 100 % DEFAULT`, and when `StepLimit` (a DAT value) ends the run after a search, or a search is missed, it ends with `LIN SearchStart` about 3 mm from the sphere, not at HOME; its contract requires jogging clear in T1 before the next start.
  Evidence: `resources/files/rsi/Calibration/RSI_Probe_Calibration.src` lines 17, 57–69; `RSI_Probe_Calibration_CONTRACT.md` "KRL flow and failures".
- Observation: in AUT EXT the controller moves at programmed velocity (KSS 8.3 SI §6.17.2.1, §8.11); the 250 mm/s limit applies only to T1. Before this plan two motions were programmed at 100 % axis velocity: CELL's `PTP HOME` (`BAS (#VEL_PTP,100)`) and the first `PTP HOME Vel= 100 %` of `RSI_Probe_Calibration`. KSS forbids only the submit interpreter or the PLC to write `$OV_PRO` (because it would also act in T1/T2); a robot program may set it. The RSI manual (KST RSI 3.3) does not state whether the program override scales RSI_MOVECORR corrections.
  Evidence: `manuals/KUKA/KUKA KSS 8.3 … SI [ENG].pdf` §2 safety measures for submit/PLC ("In T1 and T2, the system variable $OV_PRO must not be written to by the submit interpreter or the PLC"); RSI 3.3 §2.4.3 and §7.1.6 (no mention of override).
- Observation: on the controller `RSI_Probe_Calibration` lies in `KRC:\R1\Program\RSI` (`&PARAM DISKPATH` of the controller's file), while CELL lies in `R1`. If CELL reports the module as unknown, the `EXT RSI_Probe_Calibration ( )` line described under `Artifacts and Notes` is needed.
  Evidence: header of the controller version of `RSI_Probe_Calibration.src` (2026-11-13), now the repository file.
- Observation: after the BCO run of CELL in T1, `$STOPMESS` (`$OUT[4]`) is TRUE while the drives are on (`$PERI_RDY` TRUE); `$I_O_ACTCONF`, `$PRO_ACT`, PGNO_REQ and APPL_RUN are FALSE, as expected in T1 after the BCO stop (AUT EXT not selected, interpreter stopped, `INIT_EXT` not yet run). Which message holds `$STOPMESS` was not recorded. In AUT EXT, ST_PREP's `$CONF_MESS` pulse acknowledges acknowledgeable messages; a message that cannot be acknowledged keeps `$STOPMESS` TRUE, and the start then ends with TIMEOUT after 5 s (`TR_Prepared` requires `$STOPMESS` FALSE).
  Evidence: user's I/O readout on the smartPAD, 2026-11-13.
- Observation: ISPSoft rejects WORD constants above 16#7FFF in some places: 16#FFFF as a WORD initial value ("out of datatype range"), and `IF RxHeader.magic <> 16#AA55 THEN` in `FB_CmdMonitor` ("Error Code : 1003, Command test error, Error Command : LD", 2026-11-13); the existing `wTxWord(idx:=0, val:=16#AA55)` compiles. The comparison is rewritten as `(RxHeader.magic XOR 16#AA55) <> 0`. Avoid comparisons with constants above 16#7FFF.
  Evidence: user's ISPSoft compile output, 2026-11-13.
- Observation: `$STOPMESS` follows operator safety as KSS describes `$USER_SAF`: in T1 it is TRUE with the enabling switch released and FALSE while it is held; in AUT EXT with nothing started it is FALSE (`$USER_SAF`, `$ALARM_STOP`, `$PERI_RDY` TRUE, no message). The smartPAD variable `$STOPMESS`, the digital output `$OUT[4]` and PLC input X1.3 agree in all three states, so there is no inversion between controller, wiring and PLC. The TRUE value seen in T1 after the BCO run (Milestone 2) was the released enabling switch. Nothing blocks `TR_Prepared` in AUT EXT.
  Evidence: user's three-source table, 2026-11-13.
- Observation: P00 `#PGNO_GET` takes the program number whenever it sees `$EXT_START` TRUE in its wait loop, including when the interpreter was stopped inside that loop and is continued by `$EXT_START`. This made the first `RobotIo` version unsafe for a CELL stopped while waiting for a number (Decision Log, 2026-11-13).
  Evidence: `resources/files/KR C4/R1/TP/p00.src` lines 176–220 (`WAIT FOR $EXT_START==FALSE`, PGNO_REQ TRUE, `REPEAT … UNTIL $EXT_START==TRUE`, then the bits are summed).
- Observation: after the `RobotIo` correction was downloaded, the PLC answered no request (every request timed out, Y2 switches had no effect). ISPSoft monitor with RoboCrap connected: SM1270 TCP_CONNECTED TRUE, SM1271 TRUE, SM1275 TCP_SENDING FALSE, SM1272 TCP_SENT FALSE, SM1277 TCP_ERROR TRUE, `WANTTOSEND` TRUE, `SendStarted` FALSE, `TxBusy` TRUE, `Rx[0]` = 16#001E (30 bytes waiting), `CmdMonitor.Pending` FALSE, SR180 0. The socket error flag stayed ON; `PCHandler` (SSEND only with NOT TCP_ERROR) and `FB_CmdMonitor` (requests only with NOT TCP_ERROR) never clear it, so the first frame stayed unsent and no request was taken. The original project had the same dependency. Which event set the flag (the download while connected, or the RST RoboCrap sends on Disconnect) is not known. A first fix that cleared TCP_ERROR in the program failed: the flag came back ON at once. Reason, from the AS Programming Manual §2: SM1272 (data sent) and SM1277 (error flag) have the attribute R (read-only for the program), SM1277 is latched, and SM1270 … SM1353 are refreshed only when a socket instruction (SOPEN/SSEND/SCLOSE) is executed. With SSEND conditioned on NOT TCP_ERROR and SOPEN only while disconnected, no socket instruction ever ran again, so the flag could never change. Fix: SSEND no longer waits for TCP_ERROR (its execution refreshes the flags), the completion check runs before a new SSEND and requires TCP_SENDING OFF, the write to the read-only TCP_SENT is removed, and `FB_CmdMonitor` takes requests without the TCP_ERROR condition.
  Evidence: user's monitor values, 2026-11-13.
- Observation: after the socket fix, the request RoboCrap sent immediately at ConnectedState was never answered, at every connect, while all later requests were. Not explained by the manuals read; a plausible cause is that the PLC drops data that arrives before its program sees SM1270. The PLC already sends its IO and CELL_STATE frames on a new connection (connection edges in `FB_IOMonitor`/`FB_VarMonitor`), so RoboCrap no longer sends a SNAPSHOT on connect; the first heartbeat follows after 2 s.
  Evidence: user's console log, 2026-11-13 (`PLC reply timeout for transaction 2` 1 s after connect, then normal WRITE_IO traffic).
- Observation: first stage A run (program 3, log `records/plc_io_20261113_194053.csv`): run 3 at 19:42:26.442; CONF_MESS 26.770; PGNO1 = 1, PGNO0 = 0 at 26.971 (ST_START); EXT_START 27.072; PRO_ACT 27.112 (CELL started); PGNO_REQ = 1 at 27.307; EXT_START again 27.416 (ST_SELECT); PGNO_REQ = 0 at 28.166 (P00 acknowledged a number) but APPL_RUN (X1.6) never 1; result TIMEOUT at 29.316 (ST_SELECT 2 s limit); EXT_START and PGNO bits 0 and PGNO_REQ = 1 again at 29.344, about 1.2 s after the acknowledgement; no later CELL_STATE frame with step IDLE, while the panel still showed "Step SELECT". The smartPAD variable `PGNO` reads 2 afterwards, so CELL read the right number and ran `CASE 2` (`WAIT SEC 2`); the 1.2 s gap between the logged PGNO_REQ fall and rise is a PC receive-time artefact (the fall was logged late), not a shorter branch. P00 sets APPL_RUN together with the acknowledgement, so APPL_RUN did not reach X1.6 (EL2809 output 7 → X1.6 was never seen TRUE before). The smartPAD showed P00's state message "Wait for $EXT_START=True" while CELL waited for the number (normal). Under investigation; stage A stopped.
  Evidence: the log rows quoted.
- Observation: the cause of the failed first start is the controller's CELL.SRC, which is not `resources/files/AutExt/CELL.src`: it has `CASE 1` = `RSI_Probe_Calibration`, `CASE 2` and `CASE 3` with the acknowledgement only, no `CASE 0`, and `$OV_PRO=AutExtOv` only before the first motion (not in the LOOP). Program 3 (number 2) therefore ran an empty `CASE 2`: APPL_RUN was ON only for an instant and PGNO_REQ rose again as soon as EXT_START fell. With that CELL, RoboCrap's program 2 (number 1) starts the calibration and program 1 (number 0) ends in DEFAULT. The Milestone 2 POV check passed because that CELL also sets 10 % before its first motion. The controller's CELL needs `EXT RSI_Probe_Calibration ( )`; that line is now in the repository file. Variables `APPL_RUN` = 7 and `PGNO_REQ` = 6 on the controller; `$OUT[7]` set by hand in T1 reaches X1.6 (log `plc_io_20261113_195627.csv`, 19:59:32–36).
  Evidence: CELL.SRC text read on the smartPAD by the user, 2026-11-13; smartPAD variable display.
- Observation: after a reconnect the PLC sent no `CELL_STATE` frame (log `plc_io_20261113_200041.csv` has only one io row), although `FB_VarMonitor` should send one on every new connection; also no frame with step IDLE followed the TIMEOUT frame of the failed start. Unexplained so far; RoboCrap now also writes unreadable frames (`read error <code> frame <hex>`) and unmatched replies to the CSV log, so the next log shows whether the frames arrive but are rejected.
  Evidence: logs `plc_io_20261113_194053.csv` and `plc_io_20261113_200041.csv`.
- Observation: stage A after installing the repository CELL (log `plc_io_20261113_202709.csv`): A1 program 3 passed (EXT_START, PRO_ACT, PGNO_REQ, second EXT_START, APPL_RUN 14.100–16.088 = 1.99 s); A2 program 2 (APPL_RUN 0.99 s) and program 4 (2.99 s) passed. Not one `CELL_STATE` frame arrived during the whole session (no `state` row; the panel shows "last start -"), although the PLC's step and result changed with every start; `FB_VarMonitor` frames do not reach RoboCrap. A3: the user pressed the EMERGENCY STOP button instead of the STOP key; the log shows STOPMESS = 1 with APPL_RUN still 1 (20:29:24.757), then PERI_RDY, PRO_ACT and APPL_RUN = 0 (24.808). The panel then showed "program interrupted" with Resume disabled, as the PLC guard (APPL_RUN AND NOT PRO_ACT) requires. The conclusion drawn at first, that an E-STOP clears APPL_RUN, is withdrawn by the next observation: APPL_RUN was ON again afterwards.
  Evidence: the log rows quoted; panel screenshot 2026-11-13.
- Observation: after the E-STOP of A3, APPL_RUN (X1.6) is ON with `$PRO_ACT` OFF and stays ON in T1 and after the stopped program was cancelled and deselected (logs `plc_io_20261113_203809.csv`, `…_203931.csv`, `…_204056.csv`), so Run stays disabled. P00 clears APPL_RUN only in `INIT_EXT` and at CELL's next number request; neither runs in T1, because CELL's BCO run stops at `PTP XHOME` before `INIT_EXT`. Resume at 20:38:29.297 (STOPMESS = 1): CONF_MESS pulse 29.625–29.823, STOPMESS stayed 1, ST_PREP ended with TIMEOUT at 34.323 (5 s), STOPMESS fell only at 35.674. The frame with step IDLE after that TIMEOUT never arrived: the panel kept "PREP", and Abort at 37.940 was refused with `cmd=188 code=60939` (SET_VAR, 0xEE0B NOT_ALLOWED: the PLC was already idle). In this session one CELL_STATE frame did arrive (20:38:34.323), so frames are lost, not never sent. Likely cause (not verified): a frame built in the scan right after another one is lost, because `PCHandler` takes the send as complete from socket flags that may still show the previous send; the late PGNO_REQ fall of the first run (observation above) fits a lost IO frame repaired by the next 2 s heartbeat.
  Evidence: the log rows quoted; user report "APPL_RUN now always ON, even in T1, even if program not selected", 2026-11-13.
- Observation: A3 with the STOP key (log `plc_io_20261114_134646.csv`). Program 4 started 13:48:20.678; STOP at 21.578: STOPMESS = 1, PRO_ACT = 0, APPL_RUN stays 1, so the P00 expectation holds and Resume is enabled. The smartPAD shows the acknowledgeable message "KSS01370 Passive STOP" (user). Resume at 13:48:23.746, 32.334, 13:51:03.680, 34.271, 13:54:15.900: in each, ST_PREP raises CONF_MESS (Y1.2) for 200 ms from 0.3 s on, STOPMESS stays 1, and ST_PREP ends with TIMEOUT after 5 s. After the user acknowledged the message on the smartPAD (STOPMESS = 0 at 13:51:56.930 and 13:55:44.000), Resume worked: 13:51:59.064 → EXT_START 59.695 → PRO_ACT 59.727, result RESUMED, PGNO_REQ = 1 (finished) at 13:52:01.552; 13:55:46.892 → RESUMED 47.560, finished 49.600. No CONF_MESS edge of this plan has cleared STOPMESS so far (also the E-STOP case of 2026-11-13, 20:38:29). Milestone 2 showed the forced Y1.2 on `$IN[3]`, and `$CONF_MESS` = 3 is configured, so the edge reaches the controller; the manuals read say only that `$CONF_MESS` acknowledges messages "as soon as the cause of the error has been eliminated" and do not say whether this message can be acknowledged externally. Also: the CELL APPL_RUN reset works (APPL_RUN = 0 during the BCO run at 13:49:34.862); step/program/result arrive in every IO frame; the 200 ms DRIVES_ON pulse appears in only 2 of 16 PREP sequences, so the IO frame sent right after the SET_VAR reply is still usually lost (the next frame repairs the state).
  Evidence: the log rows quoted; user report 2026-11-14.
- Observation: clean A3 (log `plc_io_20261114_152647.csv`, one action at a time). E1 program 3: run 15:27:15.441, PRO_ACT 16.117, APPL_RUN 16.447 STARTED, finished 18.444 (2.0 s). E2 first try (STOP too late): run 4 at 15:28:06.975, SELECT 07.506, EXT_START 07.607, then the next row is APPL_RUN = 1 at 09.461 and finished at 10.622; the APPL_RUN rise (about 07.62, 3 s before the finish) and the IDLE/STARTED frame were lost and repaired by the heartbeat SNAPSHOT. E2 repeated: run 4 at 15:28:16.288, STARTED 16.947, STOP key at 17.774: STOPMESS = 1, PRO_ACT = 0, APPL_RUN = 1, smartPAD message "KSS01370 Passive STOP". E3 Resume 15:29:00.513: PREP 00.738, CONF_MESS 00.839–01.037, STOPMESS stays 1, TIMEOUT 05.537. E4 acknowledgement on the smartPAD: STOPMESS = 0 at 15:30:26.090. E5 Resume 15:30:28.351: PREP 28.575, CONF_MESS 28.676–28.878, START 28.878, EXT_START 28.982, PRO_ACT 29.019 RESUMED, finished (APPL_RUN = 0, PGNO_REQ = 1) 31.170, i.e. 2.15 s of program 4's remaining wait. Result: the PLC's `$CONF_MESS` edge does not clear "KSS01370 Passive STOP"; acknowledgement on the smartPAD does, and Resume then works.
  Comparison with KSS 8.3 SI §6.17.4: Fig. 6-28 ("Restart after user STOP", a programmed stop) has no `$STOPMESS` and no `$CONF_MESS`: restart is only the `$EXT_START` edge. Figs. 6-25, 6-26, 6-27 (operator safety, EMERGENCY STOP, motion enable) show `$STOPMESS` TRUE; the cause disappears first (`$USER_SAF`, `$ALARM_STOP` or `$MOVE_ENABLE` TRUE again), then `$DRIVES_ON` until `$PERI_RDY` (6-25, 6-26), then the `$CONF_MESS` rising edge, `$STOPMESS` FALSE, `$CONF_MESS` FALSE, then `$EXT_START`. All four figures draw APPL_RUN falling at the stop, which this P00 does not do. The smartPAD STOP key in AUT EXT (SI §3.4: STOP 2) is not drawn; on this controller it sets `$STOPMESS` with an acknowledgeable message, and the manuals read do not say whether `$CONF_MESS` can acknowledge it. Whether `$CONF_MESS` acknowledges anything at all on this controller is not yet shown: no logged `$CONF_MESS` edge has cleared `$STOPMESS` so far, and the only EMERGENCY STOP case (2026-11-13, 20:38) is ambiguous.
  Evidence: the log rows quoted; manual figures in `manuals/KUKA/6-25.png` … `6-28.png`.
- Observation (web search, 2026-11-14; forum and third-party sources, not KUKA manuals of KSS 8.3): which messages `$CONF_MESS` may acknowledge is configured per message. The KR C4 file `C:\KRC\Roboter\Config\User\Common\KrcExtConfMsg.xml` (template header "KUKA V8.3", schema `/Roboter/Config/System/Common/Schemes/KrcExtConfMsg.xsd`) takes entries `<Message Number="xyz" AllowConfInExt="true" />`, "only confirmation messages are allowed to configure"; one post used it on KSS 8.7 for message 02858, one source names a cold start with reload of files after editing. The older equivalent was `INIT\ext_conf.ini` (`M_1234 = TRUE`, read at cold start, KSS 4.1 and from 5.3). An old KUKA message list (KSS 5.x, message 209 "Passive STOP", seen only as a search summary, the PDF host was blocked) gives the remedy as acknowledgement only on the originating unit; a KRC2 forum thread (SW 5.6.8, 2011) reports the same symptom as here, `$CONF_MESS` from the PLC not clearing "passiver Stop" after the KCP STOP key. Pulse length: only the rising edge counts (SI §6.17.2.1); a KR C1 guide asks for at least 20 ms; this plan's 200 ms pulse, 300 ms after entering ST_PREP with `$PERI_RDY` already TRUE, meets both, so pulse timing is not the cause. Not yet checked on this controller: whether `KrcExtConfMsg.xsd` and `KrcExtConfMsg.xml` exist and whether `Number="01370"` makes KSS01370 acknowledgeable.
  Evidence: robot-forum.com thread 42643, roboterforum.de threads 9893, 14283, 1649; KSS 8.3 SI §6.17.2.1.
- Observation: RSITimeout in the full session (plan `20261114_163247_675`). Run 1 at 19:33:15.038; STOPMESS = 1 at 19:43:11.192, 9 min 56 s later, during search 93. Capture `rsi_packets_20261114_163247_675_part93.csv`: RoboCrap answered every packet within 0.32 ms until IPOC 24547493; the next packet (IPOC 24547497) was received 171.6 ms later, and IPOC 24547497 … 24547537 (11 packets, 4 ms apart on the controller, `Delay` 0 … 10) arrived within 0.33 ms; the RSI thread's event loop was asleep (waiting for the socket) from 1651.93 to 1822.82 ms of the capture, so the packets were held below RoboCrap (adapter, USB or operating system), not by RoboCrap's processing. ETHERNET1 `Timeout` = 10 allows 10 missing replies, so the controller stopped. In the other 92 captures the largest receive gap was 10.5 ms (one capture above 8 ms). PC power settings read on 2026-11-14 (plan Balanced, AC): display off after 600 s, sleep after 1800 s, USB selective suspend enabled; the RSI adapter `KRC4-Direct` (ASIX USB Gigabit) has SelectiveSuspend (SSIdleTimeout 60 s) and Energy-Efficient Ethernet enabled. The stall came 9 min 56 s after the last known PC input (the Run click), close to the 10 min display-off timeout; the System, Application and Kernel-Power logs have no entry between 19:42 and 19:44:30, so the trigger is not proven. The PC did not sleep (the run continued to be captured and sleep is set to 30 min).
  Evidence: the capture and log rows quoted; `powercfg /query`, `Get-NetAdapterAdvancedProperty -Name KRC4-Direct`.
- Observation: first stage B run (2026-11-14): the program reached the first `LIN StepTarget[Step]` (step 2, the descent) and the robot moved very slowly, as the Decision Log predicts: `$OV_PRO` = 10 % scales the CP velocity too, `$VEL.CP` 0.01 m/s from the job file gives 1 mm/s. Lengths from `C:\RoboCrapJobs\calibration_data.txt` (485 steps): the descent is 123.0 mm (123 s at 1 mm/s), steps 1–10 contain 123.0 mm LIN and 30.9 mm CIRC (chords through the auxiliary point) plus three retracts of about 3 mm; the whole session contains 791.5 mm LIN, 1120.4 mm CIRC and 158 retracts of about 3 mm, about 2390 mm in total, i.e. about 40 min at 1 mm/s against about 4 min at 10 mm/s, plus 158 searches of about 3 s each if the override does not scale the RSI search.
- Observation: A6 (log `plc_io_20261114_170121.csv`). Run 3 at 17:01:40.707, SELECT 41.690, finished 46.698 (5 s: the CELL with `WAIT SEC 5` is installed; the APPL_RUN rise was logged late at 43.425). EMERGENCY STOP at 17:02:45.153 (STOPMESS = 1), PERI_RDY and PRO_ACT = 0 at 45.205; releasing it changed no signal (screenshot: STOPMESS stays 1, PERI_RDY 0); the smartPAD was not touched. Run 2 at 17:05:26.102: PREP 26.327, CONF_MESS 26.427–26.629 (the DRIVES_ON pulse before it is not logged), PERI_RDY = 1 at 27.579, STOPMESS = 0 at 27.735, START with PGNO0 = 1 and EXT_START 27.844, APPL_RUN 27.871 (P00 took the number at the continuing EXT_START), PRO_ACT 27.878, STARTED; finished 32.872 (5.0 s). The `$CONF_MESS` edge was the only acknowledging action, so the PLC's `$CONF_MESS` acknowledges the EMERGENCY STOP message on this controller; `$STOPMESS` fell 0.16 s after `$PERI_RDY`, i.e. after the drives were on (about 1.2 s after the DRIVES_ON pulse), although the edge came before `$PERI_RDY` (KSS Fig. 6-26 draws it after). Consequence: `$CONF_MESS` and its wiring work; only "KSS01370 Passive STOP" is excluded from external acknowledgement, consistent with the KUKA message list (acknowledgement only on the originating unit). The Resume rule (X1.3 OFF) therefore also requires a smartPAD acknowledgement after an EMERGENCY STOP during a program, although the PLC could acknowledge that message; `$STOPMESS` alone cannot tell the two messages apart (an EMERGENCY STOP also drops `$PERI_RDY`, the STOP key does not).
  Evidence: the log rows quoted; user screenshots of the panel after program 3, with the EMERGENCY STOP pressed and after its release, 2026-11-14.
- Observation: final stage A session (log `plc_io_20261114_163255.csv`). A1 run 3 at 16:33:38.519, STARTED 39.517, finished 41.506. A3 run 4 STARTED 16:33:57.036, STOP key: STOPMESS = 1 at 57.910, PRO_ACT = 0 at 59.459; acknowledged at 16:34:28.284 (STOPMESS 0); Resume 34.930, EXT_START 35.559, RESUMED and finished at 37.609/37.704 (2.1 s = the rest of the 3 s wait; the PRO_ACT rise was logged late, a lost frame). A4: STOP key while CELL waited (STOPMESS = 1, PGNO_REQ = 1, PRO_ACT = 0 at 16:35:26.792), acknowledged 53.515, run 3 at 16:36:14.485: PGNO1 = 1 at 15.013, EXT_START 15.116, APPL_RUN 15.143 (P00 took the number at the continuing EXT_START), STARTED via `TR_Running` at 15.152, finished 17.144 (2.0 s). A5: EMERGENCY STOP at 16:36:46.755 (STOPMESS = 1), PERI_RDY and PRO_ACT = 0 at 46.807, X1.0 stays 1. (a) run 2 at 16:37:05.491 and 16:37:21.120, Abort at 08.427 and 23.043: ABORTED (logged 1.8 s and 1.2 s late). (b) run 2 at 16:37:27.297: TIMEOUT at 32.323. (c) run 2 at 16:38:45.771: TIMEOUT at 50.800; X1.0 fell only at 16:41:49.474, so the switch to T1 did not happen within the 5 s of ST_PREP. EXT_START and the PGNO bits stayed 0 in all A5 starts. The DRIVES_ON pulse is not in the log of any start (lost frame right after the command reply). A6 was not done.
  Evidence: the log rows quoted.
- Observation: start of program 4 at 13:50:22.305 after the BCO run: CELL started (PRO_ACT 22.970), STOPMESS = 1 at 23.155 while PRO_ACT = 1, PRO_ACT = 0 at 24.338; ST_START kept EXT_START = 1 until STOPMESS fell at 13:50:46.107 and the user pressed Abort at 48.929 (result ABORTED). The message that stopped CELL is not recorded.
  Evidence: the log rows quoted.
- Observation: the PLC function block `FB_WriteTxRespErr` writes the command at byte 8, the error at byte 9 and the code at bytes 10–11, but the PC parser `PlcMessageManager::parseRespErr` (in `src/devices/plc/plcmessagemanager.cpp`) reads command, error and code from bytes 6, 7 and 8–9 (the payload starts after the 6-byte header). Every PLC error reply would be misread; it went unnoticed because no PLC code sends errors yet.
  Evidence: snapshot text of `FB_WriteTxRespErr` (`wTxByte(idx:=8, val:=cmd); wTxByte(idx:=9, val:=err); wTxWord(idx:=5, val:=code)`) versus the RESP_OK blocks that write the command at `idx:=6`.
- Observation: the AS Programming Manual describes `SCLOSE` operand S1 as "0 closes a TCP socket", the opposite of `SOPEN` (1 = TCP). This plan does not use SCLOSE unless the reconnect check of Milestone 4 fails.


## Decision Log

- Decision: on the PC the step, result and program-state codes stay plain integers (keys `robotStep`, `robotResult`, `robotProgramState`); their names live only in `RobotStartPanel.qml`. The C++ enums `ROBOT_STEP`/`ROBOT_RESULT` listed under `Interfaces and Dependencies` are not added, and the unused enum `CELL` (IDLE/RUN/FIN) is removed from `PlcMessageManager` (exported to QML as `PlcMessage`, no QML user).
  Rationale: no C++ code would read those enums (`AGENTS.md` mandatory code reduction); the PLC defines the codes in `E_RSTEP`/`E_RRESULT`.
  Date/Author: 2026-11-13, agent.
- Decision: `PlcDevice::writeMessage` drops a request while the socket is not connected (logs `PLC is not connected`) instead of queuing it; `availableTid` and its "All PLC transaction ids are pending" path are removed because with one request in flight the next id is always free.
  Rationale: a Run queued while disconnected must never be sent at a later reconnect; one request in flight makes the 256-id search unnecessary.
  Date/Author: 2026-11-13, agent.
- Decision: `RobotIo` drives the PGNO bits in ST_START as well as in ST_SELECT (only for a job, `JobPgno` ≠ 0), raises `$EXT_START` in ST_START only from 0.1 s on, and labels a `TR_Running` exit of a job (`JobPgno` ≠ 0) STARTED instead of RESUMED.
  Rationale: found while preparing Milestone 5 (2026-11-13) by reading P00. When CELL is stopped while it waits inside `#PGNO_GET` (PGNO_REQ ON, `$PRO_ACT` OFF, APPL_RUN OFF; e.g. STOP key or E-STOP while CELL waits), P00 sits in `REPEAT … UNTIL $EXT_START==TRUE`. The next Run continues CELL with the ST_START edge, and P00 reads the PGNO bits at once. With the bits driven only in ST_SELECT it would read 0, which is program 1 (the calibration). The new job then runs with `TR_Running`, because PGNO_REQ is already FALSE when `$PRO_ACT` returns.
  Date/Author: 2026-11-13, agent.
- Decision: the start-sequence state (`RobotStep`, `JobPgno`, `RobotResult`) is appended to the IO change frame (payload 8: change type 0xD0, X1, Y1, X2, Y2, step, program, result) and to the SNAPSHOT reply (payload 9: command, status, X1, Y1, X2, Y2, step, program, result); `FB_VarMonitor`, its `FB_TxArbiter` input and output and `E_CHGTYPE.CELL_STATE` are removed, and the PC drops the `CELL_STATE` case.
  Rationale: stage A logs (2026-11-13) show lost change frames. One frame type instead of two removes the back-to-back IO/CELL_STATE pairs, and the 2 s heartbeat now repairs the step as well as the IO, so a lost frame no longer leaves the panel in a wrong step for longer than 2 s (Abort refused while the panel showed PREP). Fixing the send-complete detection itself needs facts about the AS332T socket flags that the manuals read do not give.
  Date/Author: 2026-11-13, agent.
- Decision: speeds in AUT EXT are programmed, not scaled by the override (supersedes the `AutExtOv` = 10 decision below). CELL sets `$OV_PRO` = 100 before every program number. Every program calls `INIT_PTP_VEL ( )` after its INI fold and after any `BAS` call that sets the PTP velocity, and then uses plain PTPs: `$VEL_AXIS[1..6]` = 100 in T1 (the controller limits T1 to 250 mm/s) and `PtpVelAuto` (5 %, `INIT_PTP_VEL.dat`) in AUT EXT, AUT and T2. LIN/CIRC use `$VEL.CP` (calibration: the Generate feed from the job file, 10 mm/s as in T1); the RSI search speed is the PC stream (1 mm/s). Because KRL fixes a motion's velocity when the advance run plans it, a program held by APPL_RUN while AUT EXT dropped must be restarted from CELL: PLC `ResumeBlocked` (set while APPL_RUN AND NOT `AUTEXT_ACT`, cleared when APPL_RUN falls) refuses RESUME, and RoboCrap mirrors the same rule (`robotResumeBlocked`) to disable Resume with the status "Program interrupted · mode changed · restart from CELL". `INIT_PTP_VEL` stops the advance run (`WAIT SEC 0`) so it reads the mode of the moment the robot reaches it.
  Rationale: user objection (2026-11-14): with POV 10 % every LIN ran at 1 mm/s (the 485-step session about 40 min of path), and a low programmed PTP velocity made T1 recovery slow. KUKA's override is a percentage of the programmed velocity and acts on all motion types; the forum practice is 100 % in automatic modes with every speed programmed (robot-forum.com thread 37144). Limits: PC and PLC can disagree after a RoboCrap reconnect while the PLC already holds ResumeBlocked (the PLC then refuses Resume with NOT_ALLOWED); a smartPAD override change in AUT EXT lasts until CELL's next program number and can only slow motions down; `PtpVelAuto` is a user setting.
  Date/Author: 2026-11-14, user and agent.
- Decision: Resume requires `$STOPMESS` (X1.3) FALSE. The panel disables Resume while an interrupted program (APPL_RUN ON, `$PRO_ACT` OFF, step IDLE) has a pending stop message and shows "Program interrupted · acknowledge message on smartPAD"; after the acknowledgement it shows "Program interrupted · message acknowledged · Resume ready". `FB_RunSetVar` refuses RESUME with NOT_ALLOWED while `STOPMESS` is TRUE. The texts say "message", not "Passive STOP", because `$STOPMESS` is collective (EMERGENCY STOP and operator safety set it too) and the PLC receives no message number.
  Rationale: clean A3 (2026-11-14): the PLC's `$CONF_MESS` edge does not clear "KSS01370 Passive STOP", so every Resume before the smartPAD acknowledgement ended with TIMEOUT after 5 s. The external-acknowledgement alternative (`KrcExtConfMsg.xml`) is documented in `docs/solutions/KUKA_PASSIVE_STOP_EXTERNAL_ACK.md` and not applied. Run keeps its rule; ST_PREP still pulses `$CONF_MESS`.
  Date/Author: 2026-11-14, user and agent.
- Decision: `CELL.src` sets `$OUT[APPL_RUN]=FALSE` in its INIT fold, before the first motion.
  Rationale: a stale APPL_RUN (ON without a running program) disables Run in the panel and is refused by `FB_RunSetVar`; P00 clears it only in `INIT_EXT`, which CELL reaches only in AUT EXT after HOME. The INIT line runs during the BCO run of a newly selected CELL in T1, which is already the documented way back after any stop, so no new operator action is needed. A stopped program that is resumed keeps CELL selected and does not run INIT again, so Resume keeps working.
  Date/Author: 2026-11-13, agent.
- Decision: Milestone 5 is evaluated from a PLC log file written by RoboCrap, not from ISPSoft monitoring. `PlcDevice` opens `records/plc_io_<yyyyMMdd_HHmmss>.csv` beside the executable on every connect and writes one row per change of the KUKA interface bits (X1.0, X1.2 … X1.7, Y1.0 … Y1.4), per change of the PLC step, program or result, per robot command (run k, resume, abort), per PLC error reply and per reply timeout; every row carries the full current state with named columns, so it reads without forward-filling.
  Rationale: user request (2026-11-13): the pulses are too fast to watch, and the operator should not need to know where `RobotStep`, `RobotResult` or `JobPgno` are written. The PC already receives every change frame; logging them costs one file per connection.
  Date/Author: 2026-11-13, user and agent.
- Decision: two program-number bits, PGNO = k − 1 for program k = 1 … 4, `CELL.SRC` CASE 0 … 3.
  Rationale: two binary bits encode four values; P00 accepts PGNO = 0 (no check in `#PGNO_GET`, Milestone 1); a third bit adds no wire-break detection.
  Date/Author: 2026-11-13, user and agent.
- Decision: a start is possible only in AUT EXT. The panel enables Run and Resume only while `$I_O_ACTCONF` (PLC input X1.0) is ON; `FB_RunSetVar` rejects RUN and RESUME without it; `RobotIo` drives no KUKA input without it; losing it during a start sequence ends the sequence with result AUTEXT_LOST.
  Rationale: user requirement (2026-11-13). `$I_O_ACTCONF` is TRUE only when AUT EXT is selected and `$I_O_ACT` is TRUE, which is exactly the condition under which the KR C4 accepts the PLC's signals.
  Date/Author: 2026-11-13, user.
- Decision: program 1 is `RSI_Probe_Calibration`; programs 2–4 are placeholders in `CELL.SRC` (`WAIT SEC 5` each since 2026-11-14, before `WAIT SEC 1/2/3`; no motion).
  Rationale: user's choice of test program (verified by the user in T1). The placeholders give a motion-free handshake test; their waits keep APPL_RUN ON far longer than the AS16AP11P-A input filter (default 10 ms), so the PLC sees it.
  Date/Author: 2026-11-13, user and agent.
- Decision: until this plan is complete, every change of the KR C4 Automatic External configuration (and of the PLC–EtherCAT signal assignment) is recorded only in `docs/plc_project_snapshot/AUT_EXT_DIOs_live.md`, which holds the current live values (updated 2026-11-13 to the Milestone 2 configuration). `docs/context/HARDWARE_SETUP.md` and `docs/context/ARCHITECTURE.md` are not edited again before Milestone 6, which re-syncs them from that file.
  Rationale: user instruction (2026-11-13): the plan is not finished and the DIO assignment may still change.
  Date/Author: 2026-11-13, user.
- Decision (superseded 2026-11-14 by the programmed-speed decision above): AUT EXT speed limit. `CELL.SRC` sets `$OV_PRO` to the CELL constant `AutExtOv` = 10 (%) before its first motion and again at the start of every LOOP pass, before each program number is read; CELL's `PTP HOME` and the first `PTP HOME` of `RSI_Probe_Calibration` are lowered from 100 % to 20 % axis velocity. Effective values: every PTP of the session runs at 20 % × 10 % = 2 % of the rated axis speeds (A1 4.4 °/s, A2 4.2 °/s, A3 5.4 °/s, A4 7.6 °/s, A5 6.2 °/s, A6 9.8 °/s from the rated 220/210/270/381/311/492 °/s in `src/application/robot/robotdefinition.cpp`), and the CP transfers of the calibration run at 10 mm/s × 10 % = 1 mm/s. A conservative estimate of the PTP TCP speed (all axes at their 2 % limit at once, lever arms A1 1.55 m, A2 1.2 m, A3 0.6 m, A4 and A5 0.2 m) is 0.3 m/s; real PTPs are slower because the axes of a PTP are synchronized to the slowest one. The PLC never writes the override.
  Rationale: user requirement (2026-11-13) that no motion started from RoboCrap jumps to maximum speed. The override acts on every programmed motion of every program CELL calls, including future programs 2–4, so it does not depend on each program's own velocities; re-applying it in the loop undoes any change made by a program or on the smartPAD between programs. Writing it from the robot interpreter (not from the PLC or the submit interpreter) keeps within KSS's safety rule. Raising `AutExtOv` later is a user decision, recorded here.
  Date/Author: 2026-11-13, user and agent.
- Decision: a new job (RUN) requires `$IN_HOME` in every case, not "HOME or CELL waiting".
  Rationale: `RSI_Probe_Calibration` can end 3 mm from the sphere and begins with `PTP HOME` at programmed velocity; in AUT EXT there is no BCO run. KUKA Robot Programming 3 also lists `$IN_HOME` as a precondition of the program-number transfer. After such an end the operator jogs clear in T1 and reselects CELL with a BCO run to HOME before the next start.
  Date/Author: 2026-11-13, agent.
- Decision: RESUME is accepted only when APPL_RUN is ON and `$PRO_ACT` is OFF; RUN is rejected while APPL_RUN is ON; `TR_Running` requires `$PRO_ACT` AND APPL_RUN; `TR_Request` requires a program number (JobPgno ≠ 0).
  Rationale: per P00, APPL_RUN stays TRUE from the acknowledgement until CELL asks for the next number, so APPL_RUN without `$PRO_ACT` identifies a stopped application program. A resume must not pass through program-number selection, and APPL_RUN alone is already TRUE at the start of a resume, so the resume can only be recognized by `$PRO_ACT` rising again.
  Date/Author: 2026-11-13, agent.
- Decision: minimal PLC output set; `$MOVE_ENABLE` and `$DRIVES_OFF` stay on the constant `$IN[1025]` (always TRUE). The PLC therefore cannot stop a running program; the operator stops it on the smartPAD.
  Rationale: user's choice; Y1.5/Y1.6 stay free for a later `$MOVE_ENABLE` output.
  Date/Author: 2026-11-12, user.
- Decision: the PLC only performs the start sequence (4 SFC steps). Running, finished and interrupted are derived on the PC from the KUKA status bits.
  Rationale: the PLC cannot act on those states in the minimal variant; the PC already receives the status bits in the IO change message.
  Date/Author: 2026-11-12, user and agent.
- Decision: `$STOPMESS` is part of the transition from ST_PREP to ST_START.
  Rationale: the KR C4 starts a program only on a rising edge of `$EXT_START`. An edge given while a stop message is still pending is ignored, and the level that follows produces no new edge, so the start would hang until the 30 s timeout.
  Date/Author: 2026-11-12, agent.
- Decision: task order PCHandler → RobotIo → RobotHandler.
  Rationale: RobotIo reads the step flags left by the previous scan, so in the scan in which a transition fires RobotIo still sees the old step and can record the result and the `Fail` flag that the transition reads in the same scan. Outputs lag one PLC scan (milliseconds), which the KR C4 handshake does not notice.
  Date/Author: 2026-11-12, agent.
- Decision: transition programs are written in Structured Text (ST), not ladder.
  Rationale: ISPSoft allows ST transitions (one assignment to the transition name); they can be pasted from this plan, so only the SFC chart itself has to be drawn.
  Date/Author: 2026-11-12, agent.
- Decision: the PC module-1 output switches become display-only and the PLC rejects WRITE_IO for module 1.
  Rationale: every Y1 output now belongs to the robot interface or the probe copy; a manual switch on `$EXT_START` would start the robot.
  Date/Author: 2026-11-12, agent.
- Decision: the double use of Y1.7 (`LED_GREEN` in the Main Table and the probe copy in `ProbeMap`) is left unchanged.
  Rationale: both write the same value; correcting the lamp address changes lamp behaviour, which is outside this task.
  Date/Author: 2026-11-12, agent. Superseded 2026-11-13: the user removed the `LED_GREEN` coil from `ProbeMap`, so only the probe copy drives Y1.7; `LED_GREEN` still carries the address Y1.7 in the Main Table and is written only in `PCHandler`'s first scan.


## Outcomes & Retrospective

Plan complete (2026-11-14). RoboCrap starts KRL programs 1–4 in AUT EXT through the PLC; program 1, the probe sphere calibration, ran its full 485-step session (158 touches) in 16 min 41 s. What changed against the original design, and why: the start-sequence state travels in the IO frame and the SNAPSHOT reply (lost PLC frames); Resume needs a smartPAD acknowledgement of the STOP key's "Passive STOP" (`$CONF_MESS` does not clear it); speeds are programmed with the override at 100 % (a 10 % override slowed every LIN to 1 mm/s); Resume is blocked after a mode change (KRL keeps planned velocities); the PC needs power settings that keep the RSI adapter awake. Open, not part of this plan: lost PLC change frames (`docs/context/ARCHITECTURE.md` §7), external acknowledgement of the Passive STOP (`docs/solutions/KUKA_PASSIVE_STOP_EXTERNAL_ACK.md`), the calibration evaluation itself (`plans/PROBE_SPHERE_RSI_CALIBRATION_EXECPLAN.md`).

Milestone 1 (2026-11-13): backups and the live configuration are in place; the P00 reading settled the bit count and changed the Resume/Run rules (Decision Log).

Milestone 4 compliance pass (2026-11-13, `AGENTS.md`): no lambdas, templates, exceptions or output parameters; the largest signature is `nextProgramState(ProgramState current, const QVariantList& x1)` (two parameters, result by return value, private static so it can name the private enum); new invokables take zero or one `QVariantMap` and have unique names; timers are created in `startDevice` on the GeneralIO thread with the device as parent and deleted in `stopDevice` after the socket, like the socket; `runRobotProgram` uses `m_mgr` without a null check because `AbstractDevice::invoke` runs invokables only after `startDevice` created it; `publishIo` replaces two identical four-key emits; `PlcMessage` QML enum API changed (`VAR_TYPE` values, `CELL` removed) with no QML consumer. Line counts (physical/non-blank) before → after: `plcdevice.cpp` 348/281 → 444/362, `plcdevice.h` 54/41 → 71/57, `plcmessagemanager.cpp` 529/413 → 549/431, `plcmessagemanager.h` 136/114 → 131/110, `DeltaPanel.qml` 93/72 → 101/78, new `RobotStartPanel.qml` 85/66, `src/application/CMakeLists.txt` +1 line. Growth comes from the required queue/timeout/heartbeat, three invokables, the program-state rule and the new panel; the smaller alternative (sending without a queue) is ruled out by the PLC's single receive buffer.

Milestone 5 additions to the PC side (2026-11-13, compliance pass): no connect SNAPSHOT (`onConnected` only starts the heartbeat); PLC log in `PlcDevice` with two private methods `openLog()` and `writeLog(const QString& event)` (one parameter, operate on the device's own file and state), two helpers in the unnamed namespace `codeName(const QVariant&, const QStringList&)` and `bitText(const QVariantList&, int)` (two parameters, result by return value), members `m_log` (QFile created in `startDevice` with the device as parent and deleted in `stopDevice`, like the socket and timers) and `m_logged` (last logged values); file I/O is one flushed line per change on the GeneralIO thread, not on ControlIO. Counts: `plcdevice.cpp` 444/362 → 525/433, `plcdevice.h` 71/57 → 78/64; the growth is the log requested by the user.

Stage A fixes, PC side (2026-11-13, compliance pass): no new function, member or parameter; `parseStateChange` case `IOs` and `parseRespOk` case `SNAPSHOT` read three more bytes into the existing keys; the `CELL_STATE` case, its enum value and the `CELL_STATE` branch of `publishData` are removed; `publishIo` copies and logs the keys with two range-for loops over key lists instead of separate inserts and comparisons. No lambda, template, exception or output parameter. Counts (physical/non-blank) before → after: `plcdevice.cpp` 525/433 (plus the read-error lines added since) → 519/427, `plcmessagemanager.cpp` 549/431 → 545/431, `plcmessagemanager.h` 131/110 → 130/109, `plcdevice.h` unchanged 78/64. KRL: `CELL.src` +3 lines (two comment lines and the assignment). PLC text: `FB_VarMonitor` removed (about 25 lines and 6 symbols), `FB_IOMonitor` +13 lines and 3 symbols, `FB_RunSnapShot` +4 lines, `FB_TxArbiter` −5 lines and 2 symbols, `PCHandler` −2 lines and 1 symbol.

Speed scheme, PC side (2026-11-14, compliance pass): one member `m_resumeBlocked` (required state: the PC needs the PLC's latch to disable Resume and the IO frame does not carry it; reusing `ProgramState` was rejected because the latch must survive a Running phase in T1) and one constant `X1AutExt`; the rule is evaluated in `publishIo` next to the program state, published only on change and reset on disconnect with the other robot keys. No new function, parameter, lambda, template, exception or output parameter. Counts (physical/non-blank): `plcdevice.cpp` 513/422 → 526/434, `plcdevice.h` 78/64 → 80/66, `RobotStartPanel.qml` 91/72 → 94/75 (Resume rule and one status text). KRL: `CELL.src` 75/65 → 71/61, `RSI_Probe_Calibration.src` 149/137 → 149/137, new `INIT_PTP_VEL.src` 20 and `.dat` 7 lines. PLC text: `RobotIo` +7 lines, one Main Table symbol, one condition in `FB_RunSetVar`.

Milestone 2 (2026-11-13): the PLC and the KR C4 are wired and the AUT EXT interface is configured; CELL runs every AUT EXT program at POV 10 % and no program motion is programmed above 20 %. The PLC program is still the old one; nothing can be started yet.


## Context and Orientation

The equipment. The robot is a KUKA KR10 R1420 with a KR C4 smallsize-2 controller (KUKA System Software KSS 8.3). The PLC is a Delta AS332T CPU with two 8-in/8-out expansion modules: module 1, AS16AP11P-A (inputs X1.0 … X1.7, sourcing outputs Y1.0 … Y1.7), and module 2, AS16AP11T-A (X2.x/Y2.x). On the KR C4 an EtherCAT coupler Beckhoff EK1100 carries an EL1809 (16 digital inputs, seen in KRL as `$IN[1]` … `$IN[16]`) and an EL2809 (16 digital outputs, `$OUT[1]` … `$OUT[16]`). Today only one wire connects PLC and robot: Y1.7 → EL1809 input 8 (`$IN[8]`), a copy of the touch-probe signal on X2.0 made by the PLC program `ProbeMap`. That wire stays as it is; the calibration program reads it.

Automatic External (AUT EXT) is a KR C4 operating mode in which a "higher-level controller" (here the PLC) starts programs through digital signals. The KR C4 menu Configuration > Inputs/outputs > Automatic External assigns each interface signal to an input or output number; the constant input `$IN[1025]` is always TRUE and `$IN[1026]` always FALSE. In AUT EXT the controller runs `CELL.SRC` (folder R1 on the controller, copy in `resources/files/KR C4/R1/cell.src`): it checks HOME, moves `PTP XHOME`, initializes the interface (`P00(#INIT_EXT …)`), and then loops: `P00(#EXT_PGNO,#PGNO_GET,DMY[],0)` reads a program number into `PGNO`, `SWITCH PGNO … CASE n` branches, `P00(#EXT_PGNO,#PGNO_ACKN,DMY[],0)` acknowledges and the branch calls the user program; an unknown number goes to `DEFAULT` with `#PGNO_FAULT`. There is no BCO run (the slow "block coincidence" approach move) in AUT EXT: the robot moves at programmed velocity, without the 250 mm/s limit of T1. Selecting CELL and its BCO run to HOME happen in T1 before the switch to AUT EXT (KSS 8.3 SI §8.11). The program override `$OV_PRO` (POV, in %, shown on the smartPAD status bar) scales every programmed velocity; this plan's `CELL.SRC` sets it to 10 % (Decision Log).

The signals used (direction seen from the KR C4): inputs `$EXT_START` (a rising edge starts or continues a program; with PGNO_VALID = 0 P00 also takes the program number when `$EXT_START` becomes TRUE), `$DRIVES_ON` (a high pulse of at least 20 ms switches the drives on), `$CONF_MESS` (a rising edge acknowledges messages), the program-number bits starting at PGNO_FBIT; outputs `$I_O_ACTCONF` (AUT EXT selected and interface active), `$PERI_RDY` (drives on), `$STOPMESS` (a message that stops the robot is pending, including E-STOP), `$PRO_ACT` (a program — CELL or the called user program — is being executed), PGNO_REQ (TRUE while P00 waits for a program number, FALSE once acknowledged), APPL_RUN (set TRUE by P00 at the acknowledgement, set FALSE when CELL asks for the next number), `$IN_HOME` (robot at HOME).

The handshake, as P00 implements it (and as KSS 8.3 SI Fig. 6-24 draws it): drives-on pulse → `$PERI_RDY`; acknowledge pulse → `$STOPMESS` falls; `$EXT_START` edge → CELL runs, `$PRO_ACT` rises; when `$EXT_START` is FALSE again P00 raises PGNO_REQ; the PLC sets the number bits and raises `$EXT_START` → P00 reads the bits, CELL acknowledges: PGNO_REQ falls, APPL_RUN rises. When the user program ends, CELL calls `#PGNO_GET` again: APPL_RUN falls and, once `$EXT_START` is FALSE, PGNO_REQ rises; `$PRO_ACT` stays TRUE. An interrupted program is a user program that the KR C4 stopped before its END (E-STOP, safety gate, STOP key or a programmed stop): it stays selected at the stopped line, `$PRO_ACT` is FALSE, PGNO_REQ is FALSE and APPL_RUN keeps its value (TRUE per P00); after drives on and acknowledge, the next `$EXT_START` edge continues it instead of starting a new one.

The PLC project. The ISPSoft project `RoboCrapPLC` (not in the repository; its text snapshot is `docs/plc_project_snapshot/ISPSoft project.md` plus draw.io files of the ladder and SFC parts) contains these program POUs (a POU is a program, function block or function in ISPSoft): `PCHandler` (Structured Text, TCP server for the PC), `RobotHandler` (SFC, sequential function chart — steps with transitions between them), `ProbeMap` and `onLostConnection` (ladder). `PCHandler` opens TCP socket 1 as a server with `SOPEN(1,1,2)` (mode 2: the first receive word `Rx[0]` = D200 holds the number of received bytes, the frame follows from `Rx[1]`); `Tx` (D100, 16 words) is the 32-byte send buffer. `FB_CmdMonitor` takes one received request, `FB_TxArbiter` decides which message is built into `Tx` next (command reply, IO change, variable change), `FB_IOMonitor` sends IO change frames, `FB_VarMonitor` is commented out. The command blocks `FB_Run*` read request bytes with `rRxByte`/`rRxWord` and write the reply with `wTxHeader`, `wTxByte` (byte offset in the 32-byte frame) and `wTxWord` (word index into `Tx`).

The message format (shared with the PC; every 16-bit word is byte-swapped on the wire by the PC, which the PLC FBs already account for): a 6-byte header — magic 0xAA55, version 1, type (0xA0 request, 0xA3 reply OK, 0xAC reply error, 0xAF change notification), transaction id, payload length — then the payload. Requests carry `cmd`, `flags` and command data; PLC frames are always 32 bytes. The command `SET_VAR` (0xBC) carries two bytes, `var` and `attr`; its OK reply echoes them. The change notification `CELL_STATE` (0xD3) already exists in the PC parser with a 1-byte value and is never sent by the PLC.

The PC side. `src/devices/plc/plcdevice.cpp` (`PlcDevice`, runs on the I/O thread "GeneralIO") owns the `QTcpSocket`, builds requests through `PlcMessageManager` (`src/devices/plc/plcmessagemanager.cpp`, header `src/devices/include/devices/plc/plcmessagemanager.h`) and publishes values with `emit stateReady(QVariantHash)`; `DeviceRunner` (`src/application/devices/devicerunner.cpp`) copies them into the QML map `plc.data`. Public `Q_INVOKABLE` methods returning void with zero arguments or one `QVariantMap` are callable from QML as `plc.invoke("name", {...})`; names must be unique. The PLC panel is `src/application/qml/Views/Dashboard/DeltaPanel/DeltaPanel.qml` with `DeltaModuleAP.qml` per module (`xTags`, `yTags`, `xPlugged`, `yPlugged`, `yDisplayOnly`). Today `PlcDevice::writeMessage` sends every request at once; replies are matched by transaction id; nothing is sent periodically.

The calibration program (program 1). `RSI_Probe_Calibration` moves PTP HOME, reads `/RoboCrapJobs/calibration_data.txt` (written by RoboCrap's "RSI Sampling" in Measuring mode, Calibration submode), and executes the calibration steps; every approach to the sphere is a search under RSI (RobotSensorInterface: the PC streams position corrections over UDP every 4 ms) that stops on the probe input `$IN[8]`. It needs the RSI device connected in RoboCrap and a fresh RSI Sampling; without the job file it ends right after HOME. The DAT value `StepLimit` (1 … step count − 1) runs only the first steps; 10 runs touches 1–3, 58 the first pass.


## Plan of Work

The work runs in the order controller → wiring → PLC → PC → end-to-end, so each milestone can be checked before the next one depends on it.

Milestone 1 is complete (see `Progress`).

Milestone 2 (agent writes the KRL files; user wires and configures; robot in T1). The agent creates `resources/files/AutExt/CELL.src` (the archived `R1/cell.src` with the program override, PTP HOME at 20 % and the new CASE section; full text under `Artifacts and Notes`) and changes the first `PTP HOME` of `resources/files/rsi/Calibration/RSI_Probe_Calibration.src` from `Vel= 100 %` to `Vel= 20 %`, updating `RSI_Probe_Calibration_CONTRACT.md` "Frames and motion". The user then wires, with the robot controller and PLC powered off: Y1.0 → EL1809 input 1, Y1.1 → input 2, Y1.2 → input 3, Y1.3 → input 4, Y1.4 → input 5 (same wiring principle as the existing Y1.7 → input 8); EL2809 output 1 → X1.0, output 3 → X1.2, output 4 → X1.3, output 5 → X1.4, output 6 → X1.5, output 7 → X1.6, output 8 → X1.7 (output 2 and X1.1 stay free). The AS16AP11P-A inputs have S/S to 0 V; check that this matches the EL2809 sourcing (+24 V switching) outputs before powering up. On the smartPAD (user group Expert) set Automatic External inputs: `$I_O_ACT` 1025, `$EXT_START` 1, `$DRIVES_ON` 2, `$CONF_MESS` 3, PGNO_TYPE 1, PGNO_FBIT 4, PGNO_LENGTH 2, PGNO_PARITY 0, PGNO_VALID 0, `$MOVE_ENABLE` 1025, `$DRIVES_OFF` 1025; outputs: `$I_O_ACTCONF` 1, `$PERI_RDY` 3, `$STOPMESS` 4, `$PRO_ACT` 5, PGNO_REQ 6, APPL_RUN 7, `$IN_HOME` 8 (all others keep their present values). If the controller reports a signal allocation conflict (message KSS00251), find the other signal on that number and move it to an unused internal number first, then record it under `Surprises & Discoveries`. Replace `R1\CELL.SRC` on the controller by `resources/files/AutExt/CELL.src` and `RSI_Probe_Calibration.src` by the changed file from `resources/files/rsi/Calibration/` (the DAT file is unchanged). Acceptance (robot in T1): both programs open without error messages; selecting CELL and pressing Start with the enabling switch (BCO run to HOME, T1 speed) sets the POV display on the smartPAD to 10 %; in ISPSoft online monitoring force Y1.0 … Y1.4 one at a time and see `$IN[1]` … `$IN[5]` change on the smartPAD (Display > Inputs/outputs > Digital I/O); in T1 these inputs start nothing because `$I_O_ACTCONF` is FALSE outside AUT EXT. With the robot at HOME X1.7 is ON; with drives switched on in T1 (enabling switch) X1.2 is ON; X1.0 is OFF in T1. Remove all forces afterwards.

Milestone 3 (user in ISPSoft; PLC connected; robot in T1). The ISPSoft edits are described step by step under `Concrete Steps`, with the ST text under `Artifacts and Notes`. In outline: update the four DUTs (data unit types: user enumerations), add the robot symbols to the Main Table (global symbol table), fix `FB_WriteTxRespErr`, replace the bodies of `FB_RunSetVar` and `FB_VarMonitor`, add the module-1 guard to `FB_RunWriteIo`, add header validation to `FB_CmdMonitor`, add the new-connection flag to `FB_IOMonitor`, fix the send-complete logic in `PCHandler`, create the new ST program `RobotIo`, empty and redraw `RobotHandler` as a 4-step SFC with 6 ST transitions, set the task order, set the socket keep-alive, compile, download. Acceptance: compile reports no errors; the existing RoboCrap application (not yet changed) still connects, shows the module 2 IO and can switch Y2 outputs; it logs `READ ERROR: 229` (BAD_PLEN) once after each connect and after each change of `RobotStep`, `JobPgno` or `RobotResult`, because its parser still expects the old 2-byte `CELL_STATE` payload — expected until Milestone 4; switching a Y1 output from the old application now logs `PLC response error: cmd=181, err=1, code=61191` (0xB5 WRITE_IO, BAD_REQUEST, 0xEE07 BAD_MOD) instead of changing the output. With the robot in T1 (X1.0 OFF) set `JobPgno` = 1 and then `CmdRun` = TRUE in the ISPSoft monitor (Modify value; this writes PLC variables and is the user's own action): `RobotHandler` enters ST_PREP and returns to ST_IDLE within one or two scans with `RobotResult` = 5 (AUTEXT_LOST), `CmdRun` is FALSE again, and Y1.0 … Y1.4 never switch on. This proves that the PLC drives no KUKA input outside AUT EXT.

Milestone 4 (agent edits C++ and QML; user builds and runs; robot in T1). The agent implements the PC side described under `Interfaces and Dependencies`: new `VAR_TYPE` codes, the 4-byte `CELL_STATE`, one request in flight with a reply timeout, a 2 s SNAPSHOT heartbeat, three new invokables, the robot program state derived from X1, and the PLC panel changes (module 1 tags and display-only outputs, a new `RobotStartPanel.qml`). The agent then performs the `AGENTS.md` compliance pass and records counts here, and asks the user to build `robocrap` and to run the application. Acceptance with the PLC connected and the robot in T1 (no motion is possible: T1 ignores the AUT EXT inputs and needs the enabling switch): the panel shows the KUKA tags on X1/Y1 and X1.7 ON while the robot is at HOME; the module 1 output switches are display-only; Run and Resume are disabled and the status line shows "AUT EXT off" because X1.0 is OFF in T1; Abort is disabled because `robotStep` is 0. Reconnect: Disconnect and Connect three times in a row in Settings > Network; each connect succeeds and the panel shows the current IO again. Heartbeat: leave the application connected for one minute; the console shows no `PLC reply timeout` line and the connection stays up (the PLC keep-alive of 10 s would otherwise close it).

Milestone 5 (user; each stage requires the user's explicit authorization, because AUT EXT starts CELL, which commands PTP HOME at programmed velocity, and stage B moves the robot along the calibration path). Common preparation: robot exactly at HOME, cell clear, operator safety closed as the KR C4 requires for automatic modes, operator at the E-STOP. Following KSS §8.11: in T1 select `CELL.SRC`, perform the BCO run (enabling switch + Start until "Programmed path reached (BCO)"; CELL sets the program override to 10 % itself), switch to Automatic External; X1.0 turns ON and Run becomes enabled in RoboCrap. Check that the smartPAD shows POV 10 % before every run; do not raise it on the smartPAD (CELL resets it before every program anyway).

Before stage A, correct `RobotIo` in ISPSoft to the current "RobotIo body" under `Artifacts and Notes` (the `TR_Running` result block and the output block changed on 2026-11-13), compile, download, and check in T1 that RoboCrap still shows "AUT EXT off". During stage A no calibration job may exist: do not run "RSI Sampling" in this RoboCrap session (RoboCrap deletes `C:\RoboCrapJobs\calibration_data.txt` at start-up), so that even a wrong program number 0 would only run the calibration's `PTP HOME` from HOME and end, without motion.

Stage A, no motion (programs 2–4 only wait). The operator only presses panel buttons and smartPAD keys and watches the robot; every signal is evaluated afterwards from the PLC log `records/plc_io_<yyyyMMdd_HHmmss>.csv` beside the executable (one file per connection, one row per change of X1.0, X1.2 … X1.7, Y1.0 … Y1.4, of the PLC step/program/result, and per command, PLC error reply or reply timeout; the time is the PC receive time). Pulses shorter than the PLC's send cycle can be missing from the log. The user sends the log file after the stage; the agent evaluates it against the expectations below and records the result here. Sequence, without waiting for any intermediate signal:

A1, program 3 (first start, CELL starts): press Run. Expected in the log: run 3; PREP; START with PGNO1 = 1, PGNO0 = 0; EXT_START, then PRO_ACT; PGNO_REQ; SELECT; EXT_START; APPL_RUN = 1 and PGNO_REQ = 0 with result STARTED; about 2 s later APPL_RUN = 0 and PGNO_REQ = 1. No robot motion.

A2, programs 2 and 4 (CELL waiting): wait until the panel shows "program finished", run program 2, wait for "program finished", run program 4. Expected: each STARTED, APPL_RUN = 1 for about 1 s and 3 s.

A3, interruption and Resume (passed 2026-11-14 with smartPAD acknowledgement; repeated once in the final session because Resume now requires `$STOPMESS` FALSE): run program 4; while the panel shows "program running", press the STOP key on the front of the smartPAD once (not the EMERGENCY STOP device). Expected: STOPMESS = 1, PRO_ACT = 0, APPL_RUN still 1; Resume disabled; status "Program interrupted · acknowledge message on smartPAD". Acknowledge "KSS01370 Passive STOP" on the smartPAD: STOPMESS = 0, Resume enabled, status "Program interrupted · message acknowledged · Resume ready". Press Resume: PREP, START, EXT_START with PGNO bits 0, PRO_ACT = 1, result RESUMED, then APPL_RUN = 0 and PGNO_REQ = 1 after the rest of the 3 s wait.

A4, stopped CELL: when the panel shows "program finished" (CELL waits for a number: PRO_ACT = 1, PGNO_REQ = 1), press the STOP key once. Expected: PRO_ACT = 0, PGNO_REQ stays 1, STOPMESS = 1, APPL_RUN 0 (the status line keeps the step text: no interrupted application program). Acknowledge the message on the smartPAD (STOPMESS = 0; Run checks no STOPMESS, and a Run with the message pending would end with TIMEOUT), then run program 3. Expected: PGNO1 = 1, PGNO0 = 0 in the same or an earlier row than EXT_START = 1; PRO_ACT = 1; APPL_RUN = 1 for about 2 s; result STARTED. A short APPL_RUN or the calibration program on the smartPAD means a wrong number: press the STOP key at once.

A5, with the EMERGENCY STOP pressed (nothing can move), in this order: press the EMERGENCY STOP and keep it pressed. (a) Run program 2 and press Abort within 5 s: result ABORTED (passed 2026-11-14). (b) Run program 2 and wait: result TIMEOUT 5 s after PREP (passed 2026-11-14). (c) AUT EXT lost during a start: the only requirement is that X1.0 falls while the PLC is in ST_PREP, i.e. within 5 s after Run. Rehearse the mode change once without Run (turn the key switch, select T1, turn the key back, then return to AUT EXT the same way). Then press Run for program 2 and immediately change the mode to T1 the same way. Expected: result AUT_EXT_LOST at the X1.0 fall, Y1.0 … Y1.4 stay 0 (replaces the skipped forced-`CmdRun` check of Milestone 3). In (a) to (c) EXT_START and the PGNO bits stay 0. Switch the smartPAD back to AUT EXT with the EMERGENCY STOP still pressed.

A6, does the PLC's `$CONF_MESS` acknowledge the EMERGENCY STOP message (open point, KSS 8.3 SI Fig. 6-26): release the EMERGENCY STOP (turn it) and do not acknowledge anything on the smartPAD. Run program 2. Result 1: PREP with the DRIVES_ON and CONF_MESS pulses, PERI_RDY = 1, STOPMESS = 0, then START/SELECT and STARTED: `$CONF_MESS` works for this message. Result 2: STOPMESS stays 1 and TIMEOUT: `$CONF_MESS` acknowledges neither message; then acknowledge on the smartPAD and run program 2 once more (expected STARTED, with the drives switched on by the DRIVES_ON pulse).

Recovery during stage A, if X1.6 APPL_RUN is ON in the panel while no program runs (Run stays disabled; after an emergency stop, or after a stopped program was cancelled): switch the smartPAD to T1, select `CELL.SRC`, hold the enabling switch and press Start until "Programmed path reached (BCO)"; CELL's INIT fold sets APPL_RUN FALSE before the BCO motion and X1.6 turns OFF in the panel; switch back to AUT EXT.

Stage B, calibration (motion). In RoboCrap (Measuring mode, Calibration submode) generate the calibration path and run "RSI Sampling" with the RSI device connected, as in the T1 runs of `plans/PROBE_SPHERE_RSI_CALIBRATION_EXECPLAN.md`; on the smartPAD set `StepLimit` = 10 in `RSI_Probe_Calibration.dat`. With POV 10 % the PTPs run at 2 % of the rated axis speeds and the transfers at 1 mm/s (Decision Log), so the run takes longer than in T1. Whether the override also scales the RSI search (the PC streams 1 mm/s) is not documented in the sources read for this plan: if it does, each 5 mm search stream moves only 0.5 mm, the probe misses the sphere and the program ends with `LIN SearchStart` (its designed exit for a missed touch, no collision). Record the outcome and the detection cycles from `python/rsi_touches.py` under `Surprises & Discoveries`; changing `AutExtOv` or the search is then a user decision. Select program 1 and press Run. Expected: PGNO 0 (Y1.3 = Y1.4 = 0), APPL_RUN ON, the robot executes steps 1–10 (touches 1–3) and ends with `LIN SearchStart` near the sphere; state "finished"; the `records/` files of the run exist and `python -I python/rsi_touches.py <records directory> <id>` finds 3 touches. Afterwards X1.7 is OFF and Run is disabled: bring the robot back as the calibration contract requires (T1, jog clear of the sphere, select CELL, BCO run to HOME, AUT EXT). Repeat with `StepLimit` = 58 and finally 0 (full session) when the user decides to.

Milestone 6 (agent, documentation only). Update `docs/context/HARDWARE_SETUP.md` (the Automatic External table with the new values, the I/O wiring), `docs/context/ARCHITECTURE.md` §1 (PLC terminals table: X1.0 … X1.7, Y1.0 … Y1.4), §3.4 `PlcDevice` row (new invokables, one request in flight, heartbeat, robot keys) and §7 (remove the "PLC pending lifetime" open question, now answered by the reply timeout), add an "AUT EXT start" paragraph to `resources/files/rsi/Calibration/RSI_Probe_Calibration_CONTRACT.md` (program 1 of CELL, PGNO 0, `$IN_HOME` required for a new start), and bring `docs/plc_project_snapshot/ISPSoft project.md` up to date if the project changed after Milestone 3 (the snapshot was rewritten after Milestone 3, and the user reorganized the folder to the seven files `AUT_EXT_DIOs_live.md`, `FB_TRisingPulse.drawio`, `ISPSoft project.md`, `onLostConnection.drawio`, `PLC_AUT_EXT_proposal.md`, `ProbeMap.drawio`, `RobotHandler_SFC.drawio` on 2026-11-13).


## Concrete Steps

ISPSoft steps of Milestone 3, in order. Work offline (not connected) until step 16. Every ST text referenced here is under `Artifacts and Notes`.

1. Open the project `RoboCrapPLC`. Make sure the Milestone 1 backup exists.

2. DUTs. In the project tree open Data Unit Type (DUT). Open `E_VARTYPE` and replace its body by the text "DUT E_VARTYPE". Open `E_CODE` and add the three new members of "DUT E_CODE". Add two new DUTs (right-click Data Unit Type > Add, type ENUM): `E_RSTEP` and `E_RRESULT` with the texts given.

3. Main Table (Global Symbols > Main Table). Add the rows listed in "Main Table additions" (Class VAR, address and type as listed). Do not delete `CmdStartCell` and `CmdSfyOk` yet; they are removed in step 10 after their users are gone.

4. `FB_WriteTxRespErr`: replace the body by "FB_WriteTxRespErr body". No new local symbols.

5. `FB_RunSetVar`: add the local symbols listed with "FB_RunSetVar body" and replace the body.

6. `FB_RunWriteIo`: add the local symbol `wRespErr : FB_WriteTxRespErr` and wrap the existing body as shown in "FB_RunWriteIo guard".

7. `FB_CmdMonitor`: add the local symbol `wRespErr : FB_WriteTxRespErr` and replace the `IF BuildNow AND Pending THEN … END_IF;` block by "FB_CmdMonitor dispatch".

8. `FB_IOMonitor`: add the local symbol `WasConnected : BOOL := FALSE` and insert "FB_IOMonitor connection edge" directly after the existing `IF SM402 THEN … END_IF;`.

9. `FB_VarMonitor`: add the local symbols listed with "FB_VarMonitor body" and replace the commented-out body. (Removed again by "Stage A fix in ISPSoft"; the body is in git history.)

10. `PCHandler`: add the local symbol `SendStarted : BOOL := FALSE` and replace the two blocks after `// SEND DATA TO PC` by "PCHandler send". Then open `RobotHandler`, delete every action under RobotHandler > Actions (right-click > Delete) and every transition under RobotHandler > Transitions, select the whole chart in the working area and delete it, and delete all local symbols of `RobotHandler`. Now delete `CmdStartCell` and `CmdSfyOk` from the Main Table; ISPSoft must not report remaining uses (use Find/Cross reference if it does).

11. New program `RobotIo`: right-click Programs > Add (New POU), name `RobotIo`, type Program, language Structured Text, task Cyclic (the same cyclic task as `PCHandler`). Its only local symbol is `StepTimeout : BOOL`. Paste "RobotIo body".

12. `RobotHandler` SFC. In its local symbol table declare four symbols of type STEP: `ST_IDLE`, `ST_PREP`, `ST_START`, `ST_SELECT`. Create six transitions (right-click RobotHandler > Transitions > Add, language Structured Text): `TR_Run`, `TR_Prepared`, `TR_Running`, `TR_Request`, `TR_Acked`, `TR_Fail`, and paste each body from "RobotHandler transitions" (each is one assignment to the transition name; ISPSoft requires that the transition program assigns its own name). Draw the chart in the working area (the grid editor; Shift+F2 adds a step below the selected object, Shift+F3/Shift+F4 add a parallel transition to the right/left of the selected transition, which is a divergence of sequence selection — exactly one of the branches is taken, the leftmost true one first — and Shift+F11 inserts a jump at the bottom of a branch):
   1. The chart starts with one step; assign `ST_IDLE` (select it, press Enter, choose from the list) and right-click > Set Initial.
   2. Below it the transition `TR_Run`, then step `ST_PREP`.
   3. Below `ST_PREP` the transition `TR_Prepared`; select it and add a parallel transition to the right (Shift+F3), assign `TR_Fail`, and insert a jump under it (Shift+F11) to `ST_IDLE`.
   4. Below `TR_Prepared` step `ST_START`, below it transition `TR_Request`. Select `TR_Request`, add a parallel transition to the left (Shift+F4), assign `TR_Running`, jump to `ST_IDLE`. Select `TR_Request` again, add a parallel transition to the right (Shift+F3), assign `TR_Fail`, jump to `ST_IDLE`. The order from left to right is `TR_Running`, `TR_Request`, `TR_Fail`; `TR_Request`, `TR_Running`, `TR_Fail` (as drawn by the user, 2026-11-13) is equivalent, because `TR_Running` and `TR_Request` are never TRUE together (P00 sets PGNO_REQ only while APPL_RUN is FALSE and APPL_RUN only after PGNO_REQ fell; a resume has JobPgno = 0). `TR_Fail` must stay rightmost so that a success in the same scan wins over a timeout.
   5. Below `TR_Request` step `ST_SELECT`, below it transition `TR_Acked` with a jump to `ST_IDLE` at the bottom of the main path. Select `TR_Acked`, add a parallel transition to the right, assign `TR_Fail`, jump to `ST_IDLE`.
   6. No step gets an action. Compare the chart with `docs/plc_project_snapshot/RobotHandler_SFC.drawio` (the drawing's `TR_Fail` rail stands for the three right-hand `TR_Fail` branches).

13. Task order. Right-click Tasks > Task Property (Task Manager), select the cyclic task that holds the programs and arrange the Assigned POUs so that `PCHandler` is first, `RobotIo` second, `RobotHandler` third; `ProbeMap` and `onLostConnection` keep their places after them.

14. Socket keep-alive. HWCONFIG > CPU > PLC Parameter Setting > Ethernet-Advanced > Socket > TCP Socket 1: set Keep Alive Timer to 10 s (the PC heartbeat is 2 s). Record the old value under `Surprises & Discoveries`.

15. Compile (Compile > Compile all). Expected: 0 errors. Typical error sources: a transition program that does not assign its own name, a symbol typed differently from the Main Table, `RobotHandler.ST_PREP.T` compared with an INT literal (if ISPSoft rejects it, write the literal as `DWORD#500` and record it under Surprises).

16. Connect, switch the PLC to STOP, download program and HWCONFIG parameters, switch to RUN. Perform the Milestone 3 acceptance checks.

Stage A fix in ISPSoft (2026-11-13), in order, offline until step 7; texts under `Artifacts and Notes`:

1. `FB_IOMonitor`: add the local symbols `LastStep : WORD := 0`, `LastProg : WORD := 0`, `LastResult : WORD := 0` and replace the whole body by "FB_IOMonitor body".
2. `FB_RunSnapShot`: replace the block after `// write responce` by "FB_RunSnapShot reply".
3. `PCHandler`: delete the line `VarPending := VarMonitor.VarPending,` from the `TxArbiter(…)` call and the line `VarMonitor(BuildNow := TxArbiter.GrantVar);`, then delete the local symbol `VarMonitor`.
4. `FB_TxArbiter`: delete the line `GrantVar  := FALSE;` and the four lines `ELSIF VarPending THEN` … `StartSend   := TRUE;` of the VarPending branch, then delete the local symbols `VarPending` and `GrantVar`.
5. Delete the function block `FB_VarMonitor` (project tree, right-click > Delete).
6. DUT `E_CHGTYPE`: delete `CELL_STATE := 16#00D3` and the comma after `IO := 16#00D0`. Compile (Compile > Compile all); expected 0 errors. A remaining use of a deleted name is reported by name; remove that use.
7. Connect, switch the PLC to STOP, download the program, switch to RUN.

PC commands of Milestone 4 (run by the user, not by the agent). Build in the existing build directory from Qt Creator (target `robocrap`), or from a terminal in the build directory:

    cmake --build . --target robocrap

Expected end of output: `[100%] Built target robocrap`. Start the application, open Settings > Network, connect the PLC, open the PLC tool window.


## Validation and Acceptance

The plan is accepted when, with the KR C4 in AUT EXT and the robot at HOME, programs 2, 3 and 4 in the RoboCrap PLC panel each produce the result STARTED, X1.6 APPL_RUN ON for about 1, 2 or 3 s and then the state "finished"; an interruption by the smartPAD STOP key shows "interrupted" and Resume ends with RESUMED and "finished"; program 1 with `StepLimit` = 10 runs the first three calibration touches and `python/rsi_touches.py` finds them; a start is impossible outside AUT EXT (Run disabled in T1, and the forced start of Milestone 3 ends with AUTEXT_LOST without switching any Y1 output); and the smartPAD shows POV 10 % during every AUT EXT run, with no motion faster than in the T1 runs of the calibration.

The other paths, by milestone: module 1 WRITE_IO is rejected with `PLC response error: cmd=181, err=1, code=61191` (Milestone 3); three Disconnect/Connect cycles succeed and no reply timeout appears in one idle minute (Milestone 4); Abort ends a start sequence with ABORTED, a start with E-STOP pressed ends with TIMEOUT after 5 s, Run is disabled while the robot is away from HOME after a `StepLimit` run (Milestone 5). The PLC refusals of RUN while APPL_RUN is ON or away from HOME cannot be triggered from the panel, which disables Run in those states; they are covered by reading `FB_RunSetVar` against this plan.


## Idempotence and Recovery

Every ISPSoft step can be repeated: the DUT texts and FB bodies replace the old ones. If the download misbehaves, switch the PLC to STOP and download the Milestone 1 backup of `RoboCrapPLC`; the PC application's existing functions do not depend on any new PLC code. On the KR C4, the archived `resources/files/KR C4/R1/cell.src` and `STEU/Mada/$machine.dat` (taken before Milestone 2) restore the old state, together with the pre-Milestone 2 values listed under `Surprises & Discoveries` (git history also holds the 100 % line of `RSI_Probe_Calibration.src`); setting `$I_O_ACT` back to 1026 disables the interface completely. The PC changes are confined to `src/devices/plc`, `src/devices/include/devices/plc`, `src/application/qml/Views/Dashboard/DeltaPanel` and `src/application/CMakeLists.txt` (one QML file entry); reverting those files restores the old behaviour. The wiring of Milestone 2 is inert while `$I_O_ACT` = 1026 or the KR C4 is not in AUT EXT.


## Artifacts and Notes

File `resources/files/AutExt/CELL.src` (written by the agent in Milestone 2; it replaces `R1\CELL.SRC` on the controller). Compared with the archived `resources/files/KR C4/R1/cell.src` it adds the APPL_RUN reset in the INIT fold (2026-11-13), replaces the inline `PTP HOME` by plain KRL with `INIT_PTP_VEL ( )` (2026-11-14), sets `$OV_PRO=100` before every program number (2026-11-14; before: `AutExtOv` = 10) and replaces the CASE section; program k uses PGNO = k − 1.

    &ACCESS RVP
    &REL 3
    &COMMENT HANDLER on external automatic (RoboCrap: programs 1..4 = PGNO 0..3)
    DEF  CELL ( )
      ;EXT EXAMPLE1 ( )
      ;EXT EXAMPLE2 ( )
      ;EXT EXAMPLE3 ( )
      EXT RSI_Probe_Calibration ( )
      
      ;FOLD INIT
      DECL CHAR DMY[3]
      DMY[]="---"
      ; P00 clears APPL_RUN only in INIT_EXT and at the next number request, so a program that was
      ; stopped and then deselected leaves it ON. The BCO run of a newly selected CELL executes this.
      $OUT[APPL_RUN]=FALSE
      ;ENDFOLD (INIT)
    	
      ;FOLD BASISTECH INI
      GLOBAL INTERRUPT DECL 3 WHEN $STOPMESS==TRUE DO IR_STOPM ( )
      INTERRUPT ON 3 
      BAS (#INITMOV,0 )
      ;ENDFOLD (BASISTECH INI)
    	
      ;FOLD CHECK HOME
      $H_POS=XHOME
      IF CHECK_HOME==TRUE THEN
        P00 (#CHK_HOME,#PGNO_GET,DMY[],0 ) ;Testing Home-Position 
      ENDIF
      ;ENDFOLD (CHECK HOME)
    	
      ; PTP HOME at the operating-mode velocity of INIT_PTP_VEL (no inline form: its Vel would
      ; overwrite that velocity).
      $H_POS=XHOME
      PDAT_ACT=PDEFAULT
      BAS (#PTP_DAT )
      FDAT_ACT=FHOME
      BAS (#FRAMES )
      INIT_PTP_VEL ( )
      PTP  XHOME
    	
      ;FOLD AUTOEXT INI
      P00 (#INIT_EXT,#PGNO_GET,DMY[],0 ) ; Initialize extern mode
      ;ENDFOLD (AUTOEXT INI)
      LOOP
        ; Programs set their own velocities (INIT_PTP_VEL, $VEL.CP) and run at 100 % override in
        ; AUT EXT; re-applied before every program number, so a smartPAD change only lasts one program.
        $OV_PRO=100
        P00 (#EXT_PGNO,#PGNO_GET,DMY[],0 )
        SWITCH  PGNO ; Select with Programnumber

        CASE 0
          P00 (#EXT_PGNO,#PGNO_ACKN,DMY[],0 ) ; Reset Progr.No.-Request
          RSI_Probe_Calibration ( ) ; program 1: probe sphere calibration

        CASE 1
          P00 (#EXT_PGNO,#PGNO_ACKN,DMY[],0 ) ; Reset Progr.No.-Request
          WAIT SEC 5 ; program 2: placeholder without motion

        CASE 2
          P00 (#EXT_PGNO,#PGNO_ACKN,DMY[],0 ) ; Reset Progr.No.-Request
          WAIT SEC 5 ; program 3: placeholder without motion

        CASE 3
          P00 (#EXT_PGNO,#PGNO_ACKN,DMY[],0 ) ; Reset Progr.No.-Request
          WAIT SEC 5 ; program 4: placeholder without motion

        DEFAULT
          P00 (#EXT_PGNO,#PGNO_FAULT,DMY[],0 )
        ENDSWITCH
      ENDLOOP
    END

Files `resources/files/AutExt/INIT_PTP_VEL.src` and `INIT_PTP_VEL.dat` (2026-11-14; on the controller in `KRC:\R1\Program`): the operating-mode PTP velocity (Decision Log), in the controller's module template form (`resources/files/KR C4/Template/vorgabe.*`) without INI fold and HOME moves, because it is a called subprogram.

    &ACCESS RV
    &PARAM EDITMASK = *
    &PARAM TEMPLATE = C:\KRC\Roboter\Template\vorgabe
    DEF INIT_PTP_VEL( )
      ; Sets the PTP velocity $VEL_AXIS[1..6] (% of $VEL_AXIS_MA) for the current operating mode;
      ; every following plain PTP uses it until something overwrites $VEL_AXIS (BAS (#INITMOV),
      ; BAS (#PTP_PARAMS) or BAS (#VEL_PTP), an inline-form PTP). Call it after those.
      ;   T1: 100 %, limited by the controller to 250 mm/s.
      ;   AUT EXT, AUT, T2: PtpVelAuto from INIT_PTP_VEL.dat.
      ; Velocities are fixed when the advance run plans a motion, so after an operating-mode change
      ; a stopped program is restarted from CELL, never resumed (PLC ResumeBlocked).
      ; No INI fold and no PTP HOME: this subprogram is called from other programs, and BAS (#INITMOV)
      ; would reset their motion parameters.
      DECL INT I
      WAIT SEC 0 ; advance run stop: the mode is read when the robot reaches this line
      FOR I=1 TO 6
        IF $MODE_OP==#T1 THEN
          $VEL_AXIS[I]=100
        ELSE
          $VEL_AXIS[I]=PtpVelAuto
        ENDIF
      ENDFOR
    END

    &ACCESS RV
    &PARAM EDITMASK = *
    &PARAM TEMPLATE = C:\KRC\Roboter\Template\vorgabe
    DEFDAT INIT_PTP_VEL
    ;FOLD EXTERNAL DECLARATIONS;%{PE}%MKUKATPBASIS,%CEXT,%VCOMMON,%P
    ;FOLD BASISTECH EXT;%{PE}%MKUKATPBASIS,%CEXT,%VEXT,%P
    EXT  BAS (BAS_COMMAND  :IN,REAL  :IN )
    DECL INT SUCCESS
    ;ENDFOLD (BASISTECH EXT)
    ;FOLD USER EXT;%{E}%MKUKATPUSER,%CEXT,%VEXT,%P
    ;Make your modifications here

    ;ENDFOLD (USER EXT)
    ;ENDFOLD (EXTERNAL DECLARATIONS)
    ; PTP velocity (% of the maximum axis velocity $VEL_AXIS_MA) outside T1: AUT EXT, AUT and T2.
    ; 5 % gives A1 at most 11 deg/s (rated 220 deg/s).
    DECL INT PtpVelAuto=5
    ENDDAT

Change in `resources/files/rsi/Calibration/RSI_Probe_Calibration.src` (2026-11-14): the inline `PTP HOME` becomes plain KRL with `INIT_PTP_VEL ( )` after `BAS (#PTP_PARAMS,20 )`, and the loop before the steps sets only `$ACC_AXIS[1..6]` = 20 (no longer `$VEL_AXIS`):

    ; PTP HOME at the operating-mode velocity of INIT_PTP_VEL, which also serves every later PTP
    ; (no inline form: its Vel would overwrite that velocity).
    $BWDSTART = FALSE
    PDAT_ACT=PDEFAULT
    FDAT_ACT=FHOME
    BAS (#PTP_PARAMS,20 )
    INIT_PTP_VEL ( )
    $H_POS=XHOME
    PTP  XHOME

DUT E_VARTYPE (the codes must equal `PlcMessageManager::VAR_TYPE` on the PC):

    TYPE E_VARTYPE :
    (
      RUN_PROG     := 16#00C0,
      ROBOT_RESUME := 16#00C3,
      ROBOT_ABORT  := 16#00C5
    );
    END_TYPE

DUT E_CODE, members added after `LEN_MISMATCH := 16#EE08` (put a comma after `16#EE08`):

      BUSY        := 16#EE09,
      BAD_VALUE   := 16#EE0A,
      NOT_ALLOWED := 16#EE0B

DUT E_RSTEP and E_RRESULT:

    TYPE E_RSTEP :
    (
      RS_IDLE   := 0,
      RS_PREP   := 1,
      RS_START  := 2,
      RS_SELECT := 3
    );
    END_TYPE

    TYPE E_RRESULT :
    (
      RR_NONE        := 0,
      RR_STARTED     := 1,
      RR_RESUMED     := 2,
      RR_TIMEOUT     := 3,
      RR_ABORTED     := 4,
      RR_AUTEXT_LOST := 5
    );
    END_TYPE

Main Table additions (Class VAR):

    AUTEXT_ACT   X1.0   BOOL   KUKA $I_O_ACTCONF ($OUT[1]): AUT EXT selected and interface active
    PERI_RDY     X1.2   BOOL   KUKA $PERI_RDY ($OUT[3])
    STOPMESS     X1.3   BOOL   KUKA $STOPMESS ($OUT[4])
    PRO_ACT      X1.4   BOOL   KUKA $PRO_ACT ($OUT[5])
    PGNO_REQ     X1.5   BOOL   KUKA PGNO_REQ ($OUT[6])
    APPL_RUN     X1.6   BOOL   KUKA APPL_RUN ($OUT[7])
    IN_HOME      X1.7   BOOL   KUKA $IN_HOME ($OUT[8])
    EXT_START    Y1.0   BOOL   KUKA $EXT_START ($IN[1])
    DRIVES_ON    Y1.1   BOOL   KUKA $DRIVES_ON ($IN[2])
    CONF_MESS    Y1.2   BOOL   KUKA $CONF_MESS ($IN[3])
    PGNO_B0      Y1.3   BOOL   KUKA PGNO bit 0 ($IN[4])
    PGNO_B1      Y1.4   BOOL   KUKA PGNO bit 1 ($IN[5])
    JobPgno      auto   WORD   program 1..4 of the current job, 0 for resume
    RobotStep    auto   WORD   E_RSTEP, written by RobotIo
    RobotResult  auto   WORD   E_RRESULT, written by RobotIo
    CmdRun       auto   BOOL   set by FB_RunSetVar, cleared by RobotIo
    CmdResume    auto   BOOL   set by FB_RunSetVar, cleared by RobotIo
    CmdAbort     auto   BOOL   set by FB_RunSetVar, cleared by RobotIo
    Fail         auto   BOOL   read by TR_Fail, written by RobotIo

FB_WriteTxRespErr body (payload starts at byte 6: command, error, then the 16-bit code in word 4 = bytes 8–9, as the PC parser expects):

    wTxHeader(tid:=tid, type:=E_TYPE.RESP_ERR, paylen:=4);
    wTxByte(idx:=6, val:=cmd);
    wTxByte(idx:=7, val:=err);
    wTxWord(idx:=4, val:=code);

FB_RunSetVar body; local symbols added: `wRespErr : FB_WriteTxRespErr`, `accepted : BOOL`, `code : WORD`.

    rRxByte(idx:=10, val=>variable);
    rRxByte(idx:=11, val=>attr);
    accepted := FALSE;
    code := E_CODE.BAD_CMD;

    CASE variable OF
      E_VARTYPE.RUN_PROG:
        IF (attr < 1) OR (attr > 4) THEN
          code := E_CODE.BAD_VALUE;
        ELSIF RobotStep <> E_RSTEP.RS_IDLE THEN
          code := E_CODE.BUSY;
        ELSIF (NOT AUTEXT_ACT) OR (NOT IN_HOME) OR APPL_RUN THEN
          code := E_CODE.NOT_ALLOWED;  // only in AUT EXT, at HOME, with no program selected and stopped
        ELSE
          JobPgno := attr;
          CmdRun := TRUE;
          accepted := TRUE;
        END_IF;
      E_VARTYPE.ROBOT_RESUME:
        IF RobotStep <> E_RSTEP.RS_IDLE THEN
          code := E_CODE.BUSY;
        ELSIF (NOT AUTEXT_ACT) OR PRO_ACT OR (NOT APPL_RUN) THEN
          code := E_CODE.NOT_ALLOWED;  // only in AUT EXT and only a stopped application program
        ELSE
          JobPgno := 0;
          CmdResume := TRUE;
          accepted := TRUE;
        END_IF;
      E_VARTYPE.ROBOT_ABORT:
        IF RobotStep = E_RSTEP.RS_IDLE THEN
          code := E_CODE.NOT_ALLOWED;  // no start sequence to abort
        ELSE
          CmdAbort := TRUE;
          accepted := TRUE;
        END_IF;
    END_CASE;

    IF accepted THEN
      wTxHeader(type:=E_TYPE.RESP_OK, tid:=tid, paylen:=4);
      wTxByte(idx:=6, val:=E_CMD.SET_VAR);
      wTxByte(idx:=7, val:=0);
      wTxByte(idx:=8, val:=variable);
      wTxByte(idx:=9, val:=attr);
    ELSE
      wRespErr(tid:=tid, cmd:=E_CMD.SET_VAR, err:=E_ERR.BAD_REQUEST, code:=code);
    END_IF;

FB_RunWriteIo guard (module 1 belongs to the robot interface and the probe copy):

    rRxByte(idx:=13, val=>module);
    IF module = 1 THEN
      wRespErr(tid:=tid, cmd:=E_CMD.WRITE_IO, err:=E_ERR.BAD_REQUEST, code:=E_CODE.BAD_MOD);
    ELSE
      // the existing body from rRxWord(idx:=5, val=>dev); to the last wTxByte, unchanged
    END_IF;

FB_CmdMonitor dispatch (replaces the existing `IF BuildNow AND Pending THEN … END_IF;`; `RxBytes` holds `Rx[0]`, the number of received bytes):

    IF BuildNow AND Pending THEN
      IF (RxHeader.magic XOR 16#AA55) <> 0 THEN  // a direct <> 16#AA55 failed: error 1003 at LD (2026-11-13)
        wRespErr(tid:=RxHeader.tid, cmd:=Cmd, err:=E_ERR.BAD_REQUEST, code:=E_CODE.BAD_MAGIC);
      ELSIF RxHeader.ver <> 1 THEN
        wRespErr(tid:=RxHeader.tid, cmd:=Cmd, err:=E_ERR.BAD_REQUEST, code:=E_CODE.BAD_VER);
      ELSIF RxHeader.type <> E_TYPE.REQ THEN
        wRespErr(tid:=RxHeader.tid, cmd:=Cmd, err:=E_ERR.BAD_REQUEST, code:=E_CODE.BAD_TYPE);
      ELSIF RxBytes < (6 + RxHeader.len) THEN
        wRespErr(tid:=RxHeader.tid, cmd:=Cmd, err:=E_ERR.BAD_REQUEST, code:=E_CODE.LEN_MISMATCH);
      ELSE
        CASE Cmd OF
          // the existing branches READ_IO … SET_VAR, unchanged
        ELSE
          wRespErr(tid:=RxHeader.tid, cmd:=Cmd, err:=E_ERR.BAD_REQUEST, code:=E_CODE.BAD_CMD);
        END_CASE;
      END_IF;
      Pending := FALSE;
    END_IF;

FB_IOMonitor connection edge (a new PC connection gets the IO state at once):

    IF TCP_CONNECTED AND NOT WasConnected THEN
      IoPending := TRUE;
    END_IF;
    WasConnected := TCP_CONNECTED;

FB_IOMonitor body (stage A fix, 2026-11-13; it replaces `FB_VarMonitor`, whose Milestone 3 body is in git history). Local symbols added: `LastStep : WORD := 0`, `LastProg : WORD := 0`, `LastResult : WORD := 0`. Payload: change type, X1, Y1, X2, Y2, step, program, result.

    IF SM402 THEN
        IoPending := TRUE;
    END_IF;

    // NEW PC CONNECTION: SEND THE CURRENT STATE AT ONCE
    IF TCP_CONNECTED AND NOT WasConnected THEN
      IoPending := TRUE;
    END_IF;
    WasConnected := TCP_CONNECTED;

    IF IoChangeByCmd THEN
      LastY1 := Y1;
      LastY2 := Y2;
      IoChangeByCmd := FALSE;
    END_IF;

    // IO OR START-SEQUENCE CHANGE (RobotStep, JobPgno and RobotResult are written by RobotIo)
    AnyChange := (X1 <> LastX1)
              OR (Y1 <> LastY1)
              OR (X2 <> LastX2)
              OR (Y2 <> LastY2)
              OR (RobotStep <> LastStep)
              OR (JobPgno <> LastProg)
              OR (RobotResult <> LastResult);

    IF AnyChange THEN
      IoPending := TRUE;
    END_IF;

    IF BuildNow AND IoPending THEN
      wTxHeader(type:=E_Type.CHG,
                tid:=0,
                paylen:=8);
      wTxByte(idx:=6,  val:=E_CHGTYPE.IO);
      wTxByte(idx:=7,  val:=X1);
      wTxByte(idx:=8,  val:=Y1);
      wTxByte(idx:=9,  val:=X2);
      wTxByte(idx:=10, val:=Y2);
      wTxByte(idx:=11, val:=RobotStep);
      wTxByte(idx:=12, val:=JobPgno);
      wTxByte(idx:=13, val:=RobotResult);

      LastX1 := X1;
      LastY1 := Y1;
      LastX2 := X2;
      LastY2 := Y2;
      LastStep := RobotStep;
      LastProg := JobPgno;
      LastResult := RobotResult;

      IoPending := FALSE;
    END_IF;

FB_RunSnapShot reply (stage A fix, 2026-11-13; replaces the block after `// write responce`; payload: command, status, X1, Y1, X2, Y2, step, program, result):

    wTxHeader(type:=E_TYPE.RESP_OK,
              tid:=tid,
              paylen:=9);
    wTxByte(idx:=6,  val:=E_CMD.SNAPSHOT);
    wTxByte(idx:=7,  val:=0); // STATUS
    wTxByte(idx:=8,  val:=x_1);
    wTxByte(idx:=9,  val:=y_1);
    wTxByte(idx:=10, val:=x_2);
    wTxByte(idx:=11, val:=y_2);
    // start sequence, as in the IO change frame: the PC heartbeat repairs a lost change frame
    wTxByte(idx:=12, val:=RobotStep);
    wTxByte(idx:=13, val:=JobPgno);
    wTxByte(idx:=14, val:=RobotResult);

PCHandler send (replaces everything after `IF TxArbiter.StartSend THEN … END_IF;`, and the error-clearing block of the earlier revision if it was entered; 2026-11-13):

    // DATA SENT TO PC (or the send failed / the connection is gone): D100 is free again.
    // Checked before a new SSEND, so it never reads the flags of the scan that issued that SSEND.
    IF SendStarted & (((NOT TCP_SENDING) & (TCP_SENT OR TCP_ERROR)) OR (NOT TCP_CONNECTED)) THEN
      WANTTOSEND  := FALSE;
      SendStarted := FALSE;
    END_IF;

    // SEND DATA TO PC: issue SSEND once per frame.
    // SM1272/SM1277 are read-only and refreshed only when a socket instruction runs (AS manual),
    // so SSEND must not wait for TCP_ERROR: running it is what refreshes the latched error flag.
    IF WANTTOSEND & TCP_CONNECTED & (NOT TCP_SENDING) & (NOT SendStarted) THEN
      SSEND(1, 1);
      SendStarted := TRUE;
    END_IF;

FB_CmdMonitor, first `IF` (the request intake): without the `TCP_ERROR` condition:

    // No TCP_ERROR condition: SM1277 is latched and refreshed only by socket instructions.
    IF TCP_CONNECTED & (Rx[0] <> 0) AND (NOT Pending) THEN

RobotIo body. It runs before `RobotHandler`, so the step flags `X` (step active) and `T` (time in the step, ms) are those left by the previous scan; all KUKA outputs are written only here, and only while `AUTEXT_ACT` is ON.

    // Clear the one-shot commands once the sequence has started; a new job starts without a result.
    IF RobotHandler.ST_PREP.X THEN
      CmdRun := FALSE;
      CmdResume := FALSE;
      RobotResult := E_RRESULT.RR_NONE;
    END_IF;

    StepTimeout := (RobotHandler.ST_PREP.X AND (RobotHandler.ST_PREP.T >= 5000))
                OR (RobotHandler.ST_START.X AND (RobotHandler.ST_START.T >= 30000))
                OR (RobotHandler.ST_SELECT.X AND (RobotHandler.ST_SELECT.T >= 2000));

    IF RobotHandler.ST_IDLE.X THEN
      CmdAbort := FALSE;
      Fail := FALSE;
    ELSE
      Fail := CmdAbort OR StepTimeout OR (NOT AUTEXT_ACT);
      IF CmdAbort THEN
        RobotResult := E_RRESULT.RR_ABORTED;
      ELSIF NOT AUTEXT_ACT THEN
        RobotResult := E_RRESULT.RR_AUTEXT_LOST;
      ELSIF StepTimeout THEN
        RobotResult := E_RRESULT.RR_TIMEOUT;
      END_IF;
    END_IF;

    // Results of the two successful exits (the same conditions as TR_Running and TR_Acked).
    // TR_Running ends a resume (JobPgno = 0) or a new job whose number CELL took already in ST_START.
    IF RobotHandler.ST_START.X AND PRO_ACT AND APPL_RUN THEN
      IF JobPgno = 0 THEN
        RobotResult := E_RRESULT.RR_RESUMED;
      ELSE
        RobotResult := E_RRESULT.RR_STARTED;
      END_IF;
    END_IF;
    IF RobotHandler.ST_SELECT.X AND APPL_RUN AND (NOT PGNO_REQ) THEN
      RobotResult := E_RRESULT.RR_STARTED;
    END_IF;

    IF RobotHandler.ST_PREP.X THEN
      RobotStep := E_RSTEP.RS_PREP;
    ELSIF RobotHandler.ST_START.X THEN
      RobotStep := E_RSTEP.RS_START;
    ELSIF RobotHandler.ST_SELECT.X THEN
      RobotStep := E_RSTEP.RS_SELECT;
    ELSE
      RobotStep := E_RSTEP.RS_IDLE;
    END_IF;

    // KUKA outputs (Fig. 6-24, P00): drives-on pulse, acknowledge pulse, start edges, program number.
    // The number is presented already in ST_START: a CELL stopped inside P00 #PGNO_GET reads the PGNO
    // bits as soon as the continuing $EXT_START is TRUE, so they must be valid 0.1 s before that edge.
    DRIVES_ON := AUTEXT_ACT AND RobotHandler.ST_PREP.X AND (RobotHandler.ST_PREP.T < 200);
    CONF_MESS := AUTEXT_ACT AND RobotHandler.ST_PREP.X
                 AND (RobotHandler.ST_PREP.T >= 300) AND (RobotHandler.ST_PREP.T < 500);
    EXT_START := AUTEXT_ACT AND ((RobotHandler.ST_START.X AND (NOT PRO_ACT) AND (RobotHandler.ST_START.T >= 100))
                 OR (RobotHandler.ST_SELECT.X AND (RobotHandler.ST_SELECT.T >= 100)));
    PGNO_B0 := AUTEXT_ACT AND (JobPgno <> 0) AND (RobotHandler.ST_START.X OR RobotHandler.ST_SELECT.X)
               AND (((JobPgno - 1) AND 16#0001) <> 0);
    PGNO_B1 := AUTEXT_ACT AND (JobPgno <> 0) AND (RobotHandler.ST_START.X OR RobotHandler.ST_SELECT.X)
               AND (((JobPgno - 1) AND 16#0002) <> 0);

RobotHandler transitions (each a Structured Text transition program):

    TR_Run      := CmdRun OR CmdResume;
    TR_Prepared := PERI_RDY AND (NOT STOPMESS) AND (RobotHandler.ST_PREP.T >= 500);
    TR_Running  := PRO_ACT AND APPL_RUN;
    TR_Request  := PRO_ACT AND PGNO_REQ AND (JobPgno <> 0);
    TR_Acked    := APPL_RUN AND (NOT PGNO_REQ);
    TR_Fail     := Fail;

What the sequence does, with the actual timings: in ST_PREP `$DRIVES_ON` is high for the first 0.2 s and `$CONF_MESS` from 0.3 s to 0.5 s; from 0.5 s on, the step ends as soon as the drives are on and no stop message is pending. In ST_START the two PGNO bits already show k − 1 (for a job) and `$EXT_START` is high from 0.1 s until `$PRO_ACT` rises; for a new job CELL then asks for a number (PGNO_REQ, after `$EXT_START` fell) and `TR_Request` fires; if CELL already waits for a number, no edge is given and `TR_Request` fires at once; for a resume the stopped program continues, `$PRO_ACT` rises while APPL_RUN is still ON, and `TR_Running` ends the sequence. In ST_SELECT the PGNO bits stay and `$EXT_START` rises after 0.1 s; P00 reads the bits as soon as it sees `$EXT_START` TRUE; APPL_RUN with PGNO_REQ FALSE ends the sequence. A timeout (5 s, 30 s, 2 s; proposed values to tune), an Abort or the loss of AUT EXT ends any non-idle step through `TR_Fail`.


## Interfaces and Dependencies

No new libraries. C++17, Qt 6.8 QtCore/QtNetwork (QTimer, QTcpSocket), existing project types.

In `src/devices/include/devices/plc/plcmessagemanager.h`:

    enum VAR_TYPE : quint8 {
      RUN_PROG     = 0xC0,
      ROBOT_RESUME = 0xC3,
      ROBOT_ABORT  = 0xC5
    };  // replaces START_CELL/SFY_OK; equal to the PLC DUT E_VARTYPE

    enum ROBOT_STEP : quint8 { RS_IDLE = 0, RS_PREP = 1, RS_START = 2, RS_SELECT = 3 };
    enum ROBOT_RESULT : quint8 { RR_NONE = 0, RR_STARTED = 1, RR_RESUMED = 2, RR_TIMEOUT = 3,
                                 RR_ABORTED = 4, RR_AUTEXT_LOST = 5 };
    // the enum CELL (IDLE/RUN/FIN) is removed; each new enum gets Q_ENUM like the existing ones

    ParseResult robotProgramRequest(const QVariantMap& args) const;
    // args {"program": 1..4} -> request map {cmd: SET_VAR, var: RUN_PROG, attr: program};
    // error BAD_DATA for a missing, non-integer or out-of-range program (reuse readUInt8)

`isValidVar` accepts the three new codes. `parseStateChange` case `IOs`: payload size 8 (change type, X1, Y1, X2, Y2, step, program, result); `parseRespOk` case `SNAPSHOT`: payload size 9 (command, status, X1, Y1, X2, Y2, step, program, result); both publish `x1`, `y1`, `x2`, `y2` (bit lists) and `robotStep`, `robotProgram`, `robotResult` (integers); any other size is `BAD_PLEN`. There is no `CELL_STATE` change type (removed 2026-11-13, Decision Log).

In `src/devices/include/devices/plc/plcdevice.h` (`PlcDevice`), new public invokables, names unique among all devices' invokables:

    Q_INVOKABLE void runRobotProgram(const QVariantMap& args);  // {"program": 1..4}
    Q_INVOKABLE void resumeRobot();
    Q_INVOKABLE void abortRobotStart();  // aborts only the PLC start sequence, never a running program

Behaviour required in `src/devices/plc/plcdevice.cpp`: `writeMessage` enqueues the request; a request is written only when no reply is pending (one request in flight), so the PLC's single receive buffer never holds two frames. A single-shot reply timer (1000 ms, created in `startDevice` on the I/O thread with `this` as parent and deleted in `stopDevice`, like the socket) logs `[ERROR] PLC_AS332T: PLC reply timeout for transaction <tid>`, drops the pending request and sends the next one. A 2000 ms periodic heartbeat timer enqueues a SNAPSHOT when the socket is connected and nothing is queued or pending. `disconnect`, `stopDevice` and the transition to `UnconnectedState` clear the queue, stop both timers and publish `robotStep` 0, `robotResult` 0, `robotProgram` 0, `robotProgramState` 0. The existing transaction-id bookkeeping (`m_pend`, `availableTid`) stays; with one request in flight `m_pend` holds at most one entry. RESP_ERR codes 0xEE09 BUSY, 0xEE0A BAD_VALUE and 0xEE0B NOT_ALLOWED appear in the existing error log line with their numbers.

Robot program state, published as `robotProgramState` (0 unknown, 1 running, 2 finished, 3 interrupted) whenever new X1 bits arrive (SNAPSHOT reply or IO change). It is computed by one helper in the unnamed namespace of `plcdevice.cpp` that returns the next state from the current state and the X1 bit list (two parameters, result by return value, no lambda). With APPL_RUN = X1.6, `$PRO_ACT` = X1.4 and PGNO_REQ = X1.5 the rules are, in this order: APPL_RUN and `$PRO_ACT` ON give running; APPL_RUN ON and `$PRO_ACT` OFF give interrupted (P00 keeps APPL_RUN until CELL asks for the next number); from running or interrupted, APPL_RUN OFF with `$PRO_ACT` and PGNO_REQ ON gives finished; from running, `$PRO_ACT` OFF with APPL_RUN OFF gives interrupted (the behaviour the KSS figures draw); otherwise the state is unchanged. `PlcDevice` keeps the current state as one member.

QML. In `DeltaPanel.qml`, module 1 (`moduleAP_P`): `xTags` `["AUT EXT", "N/D", "PERI_RDY", "STOPMESS", "PRO_ACT", "PGNO_REQ", "APPL_RUN", "IN_HOME"]`, `xPlugged` `[1, 0, 1, 1, 1, 1, 1, 1]`, `yTags` `["EXT_START", "DRV_ON", "CONF_MESS", "PGNO0", "PGNO1", "N/D", "N/D", "PROBE"]`, `yPlugged` `[1, 1, 1, 1, 1, 0, 0, 1]`, `yDisplayOnly` `[0, 1, 2, 3, 4, 7]`. New `src/application/qml/Views/Dashboard/DeltaPanel/RobotStartPanel.qml` (listed in `src/application/CMakeLists.txt` next to the other DeltaPanel files) placed in `DeltaPanel.qml` below the modules, taking `required property Backend.DeviceRunner plc`: a `QxComboBox` with programs 1 … 4, `QxButton`s Run, Resume, Abort and one status line. The enable rules repeat the PLC's checks: Run (`plc.invoke("runRobotProgram", {"program": n})`) when `robotStep` is 0, X1.0 ON, X1.7 ON and X1.6 OFF; Resume (`resumeRobot`) when `robotStep` is 0, X1.0 ON, X1.6 ON, X1.4 OFF, X1.3 OFF and `robotResumeBlocked` FALSE (both since 2026-11-14); Abort (`abortRobotStart`) when `robotStep` is 1, 2 or 3. The status line shows "AUT EXT off" while X1.0 is OFF, otherwise the sequence step, the last result and the program state, in one line of fixed height (elided, no wrapping), so changing it never moves other controls (`AGENTS.md` "Mandatory interface stability"). Before editing, inspect `qml/Modules/Components/QxComboBox.qml`, `QxButton.qml` and `QxPanel.qml` for their properties and sizing, as the QML contract in `docs/context/AGENT_CONTRACTS.md` requires.

The PLC side depends on ISPSoft and the AS332T firmware supporting SOPEN mode 2 (firmware V1.06.30 or later, already in use by the current project).

Revision note (2026-11-12): first version of the plan, written from the agreed proposal; adds the RESP_ERR byte-offset fix, `$STOPMESS` in `TR_Prepared` and the task order PCHandler → RobotIo → RobotHandler, which the proposal did not contain.

Revision note (2026-11-14, seventh): Resume after smartPAD acknowledgement only; programmed speeds (override 100 % in AUT EXT, `INIT_PTP_VEL`, `ResumeBlocked`) replace the 10 % override; stage A and B results; RSI power settings; Milestone 6 documents. The plan is complete.

Revision note (2026-11-13, sixth): after stage A A1–A3 — the start-sequence state moves into the IO change frame and the SNAPSHOT reply and `FB_VarMonitor` is removed (lost change frames), `CELL.src` clears a stale APPL_RUN in its INIT fold, A3 names the STOP key, and stage A gains the APPL_RUN recovery.

Revision note (2026-11-13, fifth): the error-clearing block is withdrawn (SM1277 is read-only and latched); `PCHandler` send logic and `FB_CmdMonitor` intake no longer depend on TCP_ERROR, so an SSEND refreshes the socket flags.

Revision note (2026-11-13, fourth): `PCHandler` clears a latched socket error flag (SM1277) and retries the waiting frame; found when all requests timed out after a download.

Revision note (2026-11-13, third): `RobotIo` corrected before Milestone 5 — PGNO bits valid already in ST_START, `$EXT_START` in ST_START from 0.1 s, `TR_Running` exit of a job reported as STARTED — because P00 reads the number when a CELL stopped inside `#PGNO_GET` is continued; stage A gains the stopped-CELL check and the no-job precondition.

Revision note (2026-11-13, second): speed limit for AUT EXT on the user's request — `CELL.src` (now a full file instead of `CELL_CASES.txt`) sets the program override to 10 % before its first motion and before every program number, CELL's and the calibration's PTP HOME are lowered from 100 % to 20 %; Milestone 2 agent part done; Milestone 2/5 acceptance checks the POV display; stage B no longer asks for a manual override.

Revision note (2026-11-13): Milestone 1 completed and recorded. Changes: program 1 is now the existing `RSI_Probe_Calibration` and programs 2–4 are `WAIT SEC` placeholders in CELL (user's choice; the separate `AUTEXT_TEST.src` is dropped); a start is possible only in AUT EXT (user requirement: PLC guard on `AUTEXT_ACT`, outputs gated by it, result AUTEXT_LOST, panel rules); RUN always requires `$IN_HOME` because the calibration program can end near the sphere and starts with `PTP HOME`; after reading P00, APPL_RUN is known to stay TRUE until CELL's next number request, so Resume is recognized by APPL_RUN without `$PRO_ACT`, `TR_Running` requires `$PRO_ACT`, `TR_Request` requires a program number, and the PC state rules cover both P00 and figure behaviour; the Milestone 3 PLC-only check now proves the AUT EXT gate instead of a timeout; Milestone 5 is split into a motion-free stage A and a calibration stage B.
