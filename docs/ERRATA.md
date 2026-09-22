# EngModem Mini Rev5 - errata and known issues

Things a builder will trip over, found while qualifying a real Rev5 board
(see [`QUALIFICATION.md`](QUALIFICATION.md)). None of them require a board
modification; the suggestions at the end are for a future revision and are
not commitments.

## Hardware and documentation

### E1. J12 pin order differs from common bracket cables

J12 uses the **AT/Everex** header order (pins 1-10 = DCD, RXD, TXD, DTR, GND,
DSR, RTS, CTS, RI, NC, so header pin *n* = DE-9 pin *n*). Many ready-made
2x5-header-to-DE-9 cables use the **DTK/Intel** order (DCD, DSR, RXD, RTS, TXD,
CTS, DTR, RI, GND). Plugging one of those in swaps RXD/TXD/GND and produces
no serial output at all. Confirmed on hardware: a DTK/Intel-wired cable gave no
response; the same header wired 1:1 to a DE-9 breakout gave the Zimodem banner.
The J12 silkscreen is correct - this is a mismatch between the design and the
cable you own.

**Workaround:** wire J12 1:1 to a DE-9, or use an adapter - see
[`J12_CABLE_GUIDE.md`](J12_CABLE_GUIDE.md).

### E2. U3 and U5 are hand-soldered, and U5 was substituted

Both linear regulators were excluded from the JLCPCB assembly (the
originally specified parts did not reliably match the 3-lead D2PAK footprint).
Qualified with:

- **U3 (5 V):** onsemi MC7805CD2TR4G
- **U5 (3.3 V):** TI **LM1086IS-3.3/NOPB** - substituted for the LM1086CSX-3.3/NOPB
  when that variant went out of stock at Mouser (2026-09-16). Per TI's own
  package addendum it is the same DDPAK/TO-263 (KTT) 3-pin package and pinout,
  in the industrial temperature grade (-40 to 125 C) instead of the commercial
  (0 to 125 C); it ships in tubes rather than reels.

`manufacturing/BOM_full.csv` now lists the LM1086IS-3.3/NOPB. The U5 tab is
wired to the *output* on this board (unlike U3, whose tab is ground). The
regulators' output voltages were not recorded during qualification - measure
them on first power.

### E3. Earlier pin cross-check missed signal direction

Earlier revisions of this README reported a firmware/hardware cross-check of
"18/18 GPIO pins matched". That audit compared GPIO **numbers and net names**
only. It did not compare signal **direction**, so it did not catch that the
firmware's default RTS/CTS roles were the reverse of this PCB's DCE wiring.
That is fixed in the firmware (v2 and later) and the claim has been removed
from the README. Treat the schematic-vs-firmware match as *names and numbers*,
verified; *directions*, verified on hardware only for the signals covered in
the qualification report.

### E4. The three MAX3237 jumpers have no pull resistors

J4 (`EN`), J5 (`SHDN`) and J8 (`MBAUD`) each connect a MAX3237 control pin to
VCC or GND **only through the jumper**. A board with them missing boots and
runs but has non-working RS-232, which looks like a firmware problem. Tested
settings: J4 2-3, J5 1-2, J8 1-2. See
[`BRING_UP.md`](BRING_UP.md#3-set-the-jumpers).

### E5. J9 (BOOT) left fitted stops the firmware running

GPIO0 low at reset selects ROM download mode. Fit J9 only to force flashing,
then remove it. (Held for more than 5 s while running it triggers the
firmware's factory reset.)

### E6. Debug output is on J7, not USB

Boot messages and the `AT&O88` signal log go to the UART0 debug port
(GPIO43/44) brought out on **J7 pins 3 (TX) and 4 (RX)**. Nothing is printed
over USB. Some earlier firmware text said otherwise; it was wrong for this
code and has been corrected in `firmware/CHANGES.md`.

### E7. Earlier README status was out of date

Earlier revisions of the README said the board had not been fabricated or
tested, and pointed at a separate `Zimodem-VFD-Mini` firmware repository that
does not exist. The README now describes the qualified status and the firmware
lives in [`../firmware/`](../firmware/).

## Firmware behaviour to know about

### E8. VFD could come up scrambled after a reset (fixed in firmware v5)

The VFD stays powered through an ESP32 reset, its optional `/Reset` pin (J1 pin 3) is not connected, and
the E and RS lines have no pull resistors. Firmware up to v4 sent the display's init once, about 0.25 s
after boot, and never re-checked it. On the tested unit that left the display **scrambled or blank after
about half of the ESP32 restarts** (a 15-boot rotation test: the old init scrambled roughly 7-8 boots;
plain software restarts scrambled about 4 in 22). Cold power-ups were always fine. It also occasionally
dropped the first character written after a cursor-position instruction (the old wait after each
instruction was 1 us).

Firmware v5 and later start with the standard resync (three 8-bit function-set nibbles, then 4-bit),
wait 50 us after every instruction, and re-run the init about once a second during the 7-second splash
window so a bad start repairs itself. Result on the tested unit: 0 scrambles in about 40 warm resets.
The exact mechanism is **not proven** - a theory that the old single-nibble init misaligns a controller
that is already in 4-bit mode was tested and did not reproduce - so the fix rests on the measured
before/after. If you build your own firmware from older sources, expect this behaviour. Suggested for a
future revision: pull-downs on E and RS, and optionally a wire to the module's `/Reset`.

### E9. Command-mode throughput ceiling

In command mode the modem processes roughly 4.6 KB/s regardless of line speed;
with RTS/CTS on, data stays intact above that rate, it is just limited. In
connected (stream) mode ~10.5 KB/s at 115200 was measured.

### E10. Saved configuration survives reflashing

Zimodem's `AT&W` settings live in SPIFFS and are not erased when you reflash
the application. On this board the RTS/CTS pin numbers in a saved config are
ignored (v2+), but other saved settings (baud rate, WiFi, phone book) persist.
To clear everything, use the firmware's factory reset: with the firmware
running, hold GPIO0 low (jumper **J9**) for more than 5 seconds. That erases the
saved configuration, phone book, listeners and WiFi settings and reformats
SPIFFS, so you will need to reconfigure baud rate and WiFi afterwards. Remove
J9 when done. (The factory-reset action itself was not exercised in
qualification.)

### E11. Most front-panel LEDs read inverted

All eight front-panel LEDs are wired anode -> resistor -> drive signal, cathode -> GND, so each lights
when its signal is **HIGH**. But DCD, DSR and DTR are asserted LOW and a serial line idles HIGH, so on
Rev5 as built:

- **OH, AA, HS** (driven directly by GPIO) were inverted in the firmware too. **Fixed in firmware v5**:
  they now light when active.
- **MR, TR, SD, RD, CD** go through U4, a non-inverting 74HCT245, from those active-low signals, so they
  read inverted **in hardware**. Firmware cannot change this.

Idle at 115200, terminal not asserting DTR, no connection - what the panel shows with firmware v5 or later
on Rev5 as built:

| MR | TR | SD | RD | OH | CD | AA | HS |
|---|---|---|---|---|---|---|---|
| off | on | on | on | off | on | off | on |

So MR is dark when the modem is ready, CD is lit when there is **no** carrier, and SD/RD are lit at idle
and go dark during data (the opposite of a classic modem panel).
The tested unit's panel was observed exactly like this before the firmware fix (OH on, AA on, HS off at
idle), which is how the problem was found. The design-stage checks verified which signal feeds each LED
but never its active level.

**Planned fix (not yet done or verified on hardware):** replace U4 with a pin-compatible **inverting**
octal bus transceiver. The TI **CD74HCT640M** (SOIC-20 wide, same package and pinout; with DIR tied high
and OE low, as on this board, B = NOT A) is the candidate, checked against its datasheet only - stock was
not checked, and it must be the **HCT** version (the HC version does not meet the 3.3 V input threshold
at 5 V). It is a single hand-soldered part swap with no other change. After it, at idle: MR on, HS on,
everything else dark, with TR lighting when a terminal asserts DTR, OH and CD on a connection, AA with a
listener, and SD/RD flashing with data. Until someone has done and verified the swap, treat this as a
proposal. The tested unit's LED **order** (below) was verified physically.

**Check LED order at assembly.** Left to right the panel should read MR, TR, SD, RD, OH, CD, AA, HS. On
the tested unit the LEDs at positions 3 and 4 had been fitted in swapped positions and had to be
corrected. **This is not a board design defect** - checked directly against the Rev5 KiCad PCB and
schematic, the eight LED footprints sit at a uniform 5.5 mm pitch in exact D8-to-D1 order left to right,
and each designator's net traces straight through to the signal its position implies, with no gaps or
reversals. The swap was an assembly-time mistake on this one hand-built unit (the wrong LED inserted
into the D5/D6 holes), not a fault in the layout. [`../tools/led_position_test/`](../tools/led_position_test/)
blinks each LED in turn while the display names it (read its note about polarity first). TR and RD are
driven by the MAX3237 and cannot be blinked from the ESP32; identify them by opening a terminal on the
modem port (DTR asserted: TR goes dark) and by holding a break on the transmit line (RD goes dark).

### E12. Serial receive buffer was 256 bytes, not 4096 (fixed in firmware v6b)

The firmware asks for a 4096-byte modem UART receive buffer, but called `setRxBufferSize()` **after**
`begin()`, and the ESP32 Arduino core (3.3.11) rejects that once the port is running. The buffer was
therefore always the 256-byte default - in the published `firmware-v4-rev1` too. With **flow control off**
and traffic in both directions at full 115200 line rate, the receive path then overflowed and lost data in
whole 129-byte chunks. On the tested unit (PC echo server, 115200): echo streams lost data from 8 KB up
(in most runs from 20 KB up), and 30 KB and 40 KB streams were never clean (0 of 6). One-way transfers (PC sending, the
network only receiving) were lossless up to 100 KB, and RTS/CTS on was lossless in every test.

Firmware v6b sets the buffer before `begin()` and limits how many bytes the stream loop handles per pass,
because the bigger buffer alone made flow-controlled two-way traffic 30-100% slower. Measured after the
fix: flow-off echo streams from 3.6 KB to 40 KB were clean in all 20 runs; flow-controlled 200 KB two-way
streams were byte-exact at the original speed; flow-controlled 100 KB streams at 230400, 460800 and
921600 baud were byte-exact at ~17.5 KB/s.

**What remains:** with flow control off, a *sustained* full-speed two-way stream of about 60 KB can still
lose data (one of two runs did), because the modem forwards slightly slower than a full-speed sender can
push in both directions at once - no buffer size fixes that. **Use RTS/CTS (`AT&K3`) for long transfers.**
An occasional "one byte short on the echo return" in flow-off echo runs was also seen before the fix and
has an unknown cause.

### E13. On a fresh boot, a low PC RTS line stops the modem answering at all (open issue)

With flow control at its default (**off**) and the PC's RTS line **low**, a genuinely fresh boot
(never touched `AT&K3` yet) gives **zero response**, not just a slow one - tested to 20+ seconds
with nothing coming back. Either of these clears it for the rest of that boot session: raising RTS
once (the modem then answers in about 30 ms), or issuing a real `AT&K3` followed by `AT&K0` from a
terminal. Reproduced identically on `firmware-v4-rev1`, `firmware-v6b-rev1` and `firmware-v7-rev1` -
pre-existing, not something v7 introduced or fixed. Also reproduced byte-for-byte through the PC's
native 16550 UART (**COM1**), not just a USB-serial (FTDI) adapter, ruling out the USB-serial
interface as the cause - this sits on the modem board's side.

**Workaround:** make sure your terminal/cable asserts RTS on connect (most do by default), or send
`AT&K3` then `AT&K0` once after a fresh boot before relying on flow-control-off behaviour.

**Why this is a real risk:** a terminal, OS, or cable that doesn't proactively assert RTS (plausible -
many don't manage handshaking lines unless flow control is turned on) would make a freshly booted,
correctly wired board look completely dead.

**Investigation so far:** six firmware-only approaches were tried and disproven on real hardware,
including binding the CTS pin's GPIO Matrix routing earlier, priming hardware flow control on/off
during `setup()`/reset in several orderings, and reactively re-running the same working `AT&K3`/`AT&K0`
sequence the moment the first serial byte is received after boot. All failed identically, including
the first-byte approach even when the very first byte sent was the one meant to trigger it - meaning
the byte never reached the firmware's dispatch loop at all while RTS was low. Cross-checked against
the ESP32-S3 ESP-IDF v5.5.5 source (`uart_ll_set_hw_flow_ctrl()`): the register write that disables
hardware flow control is unconditional and deterministic, which doesn't obviously explain a stall on
its own. The evidence points toward a gate below what the firmware can see or reach - most likely
something in the UART peripheral's RX path that a plain `uart_set_hw_flow_ctrl(..., DISABLE)` call
doesn't fully undo until one real hardware-flow-control cycle has run - rather than anything a future
firmware-only fix from this angle is likely to solve. Not pursued further without new information
(e.g. a scope/meter reading on the CTS-input pin at the moment of the stall).

## Suggestions for a future revision (not commitments)

- Add pull resistors (or solder-jumper defaults) for J4/J5/J8 so a missing
  jumper cannot silently kill RS-232.
- Use the header pin order of the cables in common use (DTK/Intel), or add a
  solder-jumper to pick either order, or provide a DE-9 directly on the board.
- Add test points on the 5 V, 3.3 V and GND rails.
- Silkscreen J7 pins 3/4 as `DBG TX/RX`.
- Replace U4 (74HCT245) with a pin-compatible inverting part (candidate: 74HCT640) in the design, so all
  eight LEDs read correctly; and check active levels, not only connectivity, when auditing LED/signal paths.
- Pull-downs on the VFD's E and RS lines (see E8).
