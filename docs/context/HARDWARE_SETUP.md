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

## KR C4 Automatic External configuration

Automatic External is switched off and none of its signals uses the wired channels `$IN`/`$OUT`
1–16 (set by the user on the smartPAD, Configuration > Inputs/outputs > Automatic External,
2026-10-29). `$IN[1025]` is always TRUE and `$IN[1026]` always FALSE; outputs above 16 are not
mapped to the EL2809. Outputs 138 (`$PR_MODE`) and 139 (`$SS_MODE`) are taken by other machine-data
signals; assigning them again causes KSS00251 "Signal allocation conflict".

| Inputs | Value | | Outputs | Value |
|---|---|---|---|---|
| PGNO_TYPE | 1 (binary) | | `$RC_RDY1` | 137 |
| PGNO_LENGTH | 2 | | `$ALARM_STOP` | 1013 |
| PGNO_FBIT | 4001 | | `$USER_SAF` | 1011 |
| PGNO_PARITY | 0 (no evaluation) | | `$PERI_RDY` | 143 |
| PGNO_VALID | 0 (edge of `$EXT_START`) | | `$ROB_CAL` | 1001 |
| `$EXT_START` | 1026 | | `$I_O_ACTCONF` | 140 |
| `$MOVE_ENABLE` | 1025 | | `$STOPMESS` | 144 |
| `$CONF_MESS` | 1026 | | `$ALARM_STOP_INTERN` | 1002 |
| `$DRIVES_OFF` (low-active) | 1025 | | `$PRO_ACT` | 141 |
| `$DRIVES_ON` | 1026 | | PGNO_REQ | 33 |
| `$I_O_ACT` (interface active) | 1026 | | APPL_RUN | 142 |
| | | | `$PRO_MOVE` | 1022 |
| | | | `$T1`, `$T2`, `$AUT`, `$EXT` | 993, 994, 995, 996 |
| | | | `$IN_HOME`, `$ON_PATH`, `$NEAR_POSRET` | 1000, 997, 147 |

The last output row comes from the 17 Oct copy `resources/files/KR C4/STEU/Mada/$machine.dat`;
the Robot position tab was not checked on the controller.
