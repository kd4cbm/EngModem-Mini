# Changes from upstream Zimodem

This is a fork of Bo Zimmerman's [Zimodem](http://www.zimmers.net) (see `NOTICE`),
targeting the ESP32-S3-16R8 DevKitC-1 specifically, with a 24x2 character VFD status
display added. This document describes every functional change from upstream, based
on a direct diff against the upstream source tree.

This particular copy (`ENGMODEM_MINI_BOARD`) is further forked from the base
`Zimodem-VFD` to target the EngModem Mini custom PCB - see "EngModem Mini board
support" below for what's different from the base fork.

## EngModem Mini board support

Added after a systematic pin-by-pin cross-check between this firmware's
`#define`s and the actual EngModem Mini schematic/PCB (every modem-control,
SD, and VFD signal matched exactly - 18 for 18; these two gaps were the only
mismatches found - but see the v2 RTS/CTS entry below: that audit compared GPIO
numbers and net names, not signal direction, and missed a third problem). New `ENGMODEM_MINI_BOARD` flag in `zimodem.ino` gates both
fixes so the base `ARDUINO_ESP32S3_DEV` dev-board behavior is unchanged unless
this flag is defined.

- **GPIO38 boot-time write was firing on hardware that doesn't have the pin it
  was written for.** `setup()` unconditionally called `neopixelWrite(38, 0, 0,
  0)` on any `ARDUINO_ESP32S3_DEV` build, to clear the DevKitC-1 dev board's
  onboard addressable RGB LED. EngModem Mini has no such LED - that pin is
  wired out to the expansion header instead as a general-purpose GPIO - so the
  dev board's board-specific cleanup write was landing on Mini's expansion
  pin as a spurious WS2812 protocol pulse train on every boot. Now skipped
  entirely when `ENGMODEM_MINI_BOARD` is defined.

- **`SUPPORT_LED_PINS`'s 3 status-LED pins didn't match Mini's actual
  wiring, and the feature was disabled by default besides.** The existing
  AA/HS/WIFI pin block hardcoded GPIO35/34/26 - not just wrong for this
  board, but GPIO26 isn't even usable on this module (it's in the
  reserved flash/octal-PSRAM range per the Hard Constraints table in
  `PIN_MAP.md`). EngModem Mini's real wiring for its 3 direct-drive status
  LEDs is GPIO10/AA, GPIO11/HS, GPIO12/OH. Enabled `SUPPORT_LED_PINS` by
  default for this fork and added an `ENGMODEM_MINI_BOARD`-gated branch with
  the correct pins.

- **Board's third LED (silkscreened "OH") now shows genuine off-hook status,
  not WiFi association.** Previously reused `DEFAULT_PIN_WIFI`'s existing
  drive logic as-is (lit when `WiFi.status() == WL_CONNECTED`) - correct pin,
  wrong signal. Confirmed via a schematic trace that OH (GPIO12) and DCD
  (GPIO5) are separate, dedicated signals, not hardware-tied together, so
  this needed a real firmware fix rather than reusing DCD's existing pin
  write. Now driven from `checkOpenConnections()`, gated behind
  `ENGMODEM_MINI_BOARD`, based on `WiFiClientNode::getNumOpenWiFiConnections()`
  - the same protocol-agnostic connection count that already drives DCD.
  Every connection type in this codebase (telnet/`ATD`, SSH, FTP data
  channels, IRC, print) registers as a plain `WiFiClientNode` with no
  subclassing, so this one hook genuinely covers all of them: lit whenever
  at least one connection of any kind is open, off when none are. The two
  prior `DEFAULT_PIN_WIFI` writes driven by `WiFi.status()` (on Wi-Fi
  connect, and at boot) are now skipped for `ENGMODEM_MINI_BOARD` builds so
  they no longer fight the connection-status writes.

- **Hardware note (not a firmware change): U3/U5 regulator substitutions.**
  Not a code change - flagged here since anyone building this board from
  the firmware repo's docs should know about it. JLCPCB's PCBA placement
  preview surfaced a footprint mismatch on both of the board's D2PAK/
  TO-263-3 linear regulators: the originally spec'd parts (HGSEMI
  LM7805S2/TR for the 5V rail, ST LD1086D2T33TR for the 3.3V rail) don't
  reliably match the board's 3-lead footprint (confirmed against LCSC's
  own listings, and for the ST part, against ST's own datasheet, which
  defines two different D²PAK mechanical variants under one ambiguous
  order code). Verified, datasheet-confirmed replacements: onsemi
  **MC7805CD2TR4G** (5V) and TI **LM1086CSX-3.3/NOPB** (3.3V), both
  pin/tab-matched to the existing footprint. (The CSX variant later went out of
  stock; the board that was qualified uses TI **LM1086IS-3.3/NOPB**, the same
  DDPAK/TO-263 package and pinout in the industrial temperature grade.) Full detail in the hardware
  repo's `manufacturing/BOM_full.csv` and README.

- **Fixed: RTS/CTS pin roles were backwards for this PCB (v2).** The dev-board
  pin block assigns CTS=GPIO18 (input) and RTS=GPIO17 (output). On the
  EngModem Mini the MAX3237 is wired DCE-style: GPIO17 is fed by the
  receiver output ROUT2 (the PC's RTS from DB9 pin 7) and GPIO18 drives the
  transmitter input DIN3 (out to the PC's CTS on DB9 pin 8). With the
  dev-board assignment the firmware drove GPIO17 against ROUT2 whenever the
  PC's RTS was not asserted, and read GPIO18 as CTS while nothing drove it
  (the MAX3237's driver inputs have no pull-ups), so hardware RTS/CTS flow
  control (`AT&K3`) could not have worked. Earlier pin-map audits matched
  GPIO *numbers and net names* against the schematic, which is why this was
  not caught - signal *direction* was not compared. Under
  `ENGMODEM_MINI_BOARD` the assignments are now **CTS=GPIO17 (input)** and
  **RTS=GPIO18 (output)**.
- **Saved config can no longer move the RTS/CTS pins on this board (v2).**
  The config file written by `AT&W` stores the CTS/RTS pin numbers and
  re-applies them after the compiled-in defaults, and it survives a reflash.
  A config saved by v1 firmware would therefore silently restore the old,
  wrong assignment. On `ENGMODEM_MINI_BOARD` the saved CTS/RTS pin numbers
  are ignored (pin directions are still re-asserted). The `AT` commands that
  change pins at runtime are unaffected.
- **Boot diagnostic (v2).** The debug UART now prints
  `Flow control pins: CTS(in)=17 RTS(out)=18` after the saved config is
  applied, to confirm the effective assignment on hardware.
- **DTR input pin (GPIO4) is now configured (v3).** The ESP32-S3 `pinSupport[]`
  table (dev-board list: 1, 5-21, 36-38, 47, 48) omits GPIO4, and
  `pinMode(pinDTR, INPUT)` is only called for supported pins. On EngModem Mini
  DTR (MAX3237 ROUT3) lands on GPIO4, so the pin was never set up and DTR
  changes from the PC were not seen by the firmware. Found during QA: the
  `AT&O88` signal log showed no T-bit change while the PC toggled DTR. The
  firmware acts on DTR only when hang-up-on-DTR is enabled (`ATS63=2`); verified
  on hardware that dropping PC DTR then ends the call (`NO CARRIER`, DCD released)
  and that with `ATS63=0` (default) it does not. Before this fix `ATS63=2` could
  not have worked on this board.
- **RTS pin no longer left deasserted after leaving RTS/CTS mode (v3).** After
  `AT&K3` then `AT&K0` the UART peripheral kept owning the RTS pin and left it
  in the deasserted state, which on this board is the PC's CTS line - a PC that
  honours CTS would stall. Turning hardware flow control off now returns the pin
  to a plain GPIO output and asserts it, the same state as at boot.
- **Serial->TCP bytes are coalesced instead of sent one per segment (v4).**
  In stream (connected) mode every byte received from the PC was written to the
  socket on its own, and with `DEFAULT_NO_DELAY` each one became a separate TCP
  segment. Found in QA against a PC-hosted TCP server: a full-speed burst
  (3600 B at 115200) delivered only ~500 B, a sustained ~5 KB/s stream ~1000 B,
  after which the modem consumed roughly one serial byte every 10.0 s
  (the core's socket-write retry timeout) and dropped the rest; only streams
  slower than ~300 B/s were clean. This is inherited upstream behaviour, not
  something the EngModem Mini hardware causes. `ZStream::serialIncoming` now
  queues bytes (`txQueue`) and sends them as one write when 250 bytes are
  queued, when serial has been idle for 2 ms, or when the oldest queued byte is
  10 ms old (`txFlush`/`txFlushIfDue`). Added latency is at most 10 ms; byte
  order is preserved (the queue is flushed before any escape-sequence bytes
  and before leaving stream mode). Gated on `ENGMODEM_MINI_BOARD`; other
  builds keep the original per-byte path.
- **AA / HS / OH LEDs were inverted on this board (v5).** On the EngModem Mini these three
  LEDs are wired GPIO -> 330R -> LED anode, cathode -> GND, so they light when the pin is driven
  HIGH. The firmware's dev-board constants are active-LOW, so on the built board AA and OH were lit
  at idle and dark when "active", and HS was dark at 115200. Found when the owner compared the
  panel with the design: at idle the panel showed MR off, TR on, SD on, RD on, OH on, CD on, AA on,
  HS off. Under `ENGMODEM_MINI_BOARD` the AA/HS/OH active levels are now HIGH. All writes to these
  pins go through the `DEFAULT_*_ACTIVE/INACTIVE` constants, so every code path flips together.
  The other five LEDs (MR TR SD RD CD) are driven through U4, a non-inverting 74HCT245, from
  active-LOW modem signals; their inversion is a hardware matter (planned fix: replace U4 with an
  inverting 74HCT640, pin-compatible) and is not changed by firmware.
- **VFD initialisation made robust against warm resets (v5).** On the real board the display came
  up scrambled after ESP32 resets (the VFD stays powered through them): in a rotation test the v4
  init scrambled roughly half of warm resets, while a resync-first init, a resync init re-sent once
  a second, and the old init with 50 us instruction waits were each clean (15 boots each). Three
  changes under `ENGMODEM_MINI_BOARD`, all in `vfd.ino`:
  - `vfdResyncInit()`: three 8-bit function-set nibbles (0x3) then 0x2, then the usual
    instructions. Works from any controller state; used at boot.
  - A 50 us wait after every instruction (was 1 us). This also addresses an occasionally
    dropped first character on a row seen right after a Set-DDRAM-Address instruction.
  - During the 7 s splash window `vfdResyncInit()` is re-run about once a second, so a display
    that came up wrong repairs itself instead of staying scrambled until the next reset.
  The theory that the old single-nibble init misaligned an already-4-bit controller was tested
  and did NOT reproduce (repeating the old init was harmless), so the exact mechanism is still
  unproven; the fix is justified by the measured before/after, not by a proven cause.
  A test-only macro `ENGMODEM_VFD_RESET_TEST` (never defined in release builds) restarts the ESP32
  every ~8.5 s so warm resets can be watched in a row.
- **Modem UART receive buffer was stuck at 256 bytes; now 4096 (v6b).** `setup()` called
  `HWSerial.setRxBufferSize(RX_BUFFER_SIZE)` AFTER `HWSerial.begin()`. The Arduino-ESP32 core rejects that
  once the port is running ("RX Buffer can't be resized when Serial is already running"), so the UART has
  always used the 256-byte default. With flow control OFF and traffic in both directions at full 115200
  line rate, the receive path overflowed and lost data in whole 129-byte chunks (UART FIFO overflow
  resets): on the built board, echo streams lost data from 8 KB up, in most runs from 20 KB up (30 KB: 0 of 3 clean,
  40 KB: 0 of 3 clean; 20 KB: about 13% lost in the worst run). RTS/CTS ON was always lossless, and one-way
  transfers (PC sending, network sinking) were lossless up to 100 KB even before this change. Under
  `ENGMODEM_MINI_BOARD` the size is now set before `begin()`.
- **Serial->socket loop handles at most 250 bytes per pass (v6b).** Enlarging the buffer alone (an interim
  build called v6) let `ZStream::serialIncoming()` drain up to 4096 bytes in one pass, starving the
  socket->serial direction: the flow-controlled 200 KB duplex stream slowed from ~18 s to 24-37 s and one
  run returned 199,999 of 200,000 bytes. `serialIncoming()` now handles at most `ZSTREAM_RX_PASS_MAX`
  (250, one TX-buffer's worth) bytes per pass, so both directions are serviced alternately. Measured on
  the built board (115200, COM1 and COM15): flow-controlled 200 KB duplex 17.6-19.4 s, byte-exact 5 of 5
  (v5-rev1: 18.1-18.9 s); flow-off echo 3.6-40 KB clean in all 20 runs (v5-rev1 lost data from 8 KB up);
  one-way 20 KB and 100 KB lossless; 100 KB flow-controlled streams at 230400/460800/921600 byte-exact at
  ~17.3-17.8 KB/s (unchanged). Flow-off, full-duplex, sustained streams of ~60 KB can still lose data
  (2 runs: one clean, one lost 2,709 bytes): the modem forwards slightly slower than a full-speed sender in
  both directions at once, so no buffer size makes that lossless - use RTS/CTS for long transfers.
  A "1 byte short on the echo return" also appears occasionally in flow-off echo runs; it was present in
  v5-rev1 baselines too and its cause is not known.

## New: 24x2 character VFD status display

Entirely new files, `vfd.h` / `vfd.ino`. Drives a Noritake CU24025ECPB-W1J (24x2)
VFD directly over its 4-bit parallel bus - no I2C bridge, no second microcontroller.
See `PIN_MAP.md` for wiring.

- **Splash screen** on boot: "Zimodem", firmware version. Re-sent periodically for a
  short window after boot, since a VFD controller that isn't fully awake yet can lose
  a one-shot write.
- **Top row**: WiFi status ("No WiFi"), or IP address + "Ready" when idle, or the
  current/destination host address while connected - alternates between pages,
  scrolling if a hostname is too long to fit.
- **Bottom row**: `TX`/`RX` activity indicators (custom arrow glyphs, held briefly
  after each byte so brief activity is visible), an `Sd` indicator (2 columns,
  reserved whether or not shown) when the SD shell is available, the current baud
  rate (abbreviated above 9600, e.g. "115.2k"), and the current flow control mode
  ("RTS/CTS", "XON/XOFF", or "NONE"), right-aligned.
- **Brightness control**: `AT$VFDB=<0-4>` - 0 blanks the display (and powers down the
  VFD's internal converter, a real power saving, not just a dark screen), 1-4 select
  25/50/75/100% brightness. The hardware only has 4 real brightness levels, so this
  maps 1:1 rather than approximating a finer scale.

The flow control field required a genuine architectural fix, not just wiring up a
read: `ZSerial::flowControlType` was a plain per-instance member, but each mode
(`ZCommand`, `ZStream`, `ZConfigMode`, etc.) keeps its own separate `ZSerial`
instance. An AT command like `ATF0`/`AT&K3`, issued in command mode, updated
`ZCommand`'s own copy - but the display read `ZStream`'s copy (`streamMode`), which
is only otherwise touched on connect, from that connection's own remembered
preference. The physical UART itself was always being reconfigured correctly
(`uart_set_hw_flow_ctrl()` doesn't care which `ZSerial` instance calls it) - only the
display's copy of the setting was stale. Fixed by making `flowControlType` a `static`
member of `ZSerial`, shared across every instance, matching how `baudRate` already
works as a single global (`serout.h`, `serout.ino`).

The VFD status line only ever distinguished "RTS/CTS" from "NONE" at first - a
missing "XON/XOFF" label meant that mode silently displayed as "NONE" too. Added.

## Fixed: hardware RTS/CTS flow control silently hung TX

Two independent bugs in `zimodem.ino`, both required to reproduce: enabling hardware
RTS/CTS flow control (`AT&K3`/`ATF0`) caused all further serial output to be silently
and permanently withheld - the command's own echo transmitted, but its `OK` never
arrived, and every command after it was silent too, with no software recovery.

1. **`pinSupport[]` never marked the CTS pin as supported.** The setup loop that
   marks which GPIOs are registered had a gap between two disjoint ranges
   (`5..17` and `19..21`) that happened to exclude GPIO18 - this board's default CTS
   pin - for its entire life. `ZSerial::setFlowControlType()` gates the actual
   `uart_set_pin()` CTS registration behind this check, so the CTS pin was never
   wired into the UART peripheral at the driver level, even though hardware flow
   control was still unconditionally enabled with no valid CTS input. Fixed with a
   single contiguous range.

2. **Unclamped `dequeSize` made the TX throttle a silent no-op at common baud
   rates.** `dequeSize` controls how many bytes `serialOutDeque()` releases from the
   software output buffer into the hardware TX ring per pass. The formula
   (`1+(baud/380)`) isn't clamped against the hardware ring's actual size
   (`SER_BUFSIZE`, 127 usable bytes) - at 115200 baud it evaluates to 304, larger
   than the buffer could ever report as "used," making the throttle condition
   permanently false. Once bug #1 was fixed and flow control genuinely started
   applying backpressure, this became actively harmful rather than a no-op:
   `serialOutDeque()` would drain the entire pending output buffer in one pass the
   moment flow control released, rather than trickling it, producing jumbled,
   misaligned multi-response output. Fixed with `calcDequeSize()`, which clamps the
   formula's result to `SER_BUFSIZE-1`.

Verified against real hardware across the full baud range Zimodem offers (300 through
921600), both directions (ESP32-to-DTE via RTS, DTE-to-ESP32 via CTS backpressure).

## Fixed: SD shell status could be wrong if the card wasn't ready yet at cold boot

`initSDShell()` runs very early in `setup()`, before anything else gives a
cold-booted SD card time to settle on the SPI bus. Added up to 5 retries, 50ms apart.

## Fixed: a connection you `+++`'d away from could interrupt command mode

`ZCommand::sendNextPacket()`'s condition for announcing buffered incoming data from a
non-foreground connection didn't exclude the connection you're actively `+++`'d away
from (`current`) - now it stays silent until you explicitly `ATO` back to it or `ATH`
it, matching how a real Hayes dialup modem behaves: it doesn't announce data from a
call you've stepped away from, it just waits quietly. `NO CARRIER` still fires
normally if that connection drops while you're away from it.

## ESP32-S3-16R8 DevKitC-1 pin map

The upstream `ARDUINO_ESP32S3_DEV` pin block existed but needed real hardware
verification and several corrections for this specific module (N16R8, octal PSRAM).
See `PIN_MAP.md` for the full current pin table and rationale. Notable corrections
from the original block:

- `DSR` moved from GPIO9 to GPIO7, closing a gap in an otherwise-contiguous run.
- SD card moved off the ESP32-S3 core's *default* SPI pins onto dedicated ones
  (CS=GPIO13, MOSI=GPIO14, MISO=GPIO1, SCK=GPIO2), requiring an explicit `SPIClass`
  instance rather than the bare `SD.begin(cs)` call upstream used.
- The onboard addressable RGB LED (GPIO38) is explicitly cleared to off at boot -
  it holds whatever it last received indefinitely, it doesn't reset itself just
  because the pin goes idle otherwise.

## Minor

- A commented-out debug line was removed from `ZSerial::isSerialCancelled()`
  (`serout.ino`) - dead code, no behavior change.
- A `Serial.begin(115200)` call was added in `setup()` as a standing diagnostic hook.
  **Correction (2026-09-21, from bring-up on the real EngModem Mini):** the earlier
  text here said native USB-CDC `Serial` is the only debug path. That is wrong for
  this code: `debugPrintf` is `DBSerial` (UART0), and nothing prints to USB-CDC.
  On the EngModem Mini, UART0 (GPIO43/44) is brought out on header **J7** (pin 3 =
  `DEBUG-TXD`, pin 4 = `DEBUG-RXD`, pins 5/6 = GND) - connect a 3.3 V USB-serial
  adapter there at 115200 to see the boot log, `AT&O88` signal log, and the
  `Flow control pins:` line.
