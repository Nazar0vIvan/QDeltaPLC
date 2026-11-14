# RoboCrap hardware setup procedures

Operator procedures and controller settings referenced by [ARCHITECTURE.md](ARCHITECTURE.md) §1.
Addresses and wiring are listed there; the KR C4 share is set up in
[KR_C4_WINDOWS10_SHARED_FOLDER_SETUP.md](../KR_C4_WINDOWS10_SHARED_FOLDER_SETUP.md).

## Windows firewall for RSI

The KR C4 sends RSI requests to 172.31.1.47:5555. If Windows drops them, the KRL program stops at
RSI with `RSITimeout` after the share has already been mounted and read. Inbound RSI needs this
rule, valid in every profile and restricted to the RSI address pair:

| Rule | Direction / protocol | Local | Remote | Program | Profiles | Action |
|---|---|---|---|---|---|---|
| `RoboCrap RSI from KUKA` | In / UDP | 172.31.1.47, port 5555 | 172.31.1.147 | `build\Felgo_SDK_Desktop_Qt_6_8_3_MinGW_64_bit-Debug\robocrap.exe` | Domain, Private, Public | Allow |

After the 2026-10-23 network split the rule kept the old addresses and matched nothing (found
2026-10-27 after an `RSITimeout`). Correct it in an Administrator PowerShell:

```powershell
Set-NetFirewallRule -DisplayName "RoboCrap RSI from KUKA" -LocalAddress 172.31.1.47 -RemoteAddress 172.31.1.147
```

The rule is bound to the program path, so a build in another folder (Release, another PC) needs its
own rule. Windows also created program rules named `robocrap` on the first run (Allow TCP/UDP for
Private; Block TCP and a disabled Block UDP for Public). Windows applies Block rules before Allow
rules, so the Public `robocrap` UDP Block rule must stay disabled.

## Network profile of "KRC4-Direct"

Windows remembers a network's profile by the MAC address of its default gateway. Without a gateway
"KRC4-Direct" was an unidentified network ("Неопознанная сеть") and returned to Public after every
USB adapter reconnect or KR C4 restart (observed 2026-10-27). The profile matters for the
`robocrap` program rules and for the share's Private-only TCP 139 rule (on Public the built-in
NB-Session-In rule allows TCP 139). Since 2026-10-27 a default route through the KR C4 Windows
interface, which answers ARP, gives the network an identity ("Сеть"):

```powershell
# Administrator; stored in both the active and the persistent route store
New-NetRoute -InterfaceAlias "KRC4-Direct" -DestinationPrefix "0.0.0.0/0" -NextHop 192.168.2.200 -RouteMetric 9999
Disable-NetAdapter -Name "KRC4-Direct" -Confirm:$false; Enable-NetAdapter -Name "KRC4-Direct"
Set-NetConnectionProfile -InterfaceAlias "KRC4-Direct" -NetworkCategory Private   # once; Windows keeps it
```

- The KR C4 does not route; the gateway only identifies the network. Metric 9999 keeps the route
  behind Wi-Fi (metric 0) and the VPN; with the automatic metric (25) the PC would lose Internet.
- The identity is the KR C4 KLI MAC address: a replaced controller or network card is a new
  network that must be set to Private again. Windows re-identifies the network on each link-up.
- `secpol.msc` (Unidentified Networks → Private) was not used: it would make every unidentified
  network on the PC Private.

Checked 2026-10-27: "KRC4-Direct" is "Сеть", Private; 172.31.1.147 routes through it, 8.8.8.8 does
not. Persistence across a second adapter reconnect was not re-checked.

Check before a run (no administrator rights needed):

```powershell
Get-NetConnectionProfile -InterfaceAlias "KRC4-Direct"                        # Name "Сеть", Private
Get-NetRoute -DestinationPrefix "0.0.0.0/0"                                  # KRC4-Direct route metric 9999
Find-NetRoute -RemoteIPAddress 8.8.8.8 | Select-Object InterfaceAlias -First 1   # not KRC4-Direct
netsh advfirewall firewall show rule name="RoboCrap RSI from KUKA" verbose   # LocalIP / RemoteIP
Get-NetUDPEndpoint -LocalPort 5555                                           # App socket on 172.31.1.47
ping -S 172.31.1.47 172.31.1.147
```

## Power settings for RSI

A full calibration session streams RSI for about 17 min without operator input at the PC. On
2026-11-14 the first full session stopped with KSS29002 `RSITimeout` (and KSS00489 "STOP by
$CORRECTION function") 9 min 56 s after Run: no RSI packet reached RoboCrap for 171.6 ms, then 11
arrived at once, while RoboCrap's RSI thread was idle; ETHERNET1 `Timeout` = 10 allows only 10
missing replies. The trigger is not proven (no event-log entry); the stall came close to the 10 min
display-off timeout. With these settings (set by the user, read back on the PC 2026-11-14) the
repeated 16 min 41 s session ran with a largest receive gap of 10.9 ms:

| Where | Setting |
|---|---|
| Power Options > advanced settings (plugged in) | Display: Turn off display after Never; Sleep: Sleep after Never; USB settings: USB selective suspend Disabled |
| Device Manager > "ASIX USB to Gigabit Ethernet" (`KRC4-Direct`) > Power Management | "Allow the computer to turn off this device to save power" cleared |
| Same adapter > Advanced | Energy-Efficient Ethernet Disabled (the adapter's own SelectiveSuspend stayed Enabled during that session) |

Check before a long run (no administrator rights needed):

```powershell
powercfg /query SCHEME_CURRENT SUB_VIDEO VIDEOIDLE     # Current AC Power Setting Index 0x00000000 (Never)
powercfg /query SCHEME_CURRENT SUB_SLEEP STANDBYIDLE   # Current AC Power Setting Index 0x00000000 (Never)
Get-NetAdapterAdvancedProperty -Name "KRC4-Direct" -DisplayName "Energy-Efficient Ethernet"   # Disabled
```

## KR C4 Automatic External configuration

Automatic External connects the KR C4 to the Delta PLC through `$IN[1..5]` and `$OUT[1..8]` (set by
the user on the smartPAD, Configuration > Inputs/outputs > Automatic External, 2026-11-13; wiring in
[ARCHITECTURE.md](ARCHITECTURE.md) §1; design and procedure in
`plans/PLC_AUT_EXT_PROGRAM_START_EXECPLAN.md`). `$IN[1025]` is always TRUE and `$IN[1026]` always
FALSE; outputs above 16 are not mapped to the EL2809. `$OUT[2]` is wired to PLC X1.1 but unassigned.
Outputs 138 (`$PR_MODE`) and 139 (`$SS_MODE`) are taken by other machine-data signals; assigning them
again causes KSS00251 "Signal allocation conflict". `$CHCK_MOVENA` is FALSE (`$OPTION.DAT`), so
`$MOVE_ENABLE` on `$IN[1025]` does not block external start. The smartPAD screens with these values
are transcribed in `docs/plc_project_snapshot/AUT_EXT_DIOs_live.md`; a later change of the interface
updates that file and this table together.

| Inputs | Value | | Outputs | Value |
|---|---|---|---|---|
| PGNO_TYPE | 1 (binary) | | `$I_O_ACTCONF` | 1 |
| REFLECT_PROG_NR | 0 | | `$PERI_RDY` | 3 |
| PGNO_LENGTH | 2 | | `$STOPMESS` | 4 |
| PGNO_FBIT | 4 | | `$PRO_ACT` | 5 |
| PGNO_PARITY | 0 (no evaluation) | | PGNO_REQ | 6 |
| PGNO_VALID | 0 (number taken with `$EXT_START`) | | APPL_RUN | 7 |
| `$EXT_START` | 1 | | `$IN_HOME` | 8 |
| `$MOVE_ENABLE` | 1025 | | `$RC_RDY1` | 137 |
| `$CONF_MESS` | 3 | | `$ALARM_STOP` | 1013 |
| `$DRIVES_OFF` (low-active) | 1025 | | `$USER_SAF` | 1011 |
| `$DRIVES_ON` | 2 | | `$ROB_CAL` | 1001 |
| `$I_O_ACT` (interface active) | 1025 | | `$ALARM_STOP_INTERN` | 1002 |
| | | | PGNO_FBIT_REFL | 999 |
| | | | `$PRO_MOVE`, `$ROB_STOPPED` | 1022, 1023 |
| | | | `$T1`, `$T2`, `$AUT`, `$EXT` | 993, 994, 995, 996 |
| | | | `$ON_PATH`, `$NEAR_POSRET` | 997, 147 |
| | | | `$IN_HOME1` … `$IN_HOME5` | FALSE |

`R1\CELL.SRC` on the controller is `resources/files/AutExt/cell.src`: CASE 0 calls
`RSI_Probe_Calibration` (RoboCrap program 1, program number PGNO 0), CASE 1–3 are `WAIT SEC 5`
placeholders (programs 2–4). Velocities are programmed, not reduced by the override:

- CELL sets the program override `$OV_PRO` to 100 % before every program number (only reached in
  AUT EXT). In T1 the override is the operator's: no program writes it there, and the controller
  appears to keep a separate T1 value (it read 10 % in T1 after 100 % in AUT EXT, 2026-11-14), so
  set it once on the smartPAD (POV/HOV status indicator in the status bar, Overrides window).
- PTP velocity: every program with PTP motions (CELL, `RSI_Probe_Calibration`) calls
  `INIT_PTP_VEL ( )` (module `resources/files/AutExt/INIT_PTP_VEL.src/.dat`, created on the controller
  from the module template) after its INI fold and after any `BAS` call that sets the PTP velocity,
  then uses plain PTPs (an inline-form PTP overwrites `$VEL_AXIS` with its own Vel).
  `INIT_PTP_VEL` sets `$VEL_AXIS[1..6]` to 100 % in T1 (limited by the controller to 250 mm/s) and to
  `PtpVelAuto` (5 %, `INIT_PTP_VEL.dat`) in AUT EXT, AUT and T2. CELL's `PTP XHOME` uses it.
- CP velocity: `$VEL.CP` in each program (calibration: the Generate transfer feed from the job file,
  10 mm/s); the RSI search speed is the PC stream.
- A motion keeps the velocity planned by the advance run, so after an operating-mode change an
  interrupted program is restarted from CELL, not resumed: the PLC refuses RESUME
  (`ResumeBlocked`) and RoboCrap shows "Program interrupted · mode changed · restart from CELL".

Starting CELL requires its BCO run to HOME in T1 before switching to Automatic External (KSS 8.3 SI
§8.11); CELL's INIT fold also clears a stale APPL_RUN (`$OUT[7]`) during that BCO run. A new job
(Run) needs `$IN_HOME`, APPL_RUN OFF and AUT EXT. After the smartPAD STOP key the controller shows
"KSS01370 Passive STOP" and sets `$STOPMESS`; the PLC's `$CONF_MESS` pulse does not clear it, so the
operator acknowledges it on the smartPAD before Resume (RoboCrap disables Resume until then;
external acknowledgement through `KrcExtConfMsg.xml` is described, not applied, in
`docs/solutions/KUKA_PASSIVE_STOP_EXTERNAL_ACK.md`). After a released, unacknowledged EMERGENCY STOP
a Run cleared `$STOPMESS` during ST_PREP without any smartPAD input, so the PLC's `$CONF_MESS` pulse
acknowledges that message (stage A test A6, 2026-11-14).
