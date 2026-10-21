# KR C4 ↔ Windows 10 Shared Folder Setup

## Network

- PC IP: `172.31.1.47`
  Subnet mask: 255.255.0.0
- KR C4 KLI IP: `172.31.1.147`
  Subnet mask: 255.255.0.0

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
\\192.168.1.100\RoboCrapJobs
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

For the Ethernet adapter connected to the KR C4 network, set the network profile to:

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

---

## 7. Verify basic network connectivity

From the PC:

```cmd
ping 192.168.1.200
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
    "//192.168.1.100/RoboCrapJobs",
    "KrcReader",
    "YOUR_HASH_FROM_KHASH"
)
```

Path notation is different on Windows and in KUKA.

Windows:

```text
\\192.168.1.100\RoboCrapJobs
```

KUKA `krl_mount()`:

```text
//192.168.1.100/RoboCrapJobs
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
-> 192.168.1.100
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
