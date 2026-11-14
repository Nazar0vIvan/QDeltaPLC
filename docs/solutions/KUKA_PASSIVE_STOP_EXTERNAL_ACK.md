# External acknowledgement of "KSS01370 Passive STOP" in Automatic External

This document describes how to let the PLC acknowledge the KUKA message "KSS01370 Passive STOP" through the Automatic External input `$CONF_MESS`, so that a program stopped with the smartPAD STOP key can be continued from RoboCrap with one Resume. It also records why this is needed, what is verified and what is not, the safety consequence, and the alternative without any controller change.

Status (2026-11-14): **proposed, not applied, not verified on the controller.** The mechanism (`KrcExtConfMsg.xml`) comes from forum posts and third-party articles, not from the KUKA manuals available in this project. Applying it is the user's decision (see "Safety consequence").

## System context

- Robot: KUKA KR10 R1420 with a KR C4 smallsize-2 controller, KUKA System Software (KSS) 8.3.
- PLC: Delta AS332T with expansion module 1, AS16AP11P-A (inputs X1.0 … X1.7, outputs Y1.0 … Y1.7), wired to the KR C4 EtherCAT terminals EL1809 (`$IN[1]` … `$IN[16]`) and EL2809 (`$OUT[1]` … `$OUT[16]`).
- Automatic External (AUT EXT): a KR C4 operating mode in which a higher-level controller, here the PLC, starts programs through digital signals. In AUT EXT the controller runs `CELL.SRC`, which reads a program number from the PLC and calls the matching program (repository copy `resources/files/AutExt/CELL.src`).
- RoboCrap: the PC application. Its PLC panel (`src/application/qml/Views/Dashboard/DeltaPanel/RobotStartPanel.qml`) has the buttons Run, Resume and Abort and shows the PLC inputs X1.x.
- The live AUT EXT signal assignment is recorded in `docs/plc_project_snapshot/AUT_EXT_DIOs_live.md`. The work around it is planned in `plans/PLC_AUT_EXT_PROGRAM_START_EXECPLAN.md`.

Signals used here (direction from the KR C4's point of view):

| KUKA signal | KR C4 I/O | PLC | Meaning |
| --- | --- | --- | --- |
| `$I_O_ACTCONF` | `$OUT[1]` | X1.0 | AUT EXT selected and interface active |
| `$PERI_RDY` | `$OUT[3]` | X1.2 | drives on |
| `$STOPMESS` | `$OUT[4]` | X1.3 | a message that stops the robot is pending ("collective fault") |
| `$PRO_ACT` | `$OUT[5]` | X1.4 | a program (CELL or the called program) is executing |
| PGNO_REQ | `$OUT[6]` | X1.5 | CELL waits for a program number |
| APPL_RUN | `$OUT[7]` | X1.6 | set by the KUKA package P00 when a program number is acknowledged, cleared when CELL asks for the next number |
| `$EXT_START` | `$IN[1]` | Y1.0 | a rising edge starts or continues a program |
| `$DRIVES_ON` | `$IN[2]` | Y1.1 | a high pulse of at least 20 ms switches the drives on |
| `$CONF_MESS` | `$IN[3]` | Y1.2 | a rising edge acknowledges messages (KSS 8.3 SI §6.17.2.1: "Only the rising edge of the signal is evaluated") |

## What Resume does

Resume continues a stopped program from the point where it stopped; it does not start the program again.

1. The smartPAD STOP key triggers a STOP 2 (KSS 8.3 SI §3.4): the robot brakes along its programmed path, the drives stay on, and the program pointer stays on the interrupted motion instruction.
2. RoboCrap's Resume sends the PLC the command ROBOT_RESUME. The PLC accepts it only while APPL_RUN is TRUE and `$PRO_ACT` is FALSE (an interrupted program).
3. The PLC start sequence (PLC program `RobotIo`, SFC `RobotHandler`) runs the step ST_PREP: `$DRIVES_ON` high from 0 to 200 ms, `$CONF_MESS` high from 300 to 500 ms, then it waits until `$PERI_RDY` is TRUE and `$STOPMESS` is FALSE, for at most 5 s, otherwise the result is TIMEOUT.
4. In ST_START the PLC raises `$EXT_START` from 0.1 s on. The controller then finishes the interrupted motion from the stop point to that motion's target (at the programmed velocity times the program override, 10 % in this cell), and the program continues with the next instructions. If the robot was moved off the path in the meantime (`$ON_PATH` FALSE), the controller first makes a repositioning move back to the path (KSS 8.3 SI Fig. 6-25).

Example without motion (program 4 is `WAIT SEC 3`): the STOP key came 0.83 s after the start; after Resume the program finished 2.15 s after `$PRO_ACT` rose again, i.e. the remaining part of the same wait.

## Problem

After the smartPAD STOP key in AUT EXT the controller shows the acknowledgeable message "KSS01370 Passive STOP" and sets `$STOPMESS` TRUE. The PLC's `$CONF_MESS` pulse in ST_PREP does not clear this message, so `$STOPMESS` stays TRUE and every Resume ends with TIMEOUT. After the operator acknowledges the message on the smartPAD, `$STOPMESS` becomes FALSE and Resume works.

Evidence: RoboCrap's PLC log `records/plc_io_20261114_152647.csv` (beside the executable), one action at a time:

| Action | Time | Logged result |
| --- | --- | --- |
| Run program 4 | 15:28:16.288 | STARTED 16.947 |
| STOP key | 15:28:17.774 | STOPMESS 1, PRO_ACT 0, APPL_RUN still 1 |
| Resume, message not acknowledged | 15:29:00.513 | ST_PREP 00.738, CONF_MESS 1 from 00.839 to 01.037, STOPMESS stays 1, TIMEOUT 05.537 |
| Message acknowledged on the smartPAD | 15:30:26.090 | STOPMESS 0 |
| Resume | 15:30:28.351 | CONF_MESS 28.676 to 28.878, EXT_START 28.982, PRO_ACT 29.019, result RESUMED, program finished 31.170 |

The same behaviour was seen in five earlier Resume attempts (log `plc_io_20261114_134646.csv`).

## Why the PLC's acknowledgement is ignored

- The wiring and configuration are correct: `$CONF_MESS` is assigned to `$IN[3]`, and a forced Y1.2 was seen on `$IN[3]` during commissioning.
- `$CONF_MESS` does acknowledge other messages on this controller: after an EMERGENCY STOP was released without touching the smartPAD, a Run from RoboCrap cleared `$STOPMESS` with the PLC's pulse and started the program (log `plc_io_20261114_170121.csv`: CONF_MESS 17:05:26.427, `$PERI_RDY` TRUE 27.579, `$STOPMESS` FALSE 27.735, STARTED 27.878). Only KSS01370 is refused.
- The pulse timing is not the cause. The KSS 8.3 manual evaluates only the rising edge; a KUKA KR C1 Automatic External guide asks for a high pulse of at least 20 ms. The PLC's pulse lasts 200 ms and comes 300 ms after ST_PREP starts, while `$PERI_RDY` is already TRUE and the STOP key is no longer pressed.
- The KSS 8.3 signal diagrams do not cover this case. Fig. 6-28 ("Restart after user STOP", a stop programmed in the robot program) shows no `$STOPMESS` and no `$CONF_MESS`; the restart is only an `$EXT_START` edge. Figs. 6-25 to 6-27 (operator safety, EMERGENCY STOP, motion enable) show the order: the cause of the stop disappears, `$DRIVES_ON`, rising edge of `$CONF_MESS`, `$STOPMESS` FALSE, `$CONF_MESS` FALSE, rising edge of `$EXT_START`. The smartPAD STOP key in AUT EXT is not drawn.
- KUKA decides per message whether `$CONF_MESS` may acknowledge it. An old KUKA message list (KSS 5.x, message 209 "Passive STOP") gives the remedy as acknowledgement only on the originating unit, i.e. the operator device that issued the stop. A KRC2 forum thread (software 5.6.8, 2011) reports the same symptom as here: after the STOP key, the PLC's `$CONF_MESS` did not clear "passiver Stop". A KR C4 forum thread reports that only some messages can be acknowledged by `$CONF_MESS` (for example not a motor torque error).

## Solution: whitelist KSS01370 in `KrcExtConfMsg.xml`

KSS 8 reads the file `KrcExtConfMsg.xml`, in which individual acknowledgement messages can be allowed or forbidden for acknowledgement by `$CONF_MESS` in AUT EXT. The template header names "KUKA V8.3"; the template comment says "only confirmation messages are allowed to configure". KSS01370 is a confirmation (acknowledgement) message: it waits for OK on the smartPAD. One forum user made message 02858 externally acknowledgeable this way on KSS 8.7. No source reports doing it for 01370.

File: `C:\KRC\ROBOTER\Config\User\Common\KrcExtConfMsg.xml` on the KR C4. The folder exists on this controller (it holds `SensorInterface\RSI_EthernetConfig.xml`). The schema it refers to is `C:\KRC\ROBOTER\Config\System\Common\Schemes\KrcExtConfMsg.xsd`.

Complete file, if it does not exist yet (structure from the forum template):

```xml
<?xml version="1.0" encoding="UTF-8"?>
<KrcExtConfMsg xsi:noNamespaceSchemaLocation="/Roboter/Config/System/Common/Schemes/KrcExtConfMsg.xsd" xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance">
  <Version Label="">
    <Info Version="KUKA V8.3"/>
    <Schema Version="1"/>
  </Version>
  <!-- RoboCrap: allow the PLC ($CONF_MESS) to acknowledge KSS01370 Passive STOP in AUT EXT -->
  <Message Number="01370" AllowConfInExt="true" />
</KrcExtConfMsg>
```

If the file exists, add only the `<Message … />` line before `</KrcExtConfMsg>` and keep everything else. `AllowConfInExt="false"` forbids external acknowledgement of a message that is allowed by default.

Number format: one forum example writes the number with leading zeros as shown in the message ("02858" for KSS02858), another without ("1131"). Which form KSS 8.3 expects is not known; use "01370" first, and "1370" if the test below fails.

### Procedure (user, on the controller)

1. Backup: archive the controller (Main menu > File > Archive) before the change.
2. Check support: switch to T1, log on as Expert, Main menu > Start-up > Service > Minimize HMI (KSS 8.3 SI §4.2.4). In Windows Explorer check that `C:\KRC\ROBOTER\Config\System\Common\Schemes\KrcExtConfMsg.xsd` exists. If it does not exist, this KSS does not support the file; stop here and use the alternative below.
3. Check `C:\KRC\ROBOTER\Config\User\Common\KrcExtConfMsg.xml`. If it exists, copy it to the PC first (it may already contain entries) and add the line; otherwise create it with the complete text above.
4. Copy the file to the controller. The established route in this project: put the file into the PC share `C:\RoboCrapJobs` and copy it in the KR C4 Explorer from `\\192.168.2.100\RoboCrapJobs` (see `docs/KR_C4_WINDOWS10_SHARED_FOLDER_SETUP.md`). Maximize the smartHMI again.
5. Initial cold start: Main menu > Shutdown, select Cold start (or Force cold start) and Reload files, then Reboot control PC. KSS 8.3 SI §4.6: a directly edited XML file is read only by a cold start with Reload files ("initial cold start"); a cold start without Reload files ignores the change.
6. After the restart, check for a configuration error message on the smartPAD. An invalid file or an entry for a non-confirmation message may be reported or ignored; the exact message is not known.
7. WorkVisual: the KRL files of this cell are transferred with the user's WorkVisual project. Whether a later WorkVisual deployment overwrites or deletes `Config\User\Common\KrcExtConfMsg.xml` is not known; after the next deployment check that the file is still there, or add it to the WorkVisual project.

### Verification

Repeat the clean A3 run of the ExecPlan (robot at HOME, AUT EXT, one action at a time with 10 s pauses, new log by Disconnect/Connect in RoboCrap): run program 4, press the smartPAD STOP key while "program running" is shown, then press Resume in RoboCrap **without** acknowledging on the smartPAD.

- Works: in ST_PREP the `$CONF_MESS` pulse is followed by `$STOPMESS` = 0, then ST_START, `$EXT_START`, `$PRO_ACT` = 1, result RESUMED, and the program finishes; KSS01370 disappears from the smartPAD message window.
- Does not work: `$STOPMESS` stays 1 and the result is TIMEOUT, as before. Try `Number="1370"`, check step 2 and the file location, and record the outcome in the ExecPlan.

### Rollback

Delete the `<Message Number="01370" … />` line (or the file, if it was created for this purpose) and make an initial cold start again (step 5).

## Safety consequence

Without the entry, continuing after a STOP key needs two actions in two places: someone acknowledges KSS01370 on the smartPAD, then someone presses Resume in RoboCrap. With the entry, one action in RoboCrap is enough: Resume acknowledges the stop message and continues the interrupted motion. The robot then moves on along the rest of its path although nobody at the smartPAD confirmed it, and the person who pressed STOP has no say in when it continues.

Limits that stay in place: the entry changes only this one message; EMERGENCY STOP, operator safety and other safety stops are not affected (their messages keep their own acknowledgement rules). The STOP key is an operational stop (STOP 2), not a safety function. The PLC accepts Resume only in AUT EXT with APPL_RUN TRUE and `$PRO_ACT` FALSE, and the program override stays at 10 %.

Forum users explicitly warn against allowing remote acknowledgement of collision or safety faults. KUKA's default requires local acknowledgement of the passive STOP. Whether a remote continuation after a local STOP is acceptable for this cell is the user's decision, to be recorded in the ExecPlan's Decision Log.

## Alternative without a controller change

The operator acknowledges KSS01370 on the smartPAD, as today, and RoboCrap shows whether that has happened. The acknowledgement status is already available to RoboCrap: `$STOPMESS` is wired to PLC input X1.3 and travels to the PC in every IO change frame and every 2 s heartbeat reply (key `x1`, bit 3). Acknowledging KSS01370 sets `$STOPMESS` FALSE (15:30:26.090 in the log above), provided no other stop message (for example a pressed EMERGENCY STOP) is pending.

Implemented on 2026-11-14 (current behaviour without the whitelist entry): RoboCrap enables Resume only while X1.3 is FALSE. While the program is interrupted (APPL_RUN TRUE, `$PRO_ACT` FALSE, PLC step IDLE) the status line shows "Program interrupted · acknowledge message on smartPAD" with X1.3 TRUE and "Program interrupted · message acknowledged · Resume ready" with X1.3 FALSE. The PLC's RESUME check (`FB_RunSetVar`) refuses RESUME with NOT_ALLOWED while `STOPMESS` is TRUE. This avoids the 5 s TIMEOUT but cannot tell which message holds `$STOPMESS`, so the texts say "message". If the whitelist entry is applied later, both Resume conditions on X1.3 must be removed again, otherwise Resume stays disabled until the smartPAD acknowledgement.

Identifying the message itself would need KUKA's error-number transmission (KSS 8.3 SI §6.17.3, `PLC_ENABLE=TRUE` in `P00.DAT`, which writes the number of a configured `$STOPMESS` message to 8 outputs). The PLC's module 1 has only one free input (X1.1, wired from `$OUT[2]`), so this is not possible with the current wiring.

## Open points

- Support of `KrcExtConfMsg.xml` on this KSS 8.3 controller (step 2).
- Number format "01370" or "1370".
- Interaction with WorkVisual deployment.
- Behaviour of a STOP during the RSI part of the calibration program `RSI_Probe_Calibration` and its continuation; not described in the KST RSI 3.3 manual.

## Sources

- KUKA System Software 8.3, Operating and Programming Instructions for System Integrators (`manuals/KUKA`): §3.4 stop reactions, §4.2.4 Minimize HMI, §4.6 shutdown and initial cold start, §6.17.2 Automatic External inputs/outputs, §6.17.3 error-number transmission, §6.17.4 signal diagrams (Figs. 6-23 to 6-28, also as `manuals/KUKA/6-23.png` … `6-28.png`).
- [Shutdown gracefully while in EXT, robot-forum.com](https://www.robot-forum.com/robotforum/thread/42643-shutdown-gracefully-while-in-ext/): `KrcExtConfMsg.xml` location, template and use for message 02858.
- [Benutzermeldung extern quittieren, roboterforum.de](https://www.roboterforum.de/roboter-forum/thread/14283-benutzermeldung-extern-quittieren/): second example entry.
- ["Passiver Stop" in AutomatiExtern, roboterforum.de](https://www.roboterforum.de/roboter-forum/thread/9893-passiver-stop-in-automatiextern/): same symptom on KRC2 5.6.8.
- [Error Acknowledge In Auto Ext, robot-forum.com](https://www.robot-forum.com/robotforum/thread/11046-error-acknowledge-in-auto-ext-more-complicated-than-usual/): only some messages acknowledgeable by `$CONF_MESS` on KR C4.
- [Fehler quittieren in Automatik Extern, roboterforum.de](https://www.roboterforum.de/roboter-forum/thread/1649-fehler-quittieren-in-automatik-extern/): older `INIT\ext_conf.ini` mechanism.
- [KUKA KSS System Message list (KSS 5.x)](http://www.wtech.com.tw/public/download/manual/kuka/krc2ed05/KUKA%20KSS%20System%20Message.pdf): message 209 "Passive STOP"; seen only as a search summary, the document itself could not be opened.
- [Kuka KR C1 Automatic External](http://www.loullingen.lu/projekte/kuka/kuka_automatik_extern.php?language=EN): `$CONF_MESS` high pulse of at least 20 ms.
- [KUKA KRC4 External Mode Error Acknowledgment Configuration](https://industrialmonitordirect.com/blogs/knowledgebase/kuka-krc4-external-mode-error-acknowledgment-configuration) (third-party article, unverified): file path and cold start with Reload files.
