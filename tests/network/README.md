# Offline RSI protocol regressions

`rsiprotocoltests` is gated by `ROBOCRAP_BUILD_ARCHITECTURE_TESTS=ON` (default OFF).
It compiles the production protocol and receive gate with Qt Core/Network, opens no
sockets and starts no devices. The gate used by RsiDevice::onReadyRead is exercised
with an offset-consuming callback: rejected input cannot invoke it or learn a port.
This covers production validation ordering, not the UDP event loop or tickMotion's
trajectory algorithm. Those integration checks remain pending.

The fixture is the original `resources/files/fromKRC.xml`, resolved through the
configured source directory. It has AIPos, MACur and IPOC but no RIst and is accepted.
Only IPOC is required: exactly one decimal unsigned 64-bit value (zero allowed).
RIst, AIPos and MACur, when present, must each supply six finite numbers; duplicate
recognized elements are rejected. RIst uses XYZABC; AIPos and MACur use A1–A6.
Unknown extensions remain allowed if their XML is well formed. Malformed/truncated
documents, trailing garbage and invalid/missing numeric fields are rejected.

The historical `toKRC.xml` uses RKorr A1–A6 and a padded zero IPOC. The active sender
instead uses RKorr X/Y/Z/A/B/C, C-locale general formatting with ten significant
digits, and an unpadded numeric IPOC. Tests preserve that active format, including
the existing absence of a stop field even when shouldStop is true. Incoming leading
zeros are accepted and normalized as before. No IPOC sequencing policy is added.

The configured peer address is authoritative. The first valid packet from that
address establishes a nonzero port; later packets must match both address and port.
Connect/disconnect reset the learned port. Invalid packets produce no reply and no
motion callback. Tests cover first and established peer rejection, offset retention,
port relearning, and acceptance of repeated IPOCs.

No build or runtime test was executed during step 3. After explicit authorization,
inspect/select the matching Qt/MinGW build according to AGENTS.md, then for a valid
existing hot-reload-OFF build:

```powershell
cmake -S . -B "$buildDir" -DROBOCRAP_BUILD_ARCHITECTURE_TESTS=ON
cmake --build "$buildDir" --target rsiprotocoltests robocrap --parallel
& "$buildDir/rsiprotocoltests.exe"
```

Run from the repository root with the selected Qt/MinGW runtime on PATH. Adjust
executable location for multi-configuration generators. Expect `RSI protocol
regressions passed` and exit zero. Building robocrap verifies actual device linkage;
do not launch it as an offline test because its main starts devices.

## Offline PLC regressions

`plcprotocoltests` uses the same opt-in architecture-test option. It links the real
backend parser and compiles PlcDevice, exercising its private receive/matching path
through a narrow friend test class. It creates an unconnected QTcpSocket to test a
failed write, but never connects, binds or sends to equipment. Fragmented and
coalesced frames are supplied directly to the production receive buffer.

The parser preserves the decoded device for READ_IO, WRITE_IO, READ_REG and WRITE_REG.
Pending requests retain their request maps. Replies must match transaction id and
command plus the available echoed identity: device/module for IO, D/address for
registers (also value for WRITE_REG), and var/attr for SET_VAR. Error replies contain
only command/error/code, so only id and command can be matched. SNAPSHOT and WRITE_RAW
also have no further verified request identity fields to compare. Unsolicited CHG
notifications retain their existing behavior. A mismatch leaves the original request
pending and does not publish state.

Allocation searches all 256 ids, including zero as in the original wrap behavior,
without reusing outstanding ids. Exhaustion rejects the request. Failed/short writes
remove their pending entry; a positive short write aborts the connection because the
stream would contain an incomplete request. Explicit and remote disconnect clear
pending requests and buffered bytes. No automatic retry or timeout was added: no
supported expiry policy was found. Unanswered requests can therefore exhaust the
pool until matching replies or disconnect; selecting a timeout remains a follow-up.
The wire cannot distinguish a delayed duplicate from a later identical request after
an id is legitimately reused. This step does not invent a wire generation counter.

Tests cover device identity, unchanged framing/byte swapping, wraparound/exhaustion,
failed-send cleanup, mismatches without publication, complete/fragmented replies,
duplicate replies, matched/unmatched error replies, coalesced CHG, and disconnect
cleanup. Short positive socket writes are guarded in production but not simulated.
No configure, build or binary execution has been performed for step 4.

After explicit authorization, select the matching existing build/kit per AGENTS.md:

    cmake -S . -B "$buildDir" -DROBOCRAP_BUILD_ARCHITECTURE_TESTS=ON
    cmake --build "$buildDir" --target plcprotocoltests robocrap --parallel
    & "$buildDir/plcprotocoltests.exe"

Expect PLC protocol regressions passed and exit zero. The intentionally unconnected
write may emit a Qt socket warning. Use matching Qt/MinGW DLLs on PATH and adjust
paths for multi-configuration builds. Do not launch application main for this test.
