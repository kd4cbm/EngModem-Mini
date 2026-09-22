# EngModem Mini Rev5 - hardware qualification report

**Status: one Rev5 board was built, brought up and functionally qualified,
19-22 September 2026.** Sections 1-3 record the first pass (firmware v1 to v4,
`firmware-v4-rev1`); firmware-v6b-rev1 re-qualification is in
[section 5](#5-re-qualification-on-firmware-v6b-rev1); the firmware in this
repository, `firmware-v7-rev1`, is in [section 6](#6-re-qualification-on-firmware-v7-rev1).
Everything here was run on real hardware. This is a *functional bench qualification of a single
unit*, not a compliance or environmental test - see
[What was not tested](#what-was-not-tested) before relying on the board for
anything beyond that.

Related documents: [`BRING_UP.md`](BRING_UP.md) (how to build one),
[`ERRATA.md`](ERRATA.md) (known issues), [`J12_CABLE_GUIDE.md`](J12_CABLE_GUIDE.md),
[`../firmware/`](../firmware/) (firmware, binaries and the test scripts).

## 1. Unit and setup

| Item | Detail |
|---|---|
| Design tested | Rev5. The PCB, schematic and Gerbers in this repo are byte-identical to the project owner's local Rev5 files that the board was ordered from. |
| Assembly | SMD parts as per the JLCPCB PCBA files in `manufacturing/`; **U3 and U5 and the through-hole parts hand-soldered.** U3 = onsemi MC7805CD2TR4G, U5 = TI LM1086IS-3.3/NOPB. |
| Module | ESP32-S3-WROOM-1U, identified over USB as ESP32-S3 (QFN56), 8 MB PSRAM, 16 MB flash. |
| Jumpers | J4 = 2-3, J5 = 1-2, J8 = 1-2 (MegaBaud), J9 (BOOT) off. |
| Firmware | Zimodem 4.0.3 fork, sdk v5.5.5, ESP32 Arduino core 3.3.11; final = `firmware-v4-rev1` (SHA-256s in [`../firmware/bin/SHA256SUMS.txt`](../firmware/bin/SHA256SUMS.txt)). |
| Host | Windows 11 PC, Python 3 + pyserial; modem port = J12 wired 1:1 to a DE-9 breakout ([cable guide](J12_CABLE_GUIDE.md)); separate 3.3 V USB-serial adapter on J7 for the debug UART; USB (J6) for flashing. |
| Network | Board on a home 2.4 GHz WiFi network; PC on the same LAN subnet running a TCP echo/sink server for the connection tests. |
| Modem settings | 115200 8N1 (saved configuration). Flow control **off** except where a test says otherwise. Nothing changed by the tests was saved. |

## 2. Results

"Fw" is the firmware revision the test was passed on (the fix history is in
section 3). Counts are the automated checks in `firmware/tests/`.

### Board functions

| Function | Result | Fw | Basis |
|---|---|---|---|
| Programming over USB | Pass | v1-v4 | esptool over native USB, auto-reset, each image hash-verified after write |
| Boots, debug UART | Pass | v1-v4 | Boot log on J7 at 115200 (SD init, WiFi join, pin assignment line); nothing on USB (by design) |
| WiFi | Pass | v1-v4 | Joined the network, got an IP address |
| microSD | Pass | v1-v4 | "External SD card initialized." at boot; SD shell worked (owner-verified) |
| 24x2 VFD | Pass | v1-v4 | Working after a loose wire was corrected (owner-verified); J1 verified pin-for-pin against the VFD datasheet |
| Status LEDs | Working, but **most read inverted** (see [ERRATA E11](ERRATA.md#e11-most-front-panel-leds-read-inverted)) | v1-v6b | Owner-reported working at first; polarity was checked later. Physical left-to-right order verified with a blink test on the unit |
| Regulators U3 / U5 | Functional | - | Board runs with both fitted; **voltages were not recorded** (see below) |

### RS-232 and modem-control lines

| Test | Result | Fw |
|---|---|---|
| Serial through J12/DE-9 (AT/OK, banner) | Pass, **only with 1:1 wiring** - a DTK/Intel cable failed (see [errata E1](ERRATA.md#e1-j12-pin-order-differs-from-common-bracket-cables)) | v1-v4 |
| Baud sweep 300, 1200, 2400, 9600, 19200, 38400, 57600, 115200, 230400, 460800, 921600 (AT answers, RTS holds/releases output at each rate) | **50/50** | v3, v4 |
| Hardware RTS/CTS suite (idle levels; output held while PC RTS low, released when high; modem drops CTS when its RX buffer fills; recovers) | **14/14** | v2, v3, v4 (v1: **fail** - CTS did not respond) |
| PC-side CTS stays asserted across `AT&K3`/`AT&K0` cycles | **13/13** | v3, v4 (v2: fail) |
| DTR read path (firmware sees PC DTR drop/return, 3 cycles) | Pass | v3, v4 (v2: fail) |
| Hang up on DTR drop (`ATS63=2`), twice, plus `ATS63=0` control | **18/18** | v4 |
| DCD: asserted on connect, released on hang-up (PC line on v3 and v4; the firmware's own signal log was also checked, on v3) | Pass | v3, v4 |
| RI / incoming call: RING text, RI pulsing on the ~3 s ring cadence, `ATA` answer, DCD on, data both ways, RI clears, hang-up | **20/20** | v4 |
| Command-mode echo integrity under RTS/CTS: 12.6 KB at 9600, 157.5 KB at 115200, 378 KB at 921600 | Byte-exact, all three | v3, v4 |
| TCP connection stream (PC echo server, RTS/CTS on): 3 KB at 9600, 30 KB at 57600, **200 KB at 115200** (~10.5 KB/s), 100 KB at 921600; also 3.6 KB with flow control off | Byte-exact, all | v4 (v3: **fail** - stalls, see below) |
| `+++` escape and `ATH` from a connection | Pass | v4 |
| Keystroke round-trip via TCP echo | median 45.7 / 61.3 ms; worst ~430-460 ms (WiFi power-save) | v3, v4 |

### Modem-control signal map, as verified

PC RTS -> ESP32 GPIO17 (input) and ESP32 GPIO18 (output) -> PC CTS; PC DTR ->
GPIO4 (input); GPIO5 -> DCD, GPIO6 -> RI, GPIO7 -> DSR. The firmware reports
these levels itself (`AT&O88`, prints `DCRSTOI` bits on the debug UART) and the
PC-side line states were compared against it.

## 3. Defects found during qualification (all firmware; no board change needed)

Full detail and code references are in [`../firmware/CHANGES.md`](../firmware/CHANGES.md).

| # | Defect | Effect | Fixed in |
|---|---|---|---|
| 1 | RTS/CTS pin roles were backwards for this PCB's DCE wiring | Hardware flow control (`AT&K3`) could not work | v2 |
| 2 | A saved (`AT&W`) config from older firmware restored the wrong RTS/CTS pins and survives reflashing | Stale config silently re-broke flow control | v2 |
| 3 | DTR input (GPIO4) never configured on this board | PC DTR invisible to the firmware; `ATS63=2` hang-up-on-DTR impossible | v3 |
| 4 | RTS pin left deasserted after switching flow control off | PC's CTS line stuck low after `AT&K3` -> `AT&K0` | v3 |
| 5 | Serial-to-TCP data sent one byte per TCP segment | Bursts (3.6 KB at 115200) delivered ~500 B, then one byte per 10 s; only < ~300 B/s was clean. Inherited from upstream Zimodem, not a hardware issue | v4 |

Found later (details in section 5 and the errata):

| # | Defect | Effect | Fixed in |
|---|---|---|---|
| 6 | VFD init sent once and fragile across ESP32 resets | Display scrambled or blank after about half of warm resets ([E8](ERRATA.md#e8-vfd-could-come-up-scrambled-after-a-reset-fixed-in-firmware-v5)) | v5 |
| 7 | AA / HS / OH LEDs driven active-low, but wired to light when high | Those three LEDs inverted | v5 (firmware) |
| 8 | MR / TR / SD / RD / CD read inverted through U4 (non-inverting buffer, active-low signals) | Five LEDs inverted | **not fixed** - planned hardware change ([E11](ERRATA.md#e11-most-front-panel-leds-read-inverted)) |
| 9 | Modem UART receive buffer never applied (set after `begin()`), so 256 bytes | Flow-off two-way traffic lost data from about 8 KB ([E12](ERRATA.md#e12-serial-receive-buffer-was-256-bytes-not-4096-fixed-in-firmware-v6b)) | v6b |
| 10 | On a fresh boot, flow control off (default) and PC RTS low, the modem never answers until `AT&K3` is issued once | Looks like a dead board on a fresh boot if the terminal doesn't assert RTS by default | **not fixed** - six firmware-level fix attempts tested and disproven; likely below what firmware can reach ([E13](ERRATA.md#e13-on-a-fresh-boot-a-low-pc-rts-line-stops-the-modem-answering-at-all-open-issue)) |

Defect 1 also exposed a **gap in the earlier design verification** (the "18/18
GPIO cross-check" compared GPIO numbers and net names, not signal direction) -
see [ERRATA E3](ERRATA.md#e3-earlier-pin-cross-check-missed-signal-direction).

## What was not tested

Be aware of these before treating the board as a finished product:

- **No instrument measurements.** RS-232 output levels at the DE-9, the 5 V and
  3.3 V rail voltages, ripple, and the regulators' temperature under load were
  **not measured or recorded**. Everything was judged through the PC's UART
  status lines and the firmware's own pin readings. Measure the rails when you
  first power a new board.
- **Supply input range** at J3 (only that a working supply was used).
- **MBAUD low (J8 2-3, 250 kbps mode)** and other jumper combinations. The
  board was qualified only in the J4 2-3 / J5 1-2 / J8 1-2 configuration.
- **Long-duration soak.** The longest single transfer was 200 KB (about 19 s)
  and the longest bulk run about 83 s. No multi-hour or thermal soak.
- **ESD/EMC, environmental, and mains/power-fault behaviour.**
- **More than one unit**, more than one host PC/USB-serial adapter, and real
  terminal programs (the tests drive the port from Python). Real-world use
  with a retro computer was not part of this report.
- **Firmware features other than those listed**: SSH/FTP/IRC/printing, XON/XOFF
  flow control, PPP/SLIP, the factory-reset button behaviour.
- **The enclosure and panel jigs** (see `enclosure/`, which keeps its own
  disclaimer).

## 4. Reproducing the tests

The scripts, their bench-specific settings, and what each one covers are in
[`../firmware/tests/README.md`](../firmware/tests/README.md). They need Python 3
and pyserial, a serial adapter on J12 (modem port), and for the signal-log
and TCP tests a debug-UART adapter on J7 and a PC on the modem's LAN.

## 5. Re-qualification on firmware v6b-rev1

`firmware-v6b-rev1` (the firmware in [`../firmware/`](../firmware/)) adds to v4: the VFD init fix, the
AA/HS/OH LED polarity, and the receive-buffer fix (E8, E11, E12). It was flashed to the same unit and the
whole suite re-run; the modem cable was on the USB adapter (COM15) unless noted, at 115200 baud unless noted.

| Test | Result |
|---|---|
| RTS/CTS suite | **14/14** |
| Baud sweep 300 to 921600 (USB adapter) | **50/50** |
| PC-side CTS across `AT&K3`/`AT&K0` | **13/13** |
| Hang up on DTR drop | **18/18** |
| Incoming call and RI | **20/20** |
| DTR/RTS signal log | tracks correctly |
| Flow-controlled streams: 3 KB at 9600, 30 KB at 57600, 200 KB at 115200, 100 KB at 230400 / 460800 / 921600 | byte-exact; 115200 at ~11 KB/s (17.6 to 19.4 s for 200 KB), 230400-921600 at ~17.3 to 17.8 KB/s |
| Bulk command-mode integrity 9600 / 115200 / 921600 | byte-exact |
| VFD across warm resets (a temporary build restarting the ESP32 every ~9 s) | **0 scrambled in about 40 resets** (v4-style init: roughly half) |
| Flow **off**, two-way (echo) streams 3.6 KB to 40 KB | **clean in all 20 runs** (v5-rev1: lost data from 8 KB; 30 and 40 KB clean 0 of 6) |
| Flow **off**, sustained ~60 KB two-way | one of two runs clean; the other lost 2,709 bytes - a real limit, **use RTS/CTS for long transfers** |
| Flow off, one-way (PC sends, network receives) 20 KB and 100 KB | lossless |
| Front-panel LEDs | idle panel as predicted after the firmware fix: OH off, AA off, HS on; a dynamic pass (TR dark with DTR asserted, OH on during a connection, HS off at 9600 baud, AA on with a listener) looked right but was many changes at once and is not a rigorous check |

### Native serial port versus the USB adapter
The same firmware was also tested with the modem on the PC's motherboard serial port (a standard 16550 UART,
`COM1`) and on the USB adapter (an FTDI FT232R). Every functional test was identical, and throughput and
keystroke latency (median about 51 to 61 ms, WiFi dominated) were the same within run-to-run variation.
The only difference is that the native port accepts nothing above 115200 baud, so 230400 to 921600 could be
tested only through the adapter.

### Still not verified
- The proposed U4 replacement (an inverting buffer) has **not been built or tested**; the LED behaviour after
  it is expected, not measured (E11).
- The v6b test run on the native port was cut short when the cable was moved; the same tests were run in full
  on the USB adapter.
- All items in "What was not tested" above still apply (no voltage or signal-level measurements, single unit,
  no long soak, no ESD/EMC).

## 6. Re-qualification on firmware v7-rev1

`firmware-v7-rev1` adds one fix to v6b-rev1: SSH sessions now send the configured `AT&S41` term type
to the host instead of always sending the literal string `"vanilla"` (see `CHANGES.md`). Confirmed
with a local test SSH server logging the pty-req TERM value it received: before the fix, always
`vanilla`; after, matches `AT&S41` (tested with `vt100`). Telnet's own TERMTYPE negotiation was
already correct and unaffected.

The full `firmware/tests/` suite was re-run on the same unit; every check passed except one, described
below.

| Test | Result |
|---|---|
| RTS/CTS suite, baud sweep, PC-side CTS across `AT&K3`/`AT&K0`, hang up on DTR, incoming call/RI, flow-controlled streams, bulk command-mode integrity | Same results as [section 5](#5-re-qualification-on-firmware-v6b-rev1) |
| SSH pty-req TERM value matches `AT&S41` | Pass (new test this round) |
| `rtscts_verify.py` T1 (`AT&K0`, PC RTS low) on a **genuinely fresh boot**, before anything has touched `AT&K3` | **Fail** - see defect 10 / [ERRATA E13](ERRATA.md#e13-on-a-fresh-boot-a-low-pc-rts-line-stops-the-modem-answering-at-all-open-issue) |

**How the fresh-boot failure was found:** every earlier "14/14" RTS/CTS pass, back to v2, had happened
to run right after some other test in the same boot session had already touched `AT&K3` - this was the
first time a truly fresh reboot was immediately followed by that check as the very first command
sequence, so the bug's actual precondition had never been exercised before. A dedicated regression
script, `tests/qa_freshboot_rts.py`, was added to reproduce it repeatably; on `firmware-v7-rev1` it
shows (COM7 = program/reset port, COM15 = modem port, 115200 baud):

| Step | Result |
|---|---|
| Fresh boot, RTS high (control) | Pass - 0.05 s |
| Fresh boot, RTS low, `AT&K` never touched (the defect) | **Fail - no response in 6 s** |
| `AT&K3` accepted | Pass |
| `AT&K0` accepted | Pass |
| RTS low again, after one `AT&K3`/`AT&K0` cycle | Pass - 0.05 s |

A firmware-level fix was attempted (see ERRATA E13 for the six approaches tried) but none resolved
it; `firmware-v7-rev1` ships with this behaviour unchanged from v6b-rev1 and v4-rev1, now documented
rather than undiscovered.

### Still not verified (v7-rev1, in addition to section 5's list)
- Whether asserting RTS externally (e.g. a terminal or cable that raises it by default) reliably avoids
  the fresh-boot stall in practice was not tested beyond the scripted RTS-high control case above.
- A scope/meter reading on the CTS-input pin (GPIO17) at the moment of the stall, which would confirm
  or rule out the hardware-level theory in ERRATA E13, was not done.
