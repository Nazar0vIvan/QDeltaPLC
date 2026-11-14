Text snapshot of the ISPSoft project `RoboCrapPLC` at the end of `plans/PLC_AUT_EXT_PROGRAM_START_EXECPLAN.md` (2026-11-14; entered, compiled without errors and downloaded by the user, stage A and stage B passed). The changed parts are transcribed from the plan as the user entered them, not exported from ISPSoft. Ladder and SFC parts are in the `.drawio` files of this folder.

Changes after the Milestone 3 snapshot (2026-11-13), all reflected below:
- `PCHandler` send logic and `FB_CmdMonitor` intake no longer wait for `TCP_ERROR` (SM1277 is read-only and latched; a socket instruction refreshes it).
- `RobotIo`: PGNO bits already in ST_START, `$EXT_START` in ST_START from 0.1 s, a `TR_Running` exit of a job reported as STARTED.
- `FB_VarMonitor` and `E_CHGTYPE.CELL_STATE` removed; `FB_IOMonitor` sends step/program/result in the IO change frame (payload 8) and `FB_RunSnapShot` in the SNAPSHOT reply (payload 9); `PCHandler` and `FB_TxArbiter` without the VarMonitor connection. `E_SENDSRC.SEND_VAR` is no longer used but was not deleted.
- `FB_RunSetVar`: RESUME also requires `STOPMESS` OFF (the smartPAD STOP key's "Passive STOP" is not acknowledged by `$CONF_MESS`) and `ResumeBlocked` OFF.
- Main Table row 39 `ResumeBlocked`, computed at the start of `RobotIo` (APPL_RUN held while AUT EXT dropped).

# DUT:

- E_TYPE (ENUM)
- E_CMD (ENUM)
- E_DEV (ENUM)
- E_ERR (ENUM)
- E_CODE (ENUM)
- T_HEADER (STRUCT)
- E_CHGTYPE (ENUM)
- E_SENDSRC (ENUM)
- E_VARTYPE (ENUM)
- E_RSTEP (ENUM)
- E_RRESULT (ENUM)

## E_TYPE (ENUM)

```
TYPE E_TYPE :
(
  REQ      := 16#00A0,
  RESP_OK  := 16#00A3,
  RESP_ERR := 16#00AC,
  CHG      := 16#00AF
);
END_TYPE
```

## E_CMD (ENUM)

```
TYPE E_CMD :
(
  READ_IO   := 16#00B0,
  READ_REG  := 16#00B3,
  WRITE_IO  := 16#00B5,
  WRITE_REG := 16#00B6,
  WRITE_RAW := 16#00B9,
  SNAPSHOT  := 16#00BA,
  SET_VAR   := 16#00BC,
  NOCMD     := 16#00BF    
);
END_TYPE
```

## E_DEV (ENUM)

```
TYPE E_DEV :
(
  DEV_X := 16#0058,
  DEV_Y := 16#0059,
  DEV_D := 16#0044
);
END_TYPE
```

## E_ERR (ENUM)

```
TYPE E_ERR :
(
  BAD_REQUEST := 16#0001,
  INTERNAL    := 16#0003
);
END_TYPE
```

## E_CODE (ENUM)

```
TYPE E_CODE :
(
  BAD_MAGIC     := 16#EE01,
  BAD_VER       := 16#EE02,
  BAD_TYPE      := 16#EE04,
  BAD_CMD       := 16#EE05,
  BAD_DEV       := 16#EE06,
  BAD_MOD       := 16#EE07,
  LEN_MISMATCH  := 16#EE08,
  BUSY          := 16#EE09,
  BAD_VALUE     := 16#EE0A,
  NOT_ALLOWED   := 16#EE0B
);
END_TYPE
```

## T_HEADER (STRUCT)

```
TYPE T_HEADER :
  STRUCT
    magic : WORD;
    ver   : WORD;
    type  : WORD;
    tid   : WORD;
    len   : WORD;
  END_STRUCT
END_TYPE
```

## E_CHGTYPE (ENUM)

```
TYPE E_CHGTYPE :
(
  IO         := 16#00D0
);
END_TYPE
```

## E_SENDSRC (ENUM)

```
TYPE E_SENDSRC :
(
  SEND_NONE,
  SEND_CMD,
  SEND_IO,
  SEND_VAR
);
END_TYPE
```

## E_VARTYPE (ENUM)

Codes of the SET_VAR command; equal to `PlcMessageManager::VAR_TYPE` on the PC.

```
TYPE E_VARTYPE :
(
  RUN_PROG     := 16#00C0,
  ROBOT_RESUME := 16#00C3,
  ROBOT_ABORT  := 16#00C5
);
END_TYPE
```

## E_RSTEP (ENUM)

Start-sequence step of RobotHandler, written to `RobotStep` by RobotIo and sent to the PC in the IO change frame (FB_IOMonitor).

```
TYPE E_RSTEP :
(
  RS_IDLE   := 0,
  RS_PREP   := 1,
  RS_START  := 2,
  RS_SELECT := 3
);
END_TYPE
```

## E_RRESULT (ENUM)

Result of the last start sequence, written to `RobotResult` by RobotIo.

```
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
```

# Global Symbols

## Main Table

| No. | Class | Identifiers   | Address    | Type               | Initial Value | Identifier Comment |
| --- | ----- | ------------- | ---------- | ------------------ | ------------- | ------------------ |
| 1   | VAR   | LED_RUN       | Y2.3       | BOOL               | FALSE         |                    |
| 2   | VAR   | LED_GREEN     | Y1.7       | BOOL               | FALSE         |                    |
| 3   | VAR   | LED_RED3      | Y2.5       | BOOL               | FALSE         |                    |
| 4   | VAR   | LED_RED2      | Y2.6       | BOOL               | FALSE         |                    |
| 5   | VAR   | LED_RED1      | Y2.7       | BOOL               | FALSE         |                    |
| 6   | VAR   | Tx            | D100       | ARRAY [16] OF WORD | N/A           | Sending bytes      |
| 7   | VAR   | Rx            | D200       | ARRAY [16] OF WORD | N/A           | Receiving bytes    |
| 8   | VAR   | TCP_CONNECTED | SM1270     | BOOL               | N/A           |                    |
| 9   | VAR   | TCP_SENT      | SM1272     | BOOL               | N/A           |                    |
| 10  | VAR   | TCP_CLOSED    | SM1274     | BOOL               | N/A           |                    |
| 11  | VAR   | TCP_SENDING   | SM1275     | BOOL               | N/A           |                    |
| 12  | VAR   | TCP_ERROR     | SM1277     | BOOL               | N/A           |                    |
| 13  | VAR   | rRxByte       | N/A [Auto] | FB_ReadRxByte      | N/A           |                    |
| 14  | VAR   | rRxWord       | N/A [Auto] | FB_ReadRxWord      | N/A           |                    |
| 15  | VAR   | rRxHeader     | N/A [Auto] | FB_ReadRxHeader    | N/A           |                    |
| 16  | VAR   | wTxByte       | N/A [Auto] | FB_WriteTxByte     | N/A           |                    |
| 17  | VAR   | wTxWord       | N/A [Auto] | FB_WriteTxWord     | N/A           |                    |
| 18  | VAR   | wTxHeader     | N/A [Auto] | FB_WriteTxHeader   | N/A           |                    |
| 19  | VAR   | IoChangeByCmd | N/A [Auto] | BOOL               | N/A           |                    |
| 20  | VAR   | AUTEXT_ACT    | X1.0       | BOOL               | N/A           | KUKA $I_O_ACTCONF ($OUT[1]): AUT EXT selected and interface active |
| 21  | VAR   | PERI_RDY      | X1.2       | BOOL               | N/A           | KUKA $PERI_RDY ($OUT[3]) |
| 22  | VAR   | STOPMESS      | X1.3       | BOOL               | N/A           | KUKA $STOPMESS ($OUT[4]) |
| 23  | VAR   | PRO_ACT       | X1.4       | BOOL               | N/A           | KUKA $PRO_ACT ($OUT[5]) |
| 24  | VAR   | PGNO_REQ      | X1.5       | BOOL               | N/A           | KUKA PGNO_REQ ($OUT[6]) |
| 25  | VAR   | APPL_RUN      | X1.6       | BOOL               | N/A           | KUKA APPL_RUN ($OUT[7]) |
| 26  | VAR   | IN_HOME       | X1.7       | BOOL               | N/A           | KUKA $IN_HOME ($OUT[8]) |
| 27  | VAR   | EXT_START     | Y1.0       | BOOL               | FALSE         | KUKA $EXT_START ($IN[1]) |
| 28  | VAR   | DRIVES_ON     | Y1.1       | BOOL               | FALSE         | KUKA $DRIVES_ON ($IN[2]) |
| 29  | VAR   | CONF_MESS     | Y1.2       | BOOL               | FALSE         | KUKA $CONF_MESS ($IN[3]) |
| 30  | VAR   | PGNO_B0       | Y1.3       | BOOL               | FALSE         | KUKA PGNO bit 0 ($IN[4]) |
| 31  | VAR   | PGNO_B1       | Y1.4       | BOOL               | FALSE         | KUKA PGNO bit 1 ($IN[5]) |
| 32  | VAR   | JobPgno       | N/A [Auto] | WORD               | 0             | program 1..4 of the current job, 0 for resume |
| 33  | VAR   | RobotStep     | N/A [Auto] | WORD               | 0             | E_RSTEP, written by RobotIo |
| 34  | VAR   | RobotResult   | N/A [Auto] | WORD               | 0             | E_RRESULT, written by RobotIo |
| 35  | VAR   | CmdRun        | N/A [Auto] | BOOL               | FALSE         | set by FB_RunSetVar, cleared by RobotIo |
| 36  | VAR   | CmdResume     | N/A [Auto] | BOOL               | FALSE         | set by FB_RunSetVar, cleared by RobotIo |
| 37  | VAR   | CmdAbort      | N/A [Auto] | BOOL               | FALSE         | set by FB_RunSetVar, cleared by RobotIo |
| 38  | VAR   | Fail          | N/A [Auto] | BOOL               | FALSE         | read by TR_Fail, written by RobotIo |
| 39  | VAR   | ResumeBlocked | N/A [Auto] | BOOL               | FALSE         | APPL_RUN held while AUT EXT dropped; written by RobotIo, read by FB_RunSetVar |

`CmdStartCell` and `CmdSfyOk` were removed. Row 2: `LED_GREEN` keeps the address Y1.7 of the previous snapshot; `ProbeMap` no longer writes it.

# Programs

- onLostConnection [PRG,LD]
- RobotHandler [PRG,SFC]
- PCHandler [PRG,ST]
- RobotIo [PRG,ST]
- ProbeMap [PRG,LD]

## Task order

All programs are in one cyclic task, in this order: PCHandler, RobotIo, RobotHandler, then ProbeMap and onLostConnection. RobotIo runs before RobotHandler, so it reads the step flags left by the previous scan and records the result of a transition in the scan in which the transition fires.

## HWCONFIG

Ethernet-Advanced > Socket > TCP socket 1: Keep Alive Timer 10 s (the PC sends a SNAPSHOT heartbeat every 2 s from Milestone 4 on). The previous value was not recorded.

## onLostConnection

### Local Symbols

| No. | Class | Identifiers | Address    | Type           | Initial Value | Identifier Comment |
| --- | ----- | ----------- | ---------- | -------------- | ------------- | ------------------ |
| 1   | VAR   | runShutdown | N/A [Auto] | FB_RunShutdown | N/A           |                    |

*LD is in onLostConnection.drawio*

## RobotHandler

Start sequence only: it starts CELL.SRC on the KR C4 if needed, passes the program number and returns to ST_IDLE once the KR C4 has accepted the start, or on a failure. It does not supervise the running program. All KUKA outputs are written by RobotIo from the step flags. SFC drawing: `RobotHandler_SFC.drawio`.

### Local Symbols

| No. | Class | Identifiers | Address    | Type | Initial Value | Identifier Comment |
| --- | ----- | ----------- | ---------- | ---- | ------------- | ------------------ |
| 1   | VAR   | ST_IDLE     | N/A [Auto] | STEP | N/A           | initial step |
| 2   | VAR   | ST_PREP     | N/A [Auto] | STEP | N/A           | drives-on and acknowledge pulses |
| 3   | VAR   | ST_START    | N/A [Auto] | STEP | N/A           | $EXT_START until $PRO_ACT |
| 4   | VAR   | ST_SELECT   | N/A [Auto] | STEP | N/A           | program number and second $EXT_START |

### Chart

ST_IDLE (initial) → TR_Run → ST_PREP → TR_Prepared → ST_START → TR_Request → ST_SELECT → TR_Acked → jump ST_IDLE. Parallel transitions (divergence of sequence selection), each followed by a jump to ST_IDLE: after ST_PREP `TR_Fail`; after ST_START `TR_Running` and `TR_Fail` (left to right: TR_Request, TR_Running, TR_Fail); after ST_SELECT `TR_Fail`. No step has an action.

### Actions

None.

### Transitions [ST]

```
TR_Run      := CmdRun OR CmdResume;
TR_Prepared := PERI_RDY AND (NOT STOPMESS) AND (RobotHandler.ST_PREP.T >= 500);
TR_Running  := PRO_ACT AND APPL_RUN;
TR_Request  := PRO_ACT AND PGNO_REQ AND (JobPgno <> 0);
TR_Acked    := APPL_RUN AND (NOT PGNO_REQ);
TR_Fail     := Fail;
```

## PCHandler

### Local Symbols

| No. | Class | Identifiers | Address    | Type          | Initial Value | Identifier Comment |
| --- | ----- | ----------- | ---------- | ------------- | ------------- | ------------------ |
| 1   | VAR   | WANTTOSEND  | N/A [Auto] | BOOL          | FALSE         |                    |
| 2   | VAR   | CmdMonitor  | N/A [Auto] | FB_CmdMonitor | N/A           |                    |
| 3   | VAR   | IoMonitor   | N/A [Auto] | FB_IOMonitor  | N/A           |                    |
| 4   | VAR   | TxArbiter   | N/A [Auto] | FB_TxArbiter  | N/A           |                    |
| 5   | VAR   | TxBusy      | N/A [Auto] | BOOL          | N/A           |                    |
| 6   | VAR   | SendStarted | N/A [Auto] | BOOL          | FALSE         | SSEND issued, completion not yet seen |

```
/*
PLC Send Register:    D100 (32 bytes) - TO PC
PLC Receive Register: D200 (32 bytes) - FROM PC
*/

// ON THE FIRST SCAN
IF SM402 THEN
  WANTTOSEND  := FALSE;
  SendStarted := FALSE;
  LED_RUN    := TRUE;
  LED_GREEN  := FALSE;
  LED_RED1   := FALSE;
  LED_RED2   := FALSE;
  LED_RED3   := FALSE;
END_IF;

IF NOT TCP_CONNECTED & TCP_CLOSED THEN
  SOPEN(1,1,2);
END_IF;

TxBusy := WANTTOSEND OR TCP_SENDING;

TxArbiter(
  CmdPending := CmdMonitor.CmdPending,
  IoPending  := IoMonitor.IoPending,
  TxBusy     := TxBusy
);

CmdMonitor(BuildNow := TxArbiter.GrantCmd);
IoMonitor(BuildNow  := TxArbiter.GrantIo);

IF TxArbiter.StartSend THEN
  WANTTOSEND := TRUE;
END_IF;

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
```

## RobotIo

Computes `ResumeBlocked`, every KUKA output, the timeout, `Fail`, `RobotStep` and `RobotResult` from the RobotHandler step flags (`X` active, `T` time in the step in ms) of the previous scan; drives no KUKA input while `AUTEXT_ACT` is OFF.

### Local Symbols

| No. | Class | Identifiers | Address    | Type | Initial Value | Identifier Comment |
| --- | ----- | ----------- | ---------- | ---- | ------------- | ------------------ |
| 1   | VAR   | StepTimeout | N/A [Auto] | BOOL | FALSE         | active step exceeded its time limit |

```
// A program held by APPL_RUN must not be resumed after AUT EXT dropped (an operating-mode change):
// KRL keeps the velocities it planned in the other mode, so it is restarted from CELL.
// Cleared when APPL_RUN falls (CELL reselected with a BCO run, or the next number request).
IF NOT APPL_RUN THEN
  ResumeBlocked := FALSE;
ELSIF NOT AUTEXT_ACT THEN
  ResumeBlocked := TRUE;
END_IF;

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

// KUKA outputs: drives-on pulse, acknowledge pulse, start edges, program number.
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
```

## ProbeMap

*LD is in ProbeMap.drawio*: one network, contact X2.0 (WP-500 probe) → coil Y1.7 (EL1809 input 8, `$IN[8]`).

# Function Blocks

*Commands*:

- FB_RunReadIo [FB,ST]
- FB_RunReadReg [FB,ST]
- FB_RunWriteIo [FB,ST]
- FB_RunWriteReg [FB,ST]
- FB_RunWriteRaw [FB,ST]
- FB_RunSnapShot [FB,ST]
- FB_RunShutdown [FB,ST]
- FB_RunSetVar [FB,ST]
  *Basic*:
- FB_ReadModuleIo [FB,ST]
- FB_WriteTxRespOk [FB,ST] 
- FB_WriteTxRespErr [FB,ST]
- FB_WriteModuleIoMask [FB,ST]
  *Message*:
- FB_ReadRxByte [FB,ST]
- FB_ReadRxWord [FB,ST]
- FB_WriteTxByte [FB,ST]
- FB_WriteTxWord [FB,ST]
- FB_ReadRxHeader [FB,ST]
- FB_WriteTxHeader [FB,ST]
  *Monitor*:
- FB_CmdMonitor [FB,ST]
- FB_IOMonitor [FB,ST]
- FB_TxArbiter [FB,ST]
- FB_TRisingPulse [FB,LD]

#### Commands

##### FB_RunReadIo

*Local Symbols*

| No. | Class     | Identifiers | Address    | Type            | Initial Value | Identifier Comment |
| --- | --------- | ----------- | ---------- | --------------- | ------------- | ------------------ |
| 1   | VAR_INPUT | tid         | N/A [Auto] | WORD            | N/A           |                    |
| 2   | VAR       | rModuleIo   | N/A [Auto] | FB_ReadModuleIo | N/A           |                    |
| 3   | VAR       | dev         | N/A [Auto] | WORD            | 0             |                    |
| 4   | VAR       | module      | N/A [Auto] | WORD            | N/A           |                    |
| 5   | VAR       | state       | N/A [Auto] | WORD            | N/A           |                    |

```
rRxWord(idx:=5, val=>dev);
rRxByte(idx:=13, val=>module);
rModuleIo(dev:=dev, 
          module:=module,
          val=>state);

// write responce
wTxHeader(type:=E_TYPE.RESP_OK,
          tid:=tid,
          paylen:=5);
          
wTxByte(idx:=6,  val:=E_CMD.READ_IO);
wTxByte(idx:=7,  val:=0); // STATUS
wTxWord(idx:=4,  val:=dev);
wTxByte(idx:=10, val:=module);
wTxByte(idx:=11, val:=state);
```

##### FB_RunReadReg

*Local Symbols*

| No. | Class     | Identifiers | Address    | Type | Initial Value | Identifier Comment |
| ---:| --------- | ----------- | ---------- | ---- | ------------- | ------------------ |
| 1   | VAR_INPUT | tid         | N/A [Auto] | WORD | N/A           |                    |
| 2   | VAR       | addr        | N/A [Auto] | WORD | N/A           |                    |
| 3   | VAR       | val         | N/A [Auto] | WORD | N/A           |                    |

```
rRxWord(idx:=6, val=>addr);
E0 := addr;
val := D0@E0;

// write responce
wTxHeader(type:=E_TYPE.RESP_OK,
          tid:=tid,
          paylen:=8);
          
wTxByte(idx:=6, val:=E_CMD.READ_REG);
wTxByte(idx:=7, val:=16#0000); // STATUS
wTxWord(idx:=4, val:=E_DEV.DEV_D);
wTxWord(idx:=5, val:=addr);
wTxWord(idx:=6, val:=val);
```

##### FB_RunWriteIo

*Local Symbols*

| No. | Class     | Identifiers | Address    | Type                 | Initial Value | Identifier Comment |
| ---:| --------- | ----------- | ---------- | -------------------- | ------------- | ------------------ |
| 1   | VAR_INPUT | tid         | N/A [Auto] | WORD                 | 0             |                    |
| 2   | VAR       | wModuleIo   | N/A [Auto] | FB_WriteModuleIoMask | N/A           |                    |
| 3   | VAR       | dev         | N/A [Auto] | WORD                 | 0             |                    |
| 4   | VAR       | module      | N/A [Auto] | WORD                 | 0             |                    |
| 5   | VAR       | andMask     | N/A [Auto] | WORD                 | 0             |                    |
| 6   | VAR       | orMask      | N/A [Auto] | WORD                 | 0             |                    |
| 7   | VAR       | newState    | N/A [Auto] | WORD                 | 0             |                    |
| 8   | VAR       | wRespErr    | N/A [Auto] | FB_WriteTxRespErr    | N/A           |                    |

```
rRxByte(idx:=13, val=>module);
IF module = 1 THEN
  // module 1 (Y1.0..Y1.7) belongs to the KUKA interface and the probe copy
  wRespErr(tid:=tid, cmd:=E_CMD.WRITE_IO, err:=E_ERR.BAD_REQUEST, code:=E_CODE.BAD_MOD);
ELSE
  rRxWord(idx:=5,  val=>dev);
  rRxByte(idx:=14, val=>andMask);
  rRxByte(idx:=15, val=>orMask);
  wModuleIoMask(dev:=dev,
                module:=module,
                andMask:=andMask,
                orMask:=orMask,
                newState=>newState);

  // write responce
  wTxHeader(tid:=tid, type:=E_TYPE.RESP_OK, paylen:=6);
  wTxByte(idx:=6,  val:=E_CMD.WRITE_IO);
  wTxByte(idx:=7,  val:=16#0000);
  wTxWord(idx:=4,  val:=E_DEV.DEV_Y);
  wTxByte(idx:=10, val:=module);
  wTxByte(idx:=11, val:=newState);
END_IF;
```

##### FB_RunWriteReg

*Local Symbols*

| No. | Class     | Identifiers | Address    | Type | Initial Value | Identifier Comment |
| ---:| --------- | ----------- | ---------- | ---- | ------------- | ------------------ |
| 1   | VAR_INPUT | tid         | N/A [Auto] | WORD | N/A           |                    |
| 2   | VAR       | addr        | N/A [Auto] | WORD | N/A           |                    |
| 3   | VAR       | val         | N/A [Auto] | WORD | N/A           |                    |

```
rRxWord(idx:=6, val=>addr);
rRxWord(idx:=7, val=>val);

E0 := addr;
D0@E0 := val;

// write responce
wTxHeader(type:=E_TYPE.RESP_OK,
          tid:=tid,
          paylen:=8);
          
wTxByte(idx:=6, val:=E_CMD.WRITE_REG);
wTxByte(idx:=7, val:=16#0000); // STATUS
wTxWord(idx:=4, val:=E_DEV.DEV_D);
wTxWord(idx:=5, val:=addr);
wTxWord(idx:=6, val:=val);  
```

##### FB_RunWriteRaw

*Local Symbols*

| No. | Class     | Identifiers | Address    | Type | Initial Value | Identifier Comment |
| ---:| --------- | ----------- | ---------- | ---- | ------------- | ------------------ |
| 1   | VAR_INPUT | tid         | N/A [Auto] | WORD | N/A           |                    |
| 2   | VAR_INPUT | paylen      | N/A [Auto] | WORD | N/A           |                    |
| 3   | VAR       | wn          | N/A [Auto] | INT  | N/A           |                    |
| 4   | VAR       | val         | N/A [Auto] | WORD | N/A           |                    |
| 5   | VAR       | i           | N/A [Auto] | INT  | N/A           |                    |

```
wn := (paylen + 1) / 2 - 1; // -1 for CMD+FLAGS
FOR i := 5 TO (5+wn-1) DO
  rRxWord(idx := i, val => val);
  E0 := i - 5;
  D300@E0 := val;
END_FOR;

// write responce
wTxHeader(type:=E_TYPE.RESP_OK,
          tid:=tid,
          paylen:=paylen);

wTxByte(idx:=6, val:=E_CMD.WRITE_RAW);
wTxByte(idx:=7, val:=16#0000); // STATUS

FOR i := 4 TO (4+wn-1) BY 1 DO
  E0 := i - 4;
  wTxWord(idx:=i, val:=D300@E0);
END_FOR;

 // if paylen is odd -> last word 0x00AA, but expected 0xAA00 => swap bytes
IF (paylen MOD 2 <> 0) THEN
   i := i - 1;
   Tx[i] := (Tx[i] * 256) AND 16#FF00;
END_IF;
```

##### FB_RunSnapShot

*Local Symbols*

| No. | Class     | Identifiers | Address    | Type            | Initial Value | Identifier Comment |
| ---:| --------- | ----------- | ---------- | --------------- | ------------- | ------------------ |
| 1   | VAR_INPUT | tid         | N/A [Auto] | WORD            | N/A           |                    |
| 2   | VAR       | rModuleIo   | N/A [Auto] | FB_ReadModuleIo | N/A           |                    |
| 3   | VAR       | x_1         | N/A [Auto] | WORD            | N/A           |                    |
| 4   | VAR       | y_1         | N/A [Auto] | WORD            | N/A           |                    |
| 5   | VAR       | x_2         | N/A [Auto] | WORD            | N/A           |                    |
| 6   | VAR       | y_2         | N/A [Auto] | WORD            | N/A           |                    |

```
rModuleIo(dev:=E_DEV.DEV_X, module:=1, val=>x_1);
rModuleIo(dev:=E_DEV.DEV_Y, module:=1, val=>y_1);
rModuleIo(dev:=E_DEV.DEV_X, module:=2, val=>x_2);
rModuleIo(dev:=E_DEV.DEV_Y, module:=2, val=>y_2);

// write responce
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
```

##### FB_RunShutdown

*Local Symbols*

| No. | Class | Identifiers | Address    | Type | Initial Value | Identifier Comment |
| ---:| ----- | ----------- | ---------- | ---- | ------------- | ------------------ |
| 1   | VAR   | var1        | N/A [Auto] | WORD | 0             |                    |

```
LED_GREEN:=FALSE;
LED_RED1:=FALSE;
LED_RED2:=FALSE;
LED_RED3:=FALSE;
```

##### FB_RunSetVar

*Local Symbols*

| No. | Class     | Identifiers | Address    | Type              | Initial Value | Identifier Comment |
| ---:| --------- | ----------- | ---------- | ----------------- | ------------- | ------------------ |
| 1   | VAR       | variable    | N/A [Auto] | INT               | N/A           |                    |
| 2   | VAR       | attr        | N/A [Auto] | WORD              | N/A           |                    |
| 3   | VAR_INPUT | tid         | N/A [Auto] | WORD              | N/A           |                    |
| 4   | VAR       | wRespErr    | N/A [Auto] | FB_WriteTxRespErr | N/A           |                    |
| 5   | VAR       | accepted    | N/A [Auto] | BOOL              | FALSE         |                    |
| 6   | VAR       | code        | N/A [Auto] | WORD              | 0             |                    |

```
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
    ELSIF (NOT AUTEXT_ACT) OR PRO_ACT OR (NOT APPL_RUN) OR STOPMESS OR ResumeBlocked THEN
      // only in AUT EXT, only a stopped application program, only after the stop message was
      // acknowledged on the smartPAD ($CONF_MESS does not clear the STOP key's "Passive STOP"),
      // and not after an operating-mode change (ResumeBlocked, written by RobotIo)
      code := E_CODE.NOT_ALLOWED;
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
```

#### Basic

##### FB_ReadModuleIo

*Local Symbols*

| No. | Class      | Identifiers | Address    | Type  | Initial Value | Identifier Comment |
| ---:| ---------- | ----------- | ---------- | ----- | ------------- | ------------------ |
| 1   | VAR_INPUT  | dev         | N/A [Auto] | E_Dev | N/A           |                    |
| 2   | VAR_INPUT  | module      | N/A [Auto] | INT   | N/A           |                    |
| 3   | VAR_OUTPUT | val         | N/A [Auto] | WORD  | N/A           |                    |
| 4   | VAR        | r           | N/A [Auto] | WORD  | 0             |                    |

```
r := 0;
IF module = 1 THEN
  IF dev = E_Dev.DEV_X THEN
    val := (X1 AND 16#00FF);
    /*
    IF X1.0 THEN r := r OR 1;  END_IF; IF X1.1 THEN r := r OR 2;   END_IF;
    IF X1.2 THEN r := r OR 4;  END_IF; IF X1.3 THEN r := r OR 8;   END_IF;
    IF X1.4 THEN r := r OR 16; END_IF; IF X1.5 THEN r := r OR 32;  END_IF;
    IF X1.6 THEN r := r OR 64; END_IF; IF X1.7 THEN r := r OR 128; END_IF;
    */ 
  ELSIF dev = E_Dev.DEV_Y THEN
    val := (Y1 AND 16#00FF);
    /*
    IF Y1.0 THEN r := r OR 1;  END_IF; IF Y1.1 THEN r := r OR 2;   END_IF;
    IF Y1.2 THEN r := r OR 4;  END_IF; IF Y1.3 THEN r := r OR 8;   END_IF;
    IF Y1.4 THEN r := r OR 16; END_IF; IF Y1.5 THEN r := r OR 32;  END_IF;
    IF Y1.6 THEN r := r OR 64; END_IF; IF Y1.7 THEN r := r OR 128; END_IF;
    */
  END_IF;
ELSIF module = 2 THEN
  IF dev = E_Dev.DEV_X THEN
    val := (X2 AND 16#00FF);
    /*
    IF X2.0 THEN r := r OR 1;  END_IF; IF X2.1 THEN r := r OR 2;  END_IF;
    IF X2.2 THEN r := r OR 4;  END_IF; IF X2.3 THEN r := r OR 8;  END_IF;
    IF X2.4 THEN r := r OR 16; END_IF; IF X2.5 THEN r := r OR 32;  END_IF;
    IF X2.6 THEN r := r OR 64; END_IF; IF X2.7 THEN r := r OR 128; END_IF;
    */
  ELSIF dev = E_Dev.DEV_Y THEN
    val := (Y2 AND 16#00FF);
    /*
    IF Y2.0 THEN r := r OR 1;  END_IF; IF Y2.1 THEN r := r OR 2;  END_IF;
    IF Y2.2 THEN r := r OR 4;  END_IF; IF Y2.3 THEN r := r OR 8;  END_IF;
    IF Y2.4 THEN r := r OR 16; END_IF; IF Y2.5 THEN r := r OR 32;  END_IF;
    IF Y2.6 THEN r := r OR 64; END_IF; IF Y2.7 THEN r := r OR 128; END_IF;
    */
  END_IF;
END_IF;
```

##### FB_WriteTxRespOk

*Local Symbols*

| No. | Class     | Identifiers | Address    | Type  | Initial Value | Identifier Comment |
| ---:| --------- | ----------- | ---------- | ----- | ------------- | ------------------ |
| 1   | VAR_INPUT | tid         | N/A [Auto] | WORD  | N/A           |                    |
| 2   | VAR_INPUT | cmd         | N/A [Auto] | E_Cmd | N/A           |                    |

```
/*
wTxByte(idx:=6, val:=cmd);
wTxByte(idx:=7, val:=16#0000);

CASE cmd OF 
  E_CMD.WRITE_IO:
    wTxHeader(tid:=tid, type:=E_TYPE.RESP_OK, paylen:=5);
    
    
  E_CMD.SNAPSHOT:
    wTxHeader(tid:=tid, type:=E_TYPE.RESP_OK, paylen:=1);
END_CASE;
*/
```

##### FB_WriteTxRespErr

*Local Symbols*

| No. | Class     | Identifiers | Address    | Type | Initial Value | Identifier Comment |
| ---:| --------- | ----------- | ---------- | ---- | ------------- | ------------------ |
| 1   | VAR_INPUT | tid         | N/A [Auto] | WORD | 0             |                    |
| 2   | VAR_INPUT | cmd         | N/A [Auto] | WORD | N/A           |                    |
| 3   | VAR_INPUT | err         | N/A [Auto] | WORD | N/A           |                    |
| 4   | VAR_INPUT | code        | N/A [Auto] | WORD | N/A           |                    |

```
// payload starts at byte 6: command, error, then the 16-bit code in word 4 (bytes 8-9)
wTxHeader(tid:=tid, type:=E_TYPE.RESP_ERR, paylen:=4);
wTxByte(idx:=6, val:=cmd);
wTxByte(idx:=7, val:=err);
wTxWord(idx:=4, val:=code);
```

##### FB_WriteModuleIoMask

*Local Symbols*

| No. | Class      | Identifiers | Address    | Type            | Initial Value | Identifier Comment |
| ---:| ---------- | ----------- | ---------- | --------------- | ------------- | ------------------ |
| 1   | VAR_INPUT  | dev         | N/A [Auto] | E_Dev           | N/A           |                    |
| 2   | VAR_INPUT  | module      | N/A [Auto] | INT             | N/A           |                    |
| 3   | VAR_INPUT  | andMask     | N/A [Auto] | WORD            | N/A           |                    |
| 4   | VAR_INPUT  | orMask      | N/A [Auto] | WORD            | N/A           |                    |
| 5   | VAR        | rModuleIo   | N/A [Auto] | FB_ReadModuleIo | N/A           |                    |
| 6   | VAR        | curState    | N/A [Auto] | WORD            | 0             |                    |
| 7   | VAR_OUTPUT | newState    | N/A [Auto] | WORD            | N/A           |                    |

```
// write IO external 8DI/8DO module state to Tx array

rModuleIo(dev:=dev, module:=module, val=>curState);
newState := (curState AND andMask) OR orMask;
IF module = 1 THEN
  Y1.0 := ((newState AND 1) <> 0);   Y1.1 := ((newState AND 2) <> 0);
  Y1.2 := ((newState AND 4) <> 0);   Y1.3 := ((newState AND 8) <> 0);
  Y1.4 := ((newState AND 16) <> 0);  Y1.5 := ((newState AND 32) <> 0);
  Y1.6 := ((newState AND 64) <> 0);  Y1.7 := ((newState AND 128) <> 0);
ELSIF module = 2 THEN
  Y2.0 := ((newState AND 1) <> 0);   Y2.1 := ((newState AND 2) <> 0);
  Y2.2 := ((newState AND 4) <> 0);   Y2.3 := ((newState AND 8) <> 0);
  Y2.4 := ((newState AND 16) <> 0);  Y2.5 := ((newState AND 32) <> 0);
  Y2.6 := ((newState AND 64) <> 0);  Y2.7 := ((newState AND 128) <> 0);
END_IF;
```

#### Message

##### FB_ReadRxByte

*Local Symbols*

| No. | Class      | Identifiers | Address    | Type | Initial Value | Identifier Comment |
| ---:| ---------- | ----------- | ---------- | ---- | ------------- | ------------------ |
| 1   | VAR_INPUT  | idx         | N/A [Auto] | WORD | 0             |                    |
| 2   | VAR_OUTPUT | val         | N/A [Auto] | WORD | 0             |                    |
| 3   | VAR        | wi          | N/A [Auto] | WORD | 0             |                    |
| 4   | VAR        | w           | N/A [Auto] | WORD | 0             |                    |
| 5   | VAR        | isMSB       | N/A [Auto] | BOOL | FALSE         |                    |

```
// Read 1 “byte” (as WORD) from Rx at byte offset idx

wi := idx / 2; // word index
isMSB := (idx MOD 2) = 0; // odd - LSB, even - MSB
w := Rx[wi];
IF isMSB THEN
  val := ( (w AND 16#FF00) / 256 ) AND 16#00FF; // MSB
ELSE 
  val := w AND 16#00FF; // LSB 
END_IF;
```

##### FB_ReadRxWord

*Local Symbols*

| No. | Class      | Identifiers | Address    | Type | Initial Value | Identifier Comment |
| ---:| ---------- | ----------- | ---------- | ---- | ------------- | ------------------ |
| 1   | VAR_INPUT  | idx         | N/A [Auto] | WORD | 0             |                    |
| 2   | VAR_OUTPUT | val         | N/A [Auto] | WORD | 0             |                    |

```
// Read a 16-bit WORD at idx, MSB - left, LSB - right
val := Rx[idx];
```

##### FB_WriteTxByte

*Local Symbols*

| No. | Class     | Identifiers | Address    | Type | Initial Value | Identifier Comment |
| ---:| --------- | ----------- | ---------- | ---- | ------------- | ------------------ |
| 1   | VAR_INPUT | idx         | N/A [Auto] | WORD | 0             |                    |
| 2   | VAR_INPUT | val         | N/A [Auto] | WORD | 0             |                    |
| 3   | VAR       | wi          | N/A [Auto] | WORD | 0             |                    |
| 4   | VAR       | w           | N/A [Auto] | WORD | 0             |                    |
| 5   | VAR       | isMSB       | N/A [Auto] | BOOL | FALSE         |                    |

```
// Write one “byte” (as Word) into Tx at byte offset idx

wi := idx / 2;
isMSB := (idx MOD 2) = 0;
w := Tx[wi];
IF isMSB THEN
  w := ( (val AND 16#00FF) * 256) OR (w AND 16#00FF);  // MSB
ELSE
  w := (w AND 16#FF00) OR val; // LSB
END_IF;
Tx[wi] := w;
```

##### FB_WriteTxWord

*Local Symbols*

| No. | Class     | Identifiers | Address    | Type | Initial Value | Identifier Comment |
| ---:| --------- | ----------- | ---------- | ---- | ------------- | ------------------ |
| 1   | VAR_INPUT | idx         | N/A [Auto] | INT  | 0             |                    |
| 2   | VAR_INPUT | val         | N/A [Auto] | WORD | 0             |                    |

```
// Put a 16-bit WORD val at idx, MSB - left, LSB - right

Tx[idx] := val;
```

##### FB_ReadRxHeader

*Local Symbols*

| No. | Class      | Identifiers | Address    | Type          | Initial Value | Identifier Comment |
| ---:| ---------- | ----------- | ---------- | ------------- | ------------- | ------------------ |
| 1   | VAR        | rRxWord     | N/A [Auto] | FB_ReadRxWord | N/A           |                    |
| 2   | VAR        | rRxByte     | N/A [Auto] | FB_ReadRxByte | N/A           |                    |
| 3   | VAR_OUTPUT | header      | N/A [Auto] | T_Header      | N/A           |                    |

```
rRxWord(idx:=1, val=>header.magic);
rRxByte(idx:=4, val=>header.ver);
rRxByte(idx:=5, val=>header.type);
rRxByte(idx:=6, val=>header.tid);
rRxByte(idx:=7, val=>header.len);
```

##### FB_WriteTxHeader

*Local Symbols*

| No. | Class     | Identifiers | Address    | Type           | Initial Value | Identifier Comment |
| ---:| --------- | ----------- | ---------- | -------------- | ------------- | ------------------ |
| 1   | VAR_INPUT | tid         | N/A [Auto] | WORD           | 0             |                    |
| 2   | VAR_INPUT | type        | N/A [Auto] | WORD           | 0             |                    |
| 3   | VAR_INPUT | paylen      | N/A [Auto] | WORD           | 0             |                    |
| 4   | VAR       | wTxWord     | N/A [Auto] | FB_WriteTxWord | N/A           |                    |
| 5   | VAR       | wTxByte     | N/A [Auto] | FB_WriteTxByte | N/A           |                    |

```
wTxWord(idx:=0, val:=16#AA55); // MAGIC
wTxByte(idx:=2, val:=16#0001); // VER
wTxByte(idx:=3, val:=type);    // TYPE
wTxByte(idx:=4, val:=tid);     // TID
wTxByte(idx:=5, val:=paylen);  // LEN
```

#### Monitor

##### FB_CmdMonitor

*Local Symbols*

| No. | Class      | Identifiers | Address    | Type           | Initial Value | Identifier Comment                                    |
| ---:| ---------- | ----------- | ---------- | -------------- | ------------- | ----------------------------------------------------- |
| 1   | VAR_INPUT  | BuildNow    | N/A [Auto] | BOOL           | N/A           | From arbiter: “you may build response into D100 now”  |
| 2   | VAR        | RxBytes     | N/A [Auto] | WORD           | N/A           | Rx size in bytes                                      |
| 3   | VAR        | RxHeader    | N/A [Auto] | T_Header       | N/A           |                                                       |
| 4   | VAR        | Cmd         | N/A [Auto] | E_Cmd          | N/A           |                                                       |
| 5   | VAR        | runReadIo   | N/A [Auto] | FB_RunReadIo   | N/A           |                                                       |
| 6   | VAR        | runReadReg  | N/A [Auto] | FB_RunReadReg  | N/A           |                                                       |
| 7   | VAR        | runWriteIo  | N/A [Auto] | FB_RunWriteIo  | N/A           |                                                       |
| 8   | VAR        | runWriteReg | N/A [Auto] | FB_RunWriteReg | N/A           |                                                       |
| 9   | VAR        | runWriteRaw | N/A [Auto] | FB_RunWriteRaw | N/A           |                                                       |
| 10  | VAR        | runSnapShot | N/A [Auto] | FB_RunSnapShot | N/A           |                                                       |
| 11  | VAR        | runSetVar   | N/A [Auto] | FB_RunSetVar   | N/A           |                                                       |
| 12  | VAR        | Pending     | N/A [Auto] | BOOL           | FALSE         |                                                       |
| 13  | VAR_OUTPUT | IsCmd       | N/A [Auto] | BOOL           | FALSE         | 1-scan pulse: new command received & queued           |
| 14  | VAR_OUTPUT | CmdPending  | N/A [Auto] | BOOL           | N/A           | level: TRUE while there is an unsent response pending |
| 15  | VAR        | wRespErr    | N/A [Auto] | FB_WriteTxRespErr | N/A        | builds RESP_ERR frames |

```
IsCmd := FALSE;

// TAKE ONE RECEIVED REQUEST (SOPEN mode 2: Rx[0] = received byte count, frame from Rx[1])
// No TCP_ERROR condition: SM1277 is latched and refreshed only by socket instructions.
IF TCP_CONNECTED & (Rx[0] <> 0) AND (NOT Pending) THEN
  RxBytes := Rx[0];
  rRxHeader(header => RxHeader);
  rRxByte(idx:=8, val=>Cmd);
  Pending := TRUE;
  IsCmd   := TRUE;
  Rx[0] := 0;
END_IF;

// BUILD THE REPLY WHEN THE ARBITER GRANTS D100
IF BuildNow AND Pending THEN
  IF (RxHeader.magic XOR 16#AA55) <> 0 THEN  // a direct <> 16#AA55 gives error 1003 at LD
    wRespErr(tid:=RxHeader.tid, cmd:=Cmd, err:=E_ERR.BAD_REQUEST, code:=E_CODE.BAD_MAGIC);
  ELSIF RxHeader.ver <> 1 THEN
    wRespErr(tid:=RxHeader.tid, cmd:=Cmd, err:=E_ERR.BAD_REQUEST, code:=E_CODE.BAD_VER);
  ELSIF RxHeader.type <> E_TYPE.REQ THEN
    wRespErr(tid:=RxHeader.tid, cmd:=Cmd, err:=E_ERR.BAD_REQUEST, code:=E_CODE.BAD_TYPE);
  ELSIF RxBytes < (6 + RxHeader.len) THEN
    wRespErr(tid:=RxHeader.tid, cmd:=Cmd, err:=E_ERR.BAD_REQUEST, code:=E_CODE.LEN_MISMATCH);
  ELSE
    CASE Cmd OF
      E_CMD.READ_IO:
        runReadIo(tid:=RxHeader.tid);
      E_CMD.READ_REG:
        runReadReg(tid:=RxHeader.tid);
      E_CMD.WRITE_IO:
        runWriteIo(tid:=RxHeader.tid);
        IoChangeByCmd := TRUE;
      E_CMD.WRITE_REG:
        runWriteReg(tid:=RxHeader.tid);
      E_CMD.WRITE_RAW:
        runWriteRaw(tid:=RxHeader.tid, paylen:=RxHeader.len);
      E_CMD.SNAPSHOT:
        runSnapShot(tid:=RxHeader.tid);
      E_CMD.SET_VAR:
        runSetVar(tid:=RxHeader.tid);
    ELSE
      wRespErr(tid:=RxHeader.tid, cmd:=Cmd, err:=E_ERR.BAD_REQUEST, code:=E_CODE.BAD_CMD);
    END_CASE;
  END_IF;
  Pending := FALSE;
END_IF;

CmdPending := Pending;
```

##### FB_IOMonitor

*Local Symbols*

| No. | Class      | Identifiers | Address    | Type | Initial Value | Identifier Comment                                     |
| ---:| ---------- | ----------- | ---------- | ---- | ------------- | ------------------------------------------------------ |
| 1   | VAR_INPUT  | BuildNow    | N/A [Auto] | BOOL | N/A           | Arbiter: "build IO frame into D100 now"                |
| 2   | VAR        | LastX1      | N/A [Auto] | WORD | 0             |                                                        |
| 3   | VAR        | LastY1      | N/A [Auto] | WORD | 0             |                                                        |
| 4   | VAR        | LastX2      | N/A [Auto] | WORD | 0             |                                                        |
| 5   | VAR        | LastY2      | N/A [Auto] | WORD | 0             |                                                        |
| 6   | VAR        | AnyChange   | N/A [Auto] | BOOL | N/A           |                                                        |
| 7   | VAR_OUTPUT | IoPending   | N/A [Auto] | BOOL | N/A           | TRUE if IO state changed vs last sent and not yet sent |
| 8   | VAR        | WasConnected | N/A [Auto] | BOOL | FALSE        | TCP_CONNECTED of the previous scan |
| 9   | VAR        | LastStep    | N/A [Auto] | WORD | 0             | RobotStep last sent |
| 10  | VAR        | LastProg    | N/A [Auto] | WORD | 0             | JobPgno last sent |
| 11  | VAR        | LastResult  | N/A [Auto] | WORD | 0             | RobotResult last sent |

Sends the IO change frame: type CHG (0xAF), payload 0xD0, X1, Y1, X2, Y2, RobotStep, JobPgno, RobotResult (RobotStep, JobPgno and RobotResult are written by RobotIo).

```
IF SM402 THEN
    IoPending := TRUE;
END_IF;

// NEW PC CONNECTION: SEND THE CURRENT IO STATE AT ONCE
IF TCP_CONNECTED AND NOT WasConnected THEN
  IoPending := TRUE;
END_IF;
WasConnected := TCP_CONNECTED;

IF IoChangeByCmd THEN
  LastY1 := Y1;
  LastY2 := Y2;
  IoChangeByCmd := FALSE;
END_IF;

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
```

##### FB_TxArbiter

*Local Symbols*

| No. | Class      | Identifiers | Address    | Type      | Initial Value | Identifier Comment                        |
| ---:| ---------- | ----------- | ---------- | --------- | ------------- | ----------------------------------------- |
| 1   | VAR_INPUT  | CmdPending  | N/A [Auto] | BOOL      | N/A           | TRUE if CmdMonitor has a response waiting |
| 2   | VAR_INPUT  | IoPending   | N/A [Auto] | BOOL      | N/A           | TRUE if IoMonitor has IO msg waiting      |
| 3   | VAR_INPUT  | TxBusy      | N/A [Auto] | BOOL      | N/A           | TRUE if we must NOT start a new message   |
| 4   | VAR_OUTPUT | GrantCmd    | N/A [Auto] | BOOL      | N/A           | 1-scan: allow CmdMonitor to build now     |
| 5   | VAR_OUTPUT | GrantIo     | N/A [Auto] | BOOL      | N/A           | 1-scan: allow IoMonitor to build now      |
| 6   | VAR_OUTPUT | StartSend   | N/A [Auto] | BOOL      | N/A           | 1-scan: main should set WANTTOSEND        |
| 7   | VAR_OUTPUT | CurrentSend | N/A [Auto] | E_SENDSRC | N/A           | who was chosen                            |

```
GrantCmd  := FALSE;
GrantIo   := FALSE;
StartSend := FALSE;

IF NOT TxBusy THEN
  IF CmdPending THEN
    CurrentSend := E_SENDSRC.SEND_CMD;
    GrantCmd    := TRUE;
    StartSend   := TRUE;
  ELSIF IoPending THEN
    CurrentSend := E_SENDSRC.SEND_IO;
    GrantIo     := TRUE;
    StartSend   := TRUE; 
  ELSE
    CurrentSend := E_SENDSRC.SEND_NONE;
  END_IF;
END_IF;
```

##### FB_TRisingPulse

*Local Symbols*

| No. | Class      | Identifiers | Address    | Type | Initial Value | Identifier Comment |
| ---:| ---------- | ----------- | ---------- | ---- | ------------- | ------------------ |
| 1   | VAR_IN_OUT | Q           | N/A [Auto] | BOOL | N/A           |                    |
| 2   | VAR_INPUT  | T_ms        | N/A [Auto] | INT  | 0             |                    |
| 3   | VAR        | PulseOn     | N/A [Auto] | BOOL | TRUE          |                    |

*LD is in FB_TRisingPulse.drawio*
