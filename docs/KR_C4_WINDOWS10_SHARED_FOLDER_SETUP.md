# KR C4 ↔ Windows 10 Shared Folder Setup

## Network

- PC IP: `192.168.2.100` on the USB network adapter ("KRC4-Direct") cabled to the KR C4 KLI
  Subnet mask: 255.255.255.0
- KR C4 KLI Windows interface IP: `192.168.2.200`
  Subnet mask: 255.255.255.0

(Until 2026-10-23 these were 172.31.1.47 and 172.31.1.147 with mask 255.255.0.0; 172.31.1.x is
now the RSI subnet. See `codex_plans/RSI_NETWORK_SPLIT_EXECPLAN.md`.)

The goal is to let the KR C4 read files written by the PC application through a Windows shared folder.

---

## 1. Create a dedicated folder on the PC

Create:

```text
C:\RoboCrapJobs
```

Your Qt application can write files such as:

```text
C:\RoboCrapJobs\job.txt
```

---

## 2. Create a dedicated Windows user

Create a local Windows user, for example:

```text
Username: KrcReader
Password: <some password>
```

On Windows 10:

```text
Settings
-> Accounts
-> Family & other users
-> Add someone else to this PC
-> I don't have this person's sign-in information
-> Add a user without a Microsoft account
```

Use a real password.

---

## 3. Share `C:\RoboCrapJobs`

Right-click:

```text
C:\RoboCrapJobs
-> Properties
-> Sharing
-> Advanced Sharing...
```

Enable:

```text
[x] Share this folder
```

Set:

```text
Share name: RoboCrapJobs
```

Then open:

```text
Permissions...
```

Add:

```text
KrcReader
```

Give it:

```text
Read = Allow
```

If KR C4 only reads files, `Change` and `Full Control` are not required.

The resulting Windows network path is:

```text
\\192.168.2.100\RoboCrapJobs
```

---

## 4. Set NTFS permissions

This is separate from the share permissions.

Open:

```text
Properties
-> Security
-> Edit
-> Add
```

Add:

```text
KrcReader
```

Give it at least:

```text
Read
Read & execute
List folder contents
```

Both permission layers must allow access:

```text
Share permissions
AND
NTFS Security permissions
```

---

## 5. Set the Windows Ethernet network to `Private`

For the network connection cabled to the KR C4 (`KRC4-Direct`, the USB network adapter), set the
network profile to:

```text
Private
```

Then enable:

```text
Control Panel
-> Network and Sharing Center
-> Change advanced sharing settings
-> Private
```

Turn on:

```text
File and printer sharing
```

Keep password-protected sharing enabled.

---

## 6. Check Windows Firewall

Open:

```text
Windows Defender Firewall
-> Advanced settings
-> Inbound Rules
```

Make sure rules such as:

```text
File and Printer Sharing (SMB-In)
```

are enabled for the `Private` profile.

SMB normally uses:

```text
TCP 445
```

`krl_mount` on this KR C4 connects over SMB on TCP 139 (NetBIOS session), not 445. On this PC
the built-in "File and Printer Sharing (NB-Session-In)" rules cover only the `Public` and
`Domain` profiles, so a dedicated rule allows TCP 139 from the KR C4 Windows interface. In an
Administrator PowerShell (one line):

```powershell
New-NetFirewallRule -DisplayName "KR C4 share access (TCP 139)" -Direction Inbound -Protocol TCP -LocalPort 139 -RemoteAddress 192.168.2.200 -InterfaceAlias "KRC4-Direct" -Profile Private -Action Allow
```

## 6a. Answer NetBIOS on 192.168.2.100

Before mounting, the KR C4 asks the PC for its NetBIOS name (UDP 137) at 192.168.2.100. Windows
answers NetBIOS on only one address per network adapter, and on `KRC4-Direct` it chose
172.31.1.47 (the RSI address). Marking the RSI address "skip as source" moves NetBIOS to
192.168.2.100. In an Administrator PowerShell:

```powershell
Set-NetIPAddress -InterfaceAlias "KRC4-Direct" -IPAddress 172.31.1.47 -SkipAsSource $true
```

Then disable and enable `KRC4-Direct` (`ncpa.cpl`, right-click). Check with `nbtstat -n`: under
`KRC4-Direct` it must show `Node IpAddress: [192.168.2.100]`. RSI is unaffected because the
application binds its socket to 172.31.1.47; test that address with
`ping -S 172.31.1.47 172.31.1.147`.

---

## 7. Verify basic network connectivity

From the PC:

```cmd
ping 192.168.2.200
```

You should receive replies from the KR C4.

---

## 8. Generate the KUKA password hash

Do not place the plain Windows password directly into `krl_mount()`.

On the KR C4 Windows side, use:

```text
C:\KRC\UTIL\Hash\khash.exe
```

Run:

```cmd
khash.exe YourPassword
```

Save the resulting encrypted string.

---

## 9. Mount the PC shared folder from KRL

Example:

```krl
DECL STATE_T State
DECL MODUS_T Mode

Mode = #SYNC

CWRITE(
    $FCT_CALL,
    State,
    Mode,
    "krl_mount",
    "/RoboCrap",
    "//192.168.2.100/RoboCrapJobs",
    "KrcReader",
    "YOUR_HASH_FROM_KHASH"
)
```

Path notation is different on Windows and in KUKA.

Windows:

```text
\\192.168.2.100\RoboCrapJobs
```

KUKA `krl_mount()`:

```text
//192.168.2.100/RoboCrapJobs
```

If mounting succeeds:

```text
/RoboCrap
```

becomes the KRL-side name for the PC shared folder.

For example:

```text
/RoboCrap/job.txt
```

refers to:

```text
C:\RoboCrapJobs\job.txt
```

on the PC.

---

## 10. Test file access

Create this file on the PC:

```text
C:\RoboCrapJobs\test.txt
```

with:

```text
123.456
```

Then try opening it from KRL:

```krl
DECL INT FileHandle

CWRITE(
    $FCT_CALL,
    State,
    Mode,
    "krl_fopen",
    "/RoboCrap/test.txt",
    "r",
    FileHandle
)
```

Check:

```text
FileHandle
State.RET1
State.MSG_NO
```

If `FileHandle > 0` and `State.RET1` indicates success, the network share is working.

The complete data path is then:

```text
KR C4
-> KLI
-> 192.168.2.100
-> SMB
-> RoboCrapJobs
-> test.txt
```

---

## Important: SMB1

Do not enable SMB1 initially.

Windows 10 normally uses SMB2/SMB3.

Some older KSS 8.3 installations may require SMB1 for `krl_mount()`, but SMB1 is obsolete and insecure.

If `krl_mount()` fails even though:

- routing is correct,
- credentials are correct,
- permissions are correct,
- firewall settings are correct,

then check:

```text
State.RET1
State.MSG_NO
```

before changing SMB settings.

---

## Open the share from the KR C4 Windows desktop (optional)

Windows Explorer on the KR C4 logs on as its own user (`KukaUser`), which the PC rejects. Save the
KrcReader credentials once on the KR C4 (Expert, T1, Start-up > Service > Minimize HMI, Start
menu, `cmd`):

```cmd
cmdkey /add:192.168.2.100 /user:192.168.2.100\KrcReader /pass
```

Enter the KrcReader Windows password when asked (not the `khash` string). Then
`\\192.168.2.100\RoboCrapJobs` opens in Explorer, for example to copy an updated
`RSI_EthernetConfig.xml` to `C:\KRC\ROBOTER\Config\User\Common\SensorInterface`. KrcReader has
read access only, so files can be copied from the PC to the KR C4 but not back.
