# EngModem Mini Rev5 - hardware qualification report

**Status: one Rev5 board was built, brought up and functionally qualified,
19-21 September 2026, with firmware `firmware-v4-rev1`.** Everything below was
run on real hardware. This is a *functional bench qualification of a single
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
| Status LEDs | Pass (functional) | v1-v4 | Owner-reported working; individual LED behaviour was not logged |
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
