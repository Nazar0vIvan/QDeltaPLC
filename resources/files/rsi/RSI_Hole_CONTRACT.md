# RSI_Hole nominal run: controller/PC contract

The sequence is HOME → Ps → clear lead-in start → lead-in, machining and lead-out
under RSI → Ps → HOME. Force correction is disabled.

Controller files:
- `RSI_Hole.src` and `RSI_Hole.dat`: the KRL program.
- `RSI_Hole.rsi`, `RSI_Hole.rsi.xml` and `RSI_Hole.rsi.diagram`: the RSI Visual context. Copy these from the RSI Visual export; do not hand-edit them.
- `RSI_EthernetConfig.xml`: the Ethernet configuration.

PC side:
- `src/trajectories/chamfer/chamferrsistream.cpp`: computes the corrections.
- `src/application/scene/scenemachiningpreparation.cpp`: RSI Sampling and the shared file.
- `src/devices/rsi/rsidevice.cpp`: the UDP partner.

## Frames

`$BASE=$NULLFRAME` and `$ROBROOT` is null, so BASE, WORLD and the
application's robot base coincide. `TOOL_DATA[6]` is the application's
flange-to-TCP transform, the collet frame combined with the TCP. XYZ are in mm.
ABC are Z-Y-X Euler angles in degrees. POSCORR1 uses RefCorrSys = Base. Relative
A/B/C corrections add directly to the starting TCP A/B/C angles.

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
MAP2SEN_PINT1. Only `Completed=($SEN_PINT[2]==1)` allows the return to Ps and HOME.

## PC behavior

The controller sends one `<Rob>` packet per 4 ms cycle. The PC answers it within
the same cycle, echoing its IPOC. If several packets are queued, it answers only
the newest one.

- **Start = 0:** zero corrections, Stop 0, Result 0, and the stream index returns to the first correction. A stream sampled during a run is applied now.
- **Start = 1 with a stream:** one correction per packet, Result 0, Stop 0. The stream ends with 50 zero corrections so the robot settles.
- **After the last correction:** zero corrections with Result 1, until a packet echoes Result = 1.
- **Then:** zero corrections with Result 1 and Stop 1, until Start returns to 0.
- **Start = 1 without a stream:** zero corrections with Stop 1 and Result 0. RSI_MOVECORR ends, and KRL performs no return motion.

Corrections are written with six fixed decimal places. Each one equals the
target minus the start pose minus the sum of all corrections already sent, so
rounding never accumulates.

Example request and reply (values illustrative):

    <Rob TYPE="KUKA"> ... <Start>1</Start><Result>0</Result><IPOC>123456</IPOC></Rob>
    <Sen Type="ImFree"><RKorr X="0.020000" Y="-0.000312" Z="0.001000" A="0.000000"
      B="0.000000" C="0.012000" /><Stop>0</Stop><Result>0</Result><IPOC>123456</IPOC></Sen>

## shared_data.txt

RSI Sampling writes `C:\RoboCrapJobs\shared_data.txt`, read by KRL as
`/RoboCrapJobs/shared_data.txt`. The file has five CRLF rows with six decimal
places and `;` separators:

1. Ps: staging point XYZ, with the lead-in start ABC.
2. LeadStart: lead-in start XYZABC, the RSI starting pose.
3. LeadEnd: the start pose plus the sum of all corrections, keeping the terminal Euler branch.
4. `$VEL.CP` in m/s: the Generate transfer speed (mm/s) / 1000.
5. `$ACC.CP` in m/s²: the Generate transfer acceleration (mm/s²) / 1000.

The PC holds the stream only while the sampled path exists and is compatible.
Whenever it holds no stream, it deletes the file: at application start, at a new
sampling request, on invalidation and at shutdown. KRL then stops after HOME
instead of moving to an old hole.

## Not yet verified on the controller

- ETHERNET1 `Timeout` = 10 and `Precision` = 1 are not documented in the local RSI 3.3 manual. A Precision that rounded received values would make the robot stand still.
- POSCORR1 MaxRotAngle must cover the cumulative A/B/C range shown by RSI Sampling. 360 may be too small for a full turn plus leads.
- The BOOL spelling the controller sends for Start. The PC accepts `0`/`1` and logs the first packet it cannot decode.
- Releasing the T1 Start key during RSI_MOVECORR is not supported. Cancel the program and restart from HOME.
