# RSI_Chamfer_Hole nominal run: controller/PC contract

The sequence is HOME, PTP PsIn, LIN LeadStart, lead-in / machining / lead-out
under RSI, LIN PsOut and HOME. Only completed RSI permits return. Force correction is disabled.

Controller files:
- `RSI_Chamfer_Hole.src` and `RSI_Chamfer_Hole.dat`: the KRL program (module `RSI_Chamfer_Hole`)
  with its local `READ_JOB()` file reader.
- `RSI_Chamfer_Hole.rsi`, `RSI_Chamfer_Hole.rsi.xml` and `RSI_Chamfer_Hole.rsi.diagram`: the RSI Visual context, loaded by `RSI_CREATE("RSI_Chamfer_Hole.rsi")`. Copy these from the RSI Visual export; do not hand-edit them.
- `RSI_EthernetConfig.xml`: the Ethernet configuration.
- `FILE_UTILS/REMOUNT_SHARE.src`, `FILE_UTILS/READ_POS.src` and `FILE_UTILS/READ_FRAME.src`:
  shared KRL helpers, installed as separate SRC files on the controller. The first routine of
  an SRC file is global without the `GLOBAL` keyword. Remove a previously installed
  `CLOSE_FILES.src`; nothing calls it.

PC side:
- `src/trajectories/chamfer/chamferrsistream.cpp`: computes the corrections.
- `src/application/scene/scenemachiningpreparation.cpp`: RSI Sampling and the shared file.
- `src/devices/rsi/rsidevice.cpp`: the UDP partner.

## Frames

KRL sets `$BASE=$NULLFRAME`, so BASE coincides with WORLD. The application's numerical
poses are robot-base-relative; using them unchanged also requires robot-base/WORLD alignment.
This SRC does not set or read `$ROBROOT`; it does not establish that additional alignment.
KRL sets `$TOOL` from ToolFrame:
the flange-to-TCP transform saved with the sampled path (the collet frame combined
with the TCP), not a `TOOL_DATA` entry. `$LOAD` is not set by the program; the load
data already active on the controller stays in effect. XYZ are in mm.
ABC are Z-Y-X Euler angles in degrees. POSCORR1 uses RefCorrSys = World. Each relative
A/B/C offset defines a rotation in WORLD axes at the TCP. Compose its rotation matrix
before the preceding TCP rotation matrix; do not add the offset to absolute TCP Euler angles.

## Signals

ETHERNET1 exchanges these signals with the PC at 192.168.1.100:5555 (sensor type
ImFree). The controller also sends RIst, AIPos and MACur internally; the PC
ignores them.

| Direction | Port | XML | Connection |
| --- | --- | --- | --- |
| Controller → PC | In1 | `Start`, BOOL | SEN_PINT1 reads `$SEN_PINT[1]` |
| Controller → PC | In2 | `Result`, LONG | SEN_PINT2 reads `$SEN_PINT[2]` |
| PC → controller | Out1..6 | `RKorr.X/Y/Z/A/B/C`, DOUBLE, HOLDON 0 | POSCORR1 CorrX..CorrC |
| PC → controller | Out7 | `Stop`, BOOL, HOLDON 1 | STOP1, Mode ExitMoveCorr |
| PC → controller | Out8 | `Result`, LONG, HOLDON 1 | MAP2SEN_PINT1 writes `$SEN_PINT[2]` |

KRL owns `$SEN_PINT[1]`:
- It is cleared at the start of the program.
- It is set to 1 immediately before `RSI_MOVECORR`.
- It is cleared after `Completed` has been read.

`$SEN_PINT[2]` is written every cycle by the PC's Result through
MAP2SEN_PINT1. Only `Completed=$SEN_PINT[2]==1` and a successful `RSI_OFF()` allow LIN PsOut
and the return HOME.

RSI calls use the documented defaults: `RSI_CREATE` activates the container, `RSI_ON(#RELATIVE)`
runs in `#IPO_FAST` and `RSI_MOVECORR()` stops with `#RSIBRAKE`. There is no container ID or
`RSI_DELETE`: `RSI_RESET()` at program start deletes every container left by an earlier run.
`RSI_ON`/`RSI_OFF` act on all created containers, so any other RSI program on this controller
must also start with `RSI_RESET()`.

## PC behavior

The controller sends one `<Rob>` packet per 4 ms cycle. The PC answers it within
the same cycle, echoing its IPOC. If several packets are queued, it answers only
the newest one.

- **Start = 0:** zero corrections, Stop 0, Result 0, and the stream index returns to the first correction.
- **Start = 1 with a stream:** one correction per packet, Result 0, Stop 0. The stream ends with 50 zero corrections so the robot settles.
- **After the last correction:** zero corrections with Result 1, until a packet echoes Result = 1.
- **Then:** zero corrections with Result 1 and Stop 1, until Start returns to 0.
- **Start = 1 without a stream:** zero corrections with Stop 1 and Result 0. RSI_MOVECORR ends, and KRL performs no return motion.

Every accepted RSI Sampling click discards the GUI-side result and queues an empty correction
array before starting a fresh sampling worker, even when no GUI result is held. On ControlIO,
every correction update immediately replaces the array and resets the sender to Idle at index 0,
including from Streaming, Finished or Stopping. A successful current sample and shared-file export
install the new array; failed sampling/export leaves the sender empty. The first accepted Start = 1
request after installation sends increments[0], then subsequent requests send the following values;
no preceding Start = 0 packet is required. While empty, Start = 1 sends Stop 1 and Result 0.
Before clearing/installing, the device discards requests already buffered in its socket without
replying or advancing either stream; outgoing accepted-peer rows remain in the outgoing capture.
It resets the learned UDP source port and decode-warning flag, learning the port again from a
subsequent valid request. Configured addresses and the bound socket remain available.
These queued updates reset PC state; sampling does not clear corrections accumulated in the
controller's current RSI context or start a new KRL program. Start the new KRL run after Sampling succeeds.

Offsets are written with six fixed decimal places. Initialize reconstruction from the
six-decimal exported LeadStart XYZABC. Subtract the preceding reconstructed position from
the desired position. Multiply the desired rotation by the preceding reconstructed rotation's
transpose, then extract Z-Y-X ABC from that relative rotation. Round all six offsets,
add XYZ to the preceding position and compose the rounded offset rotation before the
preceding rotation. Reject a nonfinite result or a TCP residual above 0.0001 mm / 0.000001
rotation entries before appending the offset. Use the composed pose for the next offset.

Run the existing checked TCP IK/FK on the rounded initial pose and every reconstructed
pose. Check joint positions, rated joint speeds and XYZ acceleration over 0.004 s,
including the transition to zero velocity. Retain the composed pose for the next
offset and the checked joint configuration for the next IK. At the endpoint require
the saved LeadEnd Status and nearest 360-degree joint representatives (each angle
difference below 180 degrees); TCP accuracy uses the residual checks above.
Turn encodes angle signs, so it is not compared as a revolution count.

The summary's mm value is the maximum absolute X, Y or Z displacement from LeadStart.
Its degree value is the maximum shortest rotation angle from LeadStart, in [0, 180].
It is an orientation diagnostic; it neither counts full revolutions nor specifies MaxRotAngle.

Diagnostics are automatic and use `applicationDirPath()/records` beside the executable.
Sampling creates one UTC timestamp identifier, carried with its correction array and recorded
as `# plan_id` in both logs. After successful Sampling, `rsi_plan_<id>.csv` contains the time-zero TCP pose
and joints, each subsequent analytical target, A1..A6 checked for the rounded-offset
reconstructed TCP pose, and rounded offset.
Target ABC uses the principal Z-Y-X representation and may wrap; compare rotation matrices.
Metadata includes staging joints, rated joint speeds, tool XYZABC, phase end times and the
number of zero settling cycles. Row zero has a zero increment; stream increment index 0
corresponds to the next row, normally at 0.004 s. The final target time can be shorter than
its nominal cycle boundary. Sampling writes on its existing numerical worker.

During UDP exchange, `rsi_packets_<id>.csv` uses the active array's same identifier.
Further captures of that array use `_part2`,
`_part3`, etc. to preserve earlier files. Captures without an array identifier are named
`rsi_packets_unassigned_<UTC timestamp>.csv` with the same part convention. Each capture records every request accepted by
the sender-address check, including malformed XML and queued requests not answered, and
each attempted reply. Rows contain monotonic PC nanoseconds since device start, `rx`/`tx`,
the stream index before processing the request (`-1` for received rows), writeDatagram's
byte result (`-1` on failure), the send-call duration in nanoseconds and exact XML bytes
encoded as Base64. Decode the XML column to obtain RIst, AIPos, MACur, Start, Result,
IPOC and RKorr without imposing additional receive-field requirements. A successful local
write is not proof the controller received or applied the correction.
Receive timestamps measure application dequeue time, not network-interface arrival time;
send duration measures the local write call, not a network round trip.

Capture buffers at most 64 MiB of rows; a `# dropped_rows` footer reports omitted rows.
After 250 ms without accepted-peer traffic, or on disconnect/shutdown, a copied-input
worker saves the capture; replacing the active array also flushes its old-ID rows. Received
rows are timestamped before decoding and appended with their reply after sending, keeping the
exchange together without including capture insertion in reply latency. Independent files can finish saving concurrently. There is
no file I/O in the receive callback or inactivity slot.
Only shutdown waits for outstanding saves. Console messages report saved paths or errors;
no user-operated recording control or protocol change is required.

Example request and reply (values illustrative):

    <Rob TYPE="KUKA"> ... <Start>1</Start><Result>0</Result><IPOC>123456</IPOC></Rob>
    <Sen Type="ImFree"><RKorr X="0.020000" Y="-0.000312" Z="0.001000" A="0.000000"
      B="0.000000" C="0.012000" /><Stop>0</Stop><Result>0</Result><IPOC>123456</IPOC></Sen>

## shared_data.txt

RSI Sampling writes `C:\RoboCrapJobs\shared_data.txt`. Before file access, `REMOUNT_SHARE()`
closes all KRL files from both the robot and submit interpreters (`krl_fclose_all`, `-99`),
removes `/RoboCrapJobs` and reconnects it to `//172.31.1.47/RoboCrapJobs` using the
configured share credentials. Unmount error `-2` permits a fresh mount attempt; other
cleanup errors abort. The helper returns TRUE only when the final mount reports
`#DATA_OK`; only then does `READ_JOB()` open `/RoboCrapJobs/shared_data.txt`.
The application writes version 2: eight CRLF rows, `;` separators, six-decimal
floats and decimal integer S/T. XYZ is mm; ABC is principal Z-Y-X in degrees.

1. `2`: format version.
2. PsIn: staging XYZ and LeadStart ABC, then its own S/T.
3. LeadStart: initial RSI XYZABC, then its own S/T.
4. LeadEnd: composed final TCP XYZABC, then its own S/T.
5. PsOut: its own S/T only; KRL copies XYZ from PsIn and ABC from LeadEnd.
6. ToolFrame: saved flange-to-TCP XYZABC, without S/T.
7. `$VEL.CP` in m/s: Generate transfer speed (mm/s) / 1000.
8. `$ACC.CP` in m/s²: Generate transfer acceleration (mm/s²) / 1000.

Generate accepts only transfer speeds in (0, 3000] mm/s and accelerations in
(0, `cartesianAcceleration`] mm/s², that is at most `$VEL_MA.CP` = 3 m/s and `$ACC_MA.CP`
= 10 m/s² from the archived `$machine.dat`. The controller additionally rejects an
assignment above its live `$VEL_MA`/`$ACC_MA` ("Value assignment inadmissible") and stops.

S/T comes from saved animation joints at boundaries 1, 2, 5 and 6 respectively,
using the existing configuration classifier and one local FK per boundary. Turn
sets bits 0-5 for negative A1-A6. The writer checks all four exported/constructed
TCP poses against their saved boundary TCPs with 0.0001 mm / 0.000001 rotation-entry
tolerances before opening QSaveFile. A failure leaves corrections uninstalled.

`READ_JOB()` (local to the SRC) requires version 2 and accumulates one success flag:
READ_POS and READ_FRAME combine it with their own result (`Ok=Ok AND ...`).
READ_POS reads six REAL and two INT components into a POS initialized with S/T = -1;
#DATA_OK and S in 0-7 / T in 0-63 are required, so a truncated row fails. READ_FRAME
reads ToolFrame's six components. PsOut starts as a copy of PsIn, takes A/B/C from
LeadEnd and then reads and checks its own S/T pair. Both CP rows require #DATA_OK.
All four motion targets are POS in DAT. The file is closed before READ_JOB returns,
including after a wrong version; a failed `krl_fclose` also fails the job, and the next
run's REMOUNT_SHARE closes any file left open. Every input failure returns after HOME and
before `RSI_CREATE`. `$VEL.CP`/`$ACC.CP` are assigned before PTP PsIn, so a controller
limit violation also stops before PsIn. A matching app export, SRC/DAT and FILE_UTILS
helpers are required together. S/T selects the configuration for PTP; LIN ignores these
fields. No live configuration comparison before RSI_ON is required.

The PC holds the stream only while the sampled path exists and is compatible.
Whenever it holds no stream, it deletes the file: at application start, at a new
sampling request, on invalidation and at shutdown. KRL then stops after HOME
instead of moving to an old hole.

## Not yet verified on the controller

- ETHERNET1 `Timeout` = 10 and `Precision` = 1 are not documented in the local RSI 3.3 manual. A Precision that rounded received values would make the robot stand still.
- Whether configured POSCORR1 limits cover the intended movement; the summary's shortest-angle excursion is not a MaxRotAngle bound.
- The BOOL spelling the controller sends for Start. The PC accepts `0`/`1` and logs the first packet it cannot decode.
- Whether a direct `$TOOL` assignment updates the smartPAD active-tool display (`$ACT_TOOL`); it does not affect motion.
- Releasing the T1 Start key during RSI_MOVECORR is not supported. Cancel the program and restart from HOME.
- Whether `krl_fscanf` reports a partially converted FRAME or REAL row as `#DATA_OK`
  (`STATE_T.HITS` would show the converted count), and that `$VEL.CP`/`$ACC.CP` keep their
  assigned values through the RSI calls up to LIN PsOut. Compilation of the restructured SRC
  and helpers is also unverified.
