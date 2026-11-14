# Starting KRL programs from RoboCrap through the PLC (AUT EXT) — proposal

Status (2026-11-13): design study, implemented and refined by `plans/PLC_AUT_EXT_PROGRAM_START_EXECPLAN.md`, which wins where the two differ; the PLC side is in `ISPSoft project.md` of this folder. Differences made during implementation: a start is possible only in AUT EXT (PLC guard on `$I_O_ACTCONF`, all KUKA outputs gated by it, result AUTEXT_LOST); a new job always requires `$IN_HOME`; Resume requires APPL_RUN without `$PRO_ACT` (P00 keeps APPL_RUN until CELL asks for the next number); `TR_Running` requires `$PRO_ACT` and `TR_Request` a program number; CELL.SRC fixes the program override at 10 % and no PTP is programmed above 20 %; program 1 is `RSI_Probe_Calibration`, programs 2–4 are `WAIT SEC` placeholders.

High-level proposal; nothing here is implemented. Inputs: KSS 8.3 SI §6.17 "Configuring Automatic External" and §8.11, KUKA Robot Programming 3 (KSS 8.2) ch. 8, AS Programming Manual §6.22 (SOPEN/SSEND/SCLOSE/SCONF, SM1270–SM1277), ISPSoft User Manual §3.1.4.12 (Socket) and Chapter 14 (SFC), `docs/plc_project_snapshot`, `src/devices/plc`, `docs/context/HARDWARE_SETUP.md`, `ARCHITECTURE.md` §1 and the controller copy `resources/files/KR C4` (17 Oct).

The KSS signal diagrams Fig. 6-23 … 6-28 were read from PNG copies in `manuals/KUKA` (6-23.png … 6-28.png).

Diagram: [RobotHandler_SFC.drawio](RobotHandler_SFC.drawio).

## 1. Findings in the current project

Verified from the snapshot, the manuals and the controller copy:

- Two different KR C4 signal maps exist. The 17 Oct `STEU/Mada/$machine.dat` has `$EXT_START $IN[3]`, `$CONF_MESS $IN[4]`, `$DRIVES_OFF $IN[5]`, `$DRIVES_ON $IN[6]`, `$I_O_ACT $IN[1025]`, `$RC_RDY1`/`$PERI_RDY`/`$STOPMESS`/`$PRO_ACT` on `$OUT[1..4]`; HARDWARE_SETUP.md (29 Oct) has `$I_O_ACT` = 1026 (interface inactive), `$EXT_START`, `$CONF_MESS`, `$DRIVES_ON` = 1026, `$DRIVES_OFF` = 1025, PGNO_FBIT = 4001 and outputs on internal numbers. Either way the PLC cannot use AUT EXT now, and the live configuration must be read before it is changed: under the old map `$DRIVES_OFF $IN[5]` would lie on a PGNO wire of §3.1.
- `$MOVE_ENABLE` = `$IN[1025]` blocks external start unless `$CHCK_MOVENA` = FALSE (KSS §6.17.2.1); the controller copy `STEU/Mada/$option.dat` already has `$CHCK_MOVENA=FALSE`, so no change is needed (verify on the controller).
- RobotHandler symbols still carry old addresses ("X1.0 (was)"); `AUT_EXT`, `PRO_ACT`, `APPL_RUN` used in transitions are not declared; no PGNO bits are output at all, and ST_SELECT_PROG only waits 3 s (`ST_SELECT_PROG.done`) instead of a KUKA handshake.
- ACT_ResetSignals writes `Step.timePreset`; the ISPSoft step-property table (§14.2.8.1) lists `timePreset` as readable, not writable — the preset belongs in the step dialog.
- `EXT_START`, `CONF_MESS`, `DRIVES_ON` are assigned in several N actions; after a step ends, the final scan resets only OUT coils (§14.2.1), so the ST assignments can leave a signal TRUE.
- PCHandler clears `WANTTOSEND` with `IF TCP_SENT OR (NOT TCP_ERROR)`, i.e. on every error-free scan, not on send completion.
- SOPEN(1,1,2) opens TCP socket 1 as server in mode 2: `Rx[0]` holds the received length and the frame starts at `Rx[1]` (AS §6.22, SOPEN note 8). CmdMonitor handles one frame per `Rx[0]` cycle and never checks magic/version/type/length; the defined E_CODE values are unused.
- `E_VARTYPE` differs between sides: PLC PGNO = 16#C3, SFY_OK = 16#C5; PC `VAR_TYPE::SFY_OK` = 0xC3, so a PC "SFY_OK" arrives as PGNO.
- FB_VarMonitor is commented out: the PLC sends no robot/cell state; the PC parses `CELL_STATE` but the PLC never sends it.
- Main Table maps LED_GREEN to Y1.7, which ProbeMap also drives as the probe copy; ARCHITECTURE.md §1 (2026-11-06) has LED_GREEN on Y2.4.
- AS socket: if the client (PC) closes, the PLC server stays "connected" (SCLOSE note 5) until the Keep Alive Timer expires with no data (ISPSoft §3.1.4.12). The PC sends no periodic traffic.

Handshake facts from the KSS signal diagrams (PGNO_VALID = 0, Fig. 6-24) and Robot Programming 3 ch. 8:

- Start: `$DRIVES_ON` pulse (≥ 20 ms) → `$PERI_RDY`; `$CONF_MESS` rising edge → `$STOPMESS` falls; `$EXT_START` rising edge → CELL.SRC runs, `$PRO_ACT` rises.
- Program number: CELL sets PGNO_REQ TRUE (a level) → PLC presents PGNO → second `$EXT_START` edge → APPL_RUN rises and PGNO_REQ falls (acknowledgement). At program end APPL_RUN falls, CELL runs again and PGNO_REQ rises again; `$PRO_ACT` stays TRUE.
- Interrupted program = an application program (called from a CELL.SRC CASE branch) that KSS stopped before its END: operator safety (Fig. 6-25), path-maintaining E-STOP (6-26), `$MOVE_ENABLE` = FALSE (6-27), programmed user STOP or STOP key (6-28). The program stays selected at the stopped block; APPL_RUN, `$PRO_ACT` and PGNO_REQ are FALSE. Restart: drives on, acknowledge, `$EXT_START` → the program continues; no program number is exchanged.
- The figures describe KR C4 behaviour, not checks the PLC must repeat. The KR C4 itself ignores `$EXT_START` or stops CELL with a message when its own preconditions (`$PERI_RDY`, `$IN_HOME`, `$ON_PATH`, no `$STOPMESS`, AUT EXT active) are not met; the PLC sees only "no reaction", which a step timeout covers. A `$DRIVES_ON` pulse with drives already on and a `$CONF_MESS` edge without a message change nothing, so both are pulsed unconditionally.

## 2. Solution in one paragraph

RobotHandler is a pure start sequence of 4 steps: the PC sends "run program k" (k = 1 … 4) or "resume"; the PLC pulses `$DRIVES_ON` and `$CONF_MESS`, starts CELL.SRC with `$EXT_START` if it is not running, presents PGNO = k − 1 on two outputs while PGNO_REQ is TRUE and gives the second `$EXT_START` edge (PGNO_VALID = 0); it returns to idle once APPL_RUN shows that the KR C4 accepted the start, or on a timeout. The PLC does not supervise the running program: the eight KUKA status outputs reach the PC in the existing IO change message, and the PC derives running, finished and interrupted from them. PCHandler refuses a new job that could continue an interrupted program. The minimal variant gives the PLC no means to stop a running program; the operator stops it on the smartPAD.

## 3.1 ISPSoft program overview

### KR C4 ↔ PLC signals (5 of 7 free PLC outputs, 7 of 8 PLC inputs)

| PLC | KR C4 | AUT EXT signal | Use |
| --- | --- | --- | --- |
| Y1.0 → EL1809 in 1 | `$IN[1]` | `$EXT_START` | start CELL; 2nd edge transfers PGNO; continue an interrupted program |
| Y1.1 → in 2 | `$IN[2]` | `$DRIVES_ON` | 0.2 s pulse at sequence start |
| Y1.2 → in 3 | `$IN[3]` | `$CONF_MESS` | 0.2 s pulse at sequence start |
| Y1.3, Y1.4 → in 4, 5 | `$IN[4..5]` | PGNO bits 0, 1 | PGNO = k − 1 = 0 … 3, binary |
| Y1.5, Y1.6 → in 6, 7 | `$IN[6..7]` | spare | option: Y1.5 → `$MOVE_ENABLE` (PLC stop, Fig. 6-27) |
| Y1.7 → in 8 | `$IN[8]` | — | probe copy (unchanged) |
| X1.0 ← EL2809 out 1 | `$OUT[1]` | `$I_O_ACTCONF` | status for the PC (Run button) |
| X1.1 ← out 2 | `$OUT[2]` | spare | — |
| X1.2 ← out 3 | `$OUT[3]` | `$PERI_RDY` | sequence: drives on before `$EXT_START` |
| X1.3 ← out 4 | `$OUT[4]` | `$STOPMESS` | status for the PC |
| X1.4 ← out 5 | `$OUT[5]` | `$PRO_ACT` | sequence: CELL already running; PC: interrupted |
| X1.5 ← out 6 | `$OUT[6]` | PGNO_REQ | sequence: CELL waits for the number |
| X1.6 ← out 7 | `$OUT[7]` | APPL_RUN | sequence: start accepted; PC: running/finished |
| X1.7 ← out 8 | `$OUT[8]` | `$IN_HOME` | PCHandler guard for a new job |

Two PGNO bits encode four programs (0 … 3). Before commissioning, confirm that P00 `#PGNO_GET` accepts PGNO = 0 (read P00.SRC in `C:\KRC\ROBOTER\KRC\R1\TP`, or run CELL with CASE 0 in T1); only if it does not, use Y1.5 as a third bit and PGNO = k. A broken PGNO wire yields another valid number with any bit count; detecting it needs PGNO_PARITY and one more output. `$DRIVES_ON` and `$CONF_MESS` stay outputs: `$CONF_MESS` acts only on a rising edge, and KSS documents `$DRIVES_ON` only as a ≥ 20 ms pulse. The EL2809 → X1 wiring does not exist yet; check the AS16AP11P-A S/S polarity against the EL2809 sourcing outputs.

### KR C4 configuration (smartPAD, Configuration > Inputs/outputs > Automatic External)

- First read the live values (two maps exist, §1); assign the targets below and check that no other signal remains on `$IN[1..7]`/`$OUT[1..8]`.
- Inputs: `$I_O_ACT` → 1025; `$EXT_START` → 1; `$DRIVES_ON` → 2; `$CONF_MESS` → 3; PGNO_TYPE 1, PGNO_FBIT 4, PGNO_LENGTH 2, PGNO_PARITY 0, PGNO_VALID 0; `$MOVE_ENABLE` and `$DRIVES_OFF` → 1025 (always TRUE).
- Outputs: `$I_O_ACTCONF` → 1, `$PERI_RDY` → 3, `$STOPMESS` → 4, `$PRO_ACT` → 5, PGNO_REQ → 6, APPL_RUN → 7, `$IN_HOME` → 8. Update HARDWARE_SETUP.md afterwards.
- CELL.SRC (KSS §6.17.1): CASE 0 … 3 for programs 1 … 4, each `P00(#EXT_PGNO,#PGNO_ACKN,DMY[],0)` then the program call; DEFAULT → `#PGNO_FAULT`. Until the programs exist, the branches call nothing or one test program.
- AUT EXT has no BCO run and moves at programmed velocity (KSS §6.17.2.1, §8.11); selecting CELL.SRC requires the BCO run in T1/T2 described in §8.11.

### RobotHandler (SFC, 4 steps)

| Step | Outputs (computed in RobotIo from step flag X and step time T) | Leaves by |
| --- | --- | --- |
| ST_IDLE (initial) | all off | TR_Run: CmdRun ∨ CmdResume → ST_PREP |
| ST_PREP | DRIVES_ON for T < 0.2 s; CONF_MESS for 0.3 s ≤ T < 0.5 s | TR_Prepared: PERI_RDY ∧ ¬STOPMESS ∧ T ≥ 0.5 s → ST_START (an `$EXT_START` edge given while a stop message is pending is lost) |
| ST_START | EXT_START while ¬PRO_ACT (no edge when CELL already runs) | TR_Running: PRO_ACT ∧ APPL_RUN → ST_IDLE (RESUMED); TR_Request: PRO_ACT ∧ PGNO_REQ ∧ JobPgno ≠ 0 → ST_SELECT |
| ST_SELECT | PGNO = JobPgno − 1; EXT_START for T ≥ 0.1 s | TR_Acked: APPL_RUN ∧ ¬PGNO_REQ → ST_IDLE (STARTED) |

- TR_Fail (one transition program after ST_PREP, ST_START, ST_SELECT → ST_IDLE) reads `Fail` = CmdAbort ∨ StepTimeout ∨ ¬`$I_O_ACTCONF`. Proposed limits (hypothetical, to tune): ST_PREP 5 s, ST_START 30 s (CELL first moves PTP HOME), ST_SELECT 2 s. A timeout covers every KR C4 refusal (AUT EXT off, stop message not cleared, robot not at HOME, invalid PGNO); the PC shows `$STOPMESS`, `$I_O_ACTCONF` and `$IN_HOME` beside the result.
- When CELL already waits for a number, ST_START raises no `$EXT_START` and TR_Request fires at once. A resume ends through TR_Running; a new job could reach TR_Running only by continuing an interrupted program, which the PCHandler guard prevents.
- At each divergence the leftmost true transition is taken first (ISPSoft §14.2.3); the order is the one listed. The SFC has no actions; transitions contain only contacts and comparisons (§14.2.2). RobotIo also holds JobPgno and the result code and clears the one-shot commands when ST_PREP is entered.

## 3.2 plcdevice changes

- New invokables (unique names): `runRobotProgram({program: 1…4})` → SET_VAR RUN_PROG; `resumeRobot()` → SET_VAR ROBOT_RESUME; `abortRobotStart()` → SET_VAR ROBOT_ABORT (stops the start sequence only). SET_VAR framing (var, attr) is reused; only codes change.
- PlcMessageManager: one `VAR_TYPE` table identical to the PLC `E_VARTYPE` (RUN_PROG, ROBOT_RESUME, ROBOT_ABORT; START_CELL/SFY_OK/PGNO removed); `CELL_STATE` payload becomes chg + step + program + result (4 bytes) published as `robotStep`, `robotProgram`, `robotResult`; the `CELL` enum is replaced by the step/result codes.
- Robot program state on the PC, derived in PlcDevice from the `x1` bits and published as one key: running = APPL_RUN ∧ PRO_ACT; interrupted = APPL_RUN ∧ ¬PRO_ACT (P00 keeps APPL_RUN), or ¬PRO_ACT after running; finished = ¬APPL_RUN ∧ PRO_ACT ∧ PGNO_REQ after running or interrupted; otherwise unchanged. After a PC restart "interrupted" is unknown until the next start; the PLC guard (§3.3) still applies.
- Transport: one request in flight (queue, next frame after the reply), because CmdMonitor accepts one frame per `Rx[0]` cycle; a reply timeout clears the request and logs it (closes the "PLC pending lifetime" open question); a periodic SNAPSHOT (e.g. every 2 s) keeps traffic below the PLC Keep Alive Timer so the PLC drops a dead PC connection.
- UnconnectedState also clears the robot keys. New RESP_ERR codes (BUSY, BAD_VALUE, NOT_ALLOWED) are logged.
- QML (DeltaPanel): program selector 1 … 4; Run, Resume and Abort enabled under the same conditions the PLC checks (§3.3); one status line in reserved space showing sequence step/result and program state. Run moves the robot in AUT EXT and is only ever operator-initiated.

## 3.3 PCHandler changes

- Send: clear `WANTTOSEND` on `TCP_SENT OR TCP_ERROR`; on `TCP_ERROR` close with SCLOSE and reopen with SOPEN once closed.
- CmdMonitor: check `Rx[0]` ≥ 6 + header length, magic, version and type before dispatch; answer FB_WriteTxRespErr with the existing E_CODE values.
- FB_RunSetVar, all only in ST_IDLE except ABORT: RUN_PROG with attr 1 … 4 requires `$I_O_ACTCONF` ∧ IN_HOME ∧ ¬APPL_RUN; ROBOT_RESUME requires `$I_O_ACTCONF` ∧ APPL_RUN ∧ ¬PRO_ACT; ROBOT_ABORT only in ST_PREP, ST_START, ST_SELECT. Otherwise RESP_ERR BUSY/BAD_VALUE/NOT_ALLOWED. Accepted commands set JobPgno, CmdRun/CmdResume/CmdAbort.
- FB_VarMonitor: build a `CELL_STATE` change frame when step, program or result changes and on each new connection (rising TCP_CONNECTED); IoMonitor also becomes pending on a new connection.
- WRITE_IO: reject masks that change Y1.0 … Y1.7 (robot and probe outputs); the PC panel disables those switches.

## 3.4 Other PLC changes

- Main Table: KUKA signals with addresses from §3.1; global JobPgno (WORD), RobotResult (WORD), CmdRun, CmdResume, CmdAbort, Fail (BOOL) shared by PCHandler, RobotHandler and RobotIo; remove CmdStartCell, CmdSfyOk.
- DUT: E_VARTYPE (RUN_PROG, ROBOT_RESUME, ROBOT_ABORT); E_CODE + BUSY, BAD_VALUE, NOT_ALLOWED; new E_RSTEP (IDLE, PREP, START, SELECT) and E_RRESULT (NONE, STARTED, RESUMED, TIMEOUT, ABORTED, AUTEXT_LOST).
- New PRG RobotIo (ST); task order PCHandler → RobotIo → RobotHandler, so RobotIo records the result and `Fail` from the step that a transition leaves in the same scan; outputs lag one scan. Implementation: `plans/PLC_AUT_EXT_PROGRAM_START_EXECPLAN.md`.
- RobotHandler: delete the 11 actions and the old transitions; step timePreset is not used.
- HWCONFIG socket 1: set Keep Alive Timer above the PC heartbeat (e.g. 10 s).
- onLostConnection: a lost connection ends nothing on the robot; a sequence in progress finishes or times out.
