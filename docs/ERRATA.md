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

### E8. VFD initialisation is sent once

The display's initialisation sequence is sent once, about 0.25 s after boot;
the splash-retry window only re-sends text and glyphs. A missed init (for
example a marginal or loose J1 wire) leaves the display blank until the next
reset. Check J1 wiring first, then power-cycle.

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

## Suggestions for a future revision (not commitments)

- Add pull resistors (or solder-jumper defaults) for J4/J5/J8 so a missing
  jumper cannot silently kill RS-232.
- Use the header pin order of the cables in common use (DTK/Intel), or add a
  solder-jumper to pick either order, or provide a DE-9 directly on the board.
- Add test points on the 5 V, 3.3 V and GND rails.
- Silkscreen J7 pins 3/4 as `DBG TX/RX`.
