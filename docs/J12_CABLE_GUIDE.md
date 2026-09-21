# J12 to DE-9: getting the cable right

**Short version: most ready-made "2x5 header to DE-9" cables will NOT work on
J12.** They use a different pin order. On the tested board a common
header-to-DE-9 bracket cable gave *no serial output at all* - it looked
exactly like dead firmware until the cable was ruled out.

J12 is a 2x5, 0.1" (2.54 mm) header. The board is wired as a **DCE** (a
modem), so a straight-through cable to a PC's COM port is correct - no
null-modem.

## The two header pin orders in the wild

Serial-port bracket cables for PC motherboards come in two flavours. J12 uses
the first.

| Header pin | **AT / Everex order - J12 (this board)** | DTK / Intel order (very common cable) |
|---|---|---|
| 1 | DCD | DCD |
| 2 | RXD | DSR |
| 3 | TXD | RXD |
| 4 | DTR | RTS |
| 5 | GND | TXD |
| 6 | DSR | CTS |
| 7 | RTS | DTR |
| 8 | CTS | RI |
| 9 | RI | GND |
| 10 | NC | NC |

On J12 the header pin number equals the DE-9 pin number for pins 1-9. That is
why the design *looks* standard - but a DTK/Intel cable plugged onto it swaps
RXD, TXD and GND and nothing connects.

The J12 silkscreen labels were checked pad-by-pad against the PCB and are
correct. This is a property of the design (a mismatch with the cable you own),
not a board defect. A future revision could adopt whichever order the cables in
common use have; it is listed in [`ERRATA.md`](ERRATA.md#e1-j12-pin-order-differs-from-common-bracket-cables).

## Which cable do I have?

With the cable unplugged, use a multimeter in continuity mode between the
**header socket** pin positions and the **DE-9 plug** pins:

| Header position 3 reaches DE-9 pin... | Cable type | Works on J12? |
|---|---|---|
| **3** | AT / Everex | Yes |
| **2** | DTK / Intel | **No** |

(Find header position 1 by continuity or by the marked/square end of the
connector; pins 1,3,5,7,9 are one row and 2,4,6,8,10 the other.)

## Option A (tested): wire J12 1:1 to a DE-9 yourself

Connect each J12 pin to the same-numbered DE-9 pin using a female DE-9
(so a normal straight cable runs to the PC). This is what was used to qualify
the board - first with a DE-9 breakout board fed from Dupont leads, then with
the wiring confirmed by continuity checks.

| J12 pin | Signal | DE-9 pin | Direction (modem is DCE) |
|---|---|---|---|
| 1 | DCD | 1 | modem out |
| 2 | RXD | 2 | modem out (PC receives) |
| 3 | TXD | 3 | modem in (PC transmits) |
| 4 | DTR | 4 | modem in |
| 5 | GND | 5 | - |
| 6 | DSR | 6 | modem out |
| 7 | RTS | 7 | modem in |
| 8 | CTS | 8 | modem out |
| 9 | RI | 9 | modem out |
| 10 | NC | - | leave unconnected |

For a plain terminal without flow control you only need **RXD (2), TXD (3) and
GND (5)**; the others are for DCD/DTR/DSR/RI signalling and RTS/CTS flow
control.

## Option B (derived, NOT tested): adapt a DTK/Intel cable

If you already own a DTK/Intel cable, put a small remapping adapter between
J12 and the cable's header socket. For each cable-socket pin (left) take the
signal from the J12 pin (right):

| DTK/Intel cable socket pin | carries | connect to J12 pin |
|---|---|---|
| 1 | DCD | 1 |
| 2 | DSR | 6 |
| 3 | RXD | 2 |
| 4 | RTS | 7 |
| 5 | TXD | 3 |
| 6 | CTS | 8 |
| 7 | DTR | 4 |
| 8 | RI | 9 |
| 9 | GND | 5 |

This table is derived from the two published pin orders above; it has **not**
been built or tested. It is not a simple reversal, so re-crimping a DTK/Intel
cable's connector "the other way round" does not fix it - the pins must be
individually remapped.

## Check it works

1. Terminal at 1200 baud 8N1 (factory default), no flow control.
2. Power the board, wait for the VFD to show it is ready, type `AT` + Enter.
3. `OK` = cable good. Nothing = check jumpers J4/J5/J8
   ([`BRING_UP.md`](BRING_UP.md#3-set-the-jumpers)) *and* this cable.
