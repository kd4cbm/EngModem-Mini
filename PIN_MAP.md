# EngModem Mini - Hardware Pin Map

This file covers the board's **external connectors** - what plugs into it
and how those pins map to real-world signals. For the internal ESP32-S3
GPIO-to-signal mapping (which pin drives what inside the firmware), see
the firmware's own
[`PIN_MAP.md`](firmware/PIN_MAP.md) -
duplicating that table here would just drift out of sync over time, so
this file stays scoped to the board's edge connectors and power system.

## RS-232 (J12) - DE-9 female, DCE pinout

Verified pin-by-pin against the schematic netlist, not assumed from
convention. The header pin number equals the DE-9 pin number (the
**AT/Everex** header order), so wiring J12 1:1 to a DE-9 gives the standard
DCE pinout and a normal straight-through PC serial cable works (no null-modem).
**But many ready-made 2x5-header-to-DE-9 cables use a different (DTK/Intel)
order and will not work** - see [`docs/J12_CABLE_GUIDE.md`](docs/J12_CABLE_GUIDE.md).

| Pin | Signal | Full name | Direction (modem/DCE POV) |
|---|---|---|---|
| 1 | DCD | Data Carrier Detect | Output (modem asserts) |
| 2 | RXD | Received Data | Output (modem transmits here - host receives) |
| 3 | TXD | Transmitted Data | Input (host transmits here - modem receives) |
| 4 | DTR | Data Terminal Ready | Input (host asserts) |
| 5 | GND | Signal Ground | - |
| 6 | DSR | Data Set Ready | Output (modem asserts) |
| 7 | RTS | Request To Send | Input (host asserts) |
| 8 | CTS | Clear To Send | Output (modem asserts) |
| 9 | RI | Ring Indicator | Output (modem asserts) |

Pins 2/3 are named from the DTE's (host's) point of view, so the modem's
own UART TX actually drives pin 2 and its RX reads pin 3 - the opposite
of what the names alone suggest.

**RTS/CTS direction (verified on hardware):** on this board the PC's RTS
arrives on ESP32 GPIO17 (an *input*, the firmware's CTS) and the ESP32 drives
the PC's CTS from GPIO18 (an *output*, the firmware's RTS) - the reverse of
the DevKitC-1 assignment. `firmware-v2` and later handle this.

The connector's footprint (`IDC-Header_2x05_P2.54mm_Vertical`) has a 10th
pad beyond the standard 9 signal pins (`unconnected-(J12-Pin_10-Pad10)`
in the netlist) - a mechanical/shield tab on the specific part, not a
signal. Nothing to wire there.

## Jumpers, USB and debug headers

| Ref | Function | Tested setting |
|---|---|---|
| J4 | MAX3237 `EN` (pin 1 VCC, 2 signal, 3 GND) | 2-3 |
| J5 | MAX3237 `SHDN` | 1-2 |
| J8 | MAX3237 `MBAUD` (1-2 = MegaBaud up to ~1 Mbps) | 1-2 |
| J9 | ESP32 `BOOT` (GPIO0 to GND) | **off** to run the firmware |
| J10 | ESP32 `EN` (short to GND to reset) | open |
| J3 | DC in: 1 = `/DC-IN`, 2 = GND | - |
| J6 | USB-C signal breakout: 1 GND, 2 +5 V net, 3 GND, 4 D-, 5 D+, 6 GND | - |
| J7 | 2x4 expansion + **debug UART**: 1 GPIO21, 2 GPIO38, **3 TX (GPIO43)**, **4 RX (GPIO44)**, 5-6 GND, 7 3.3 V, 8 +5 V | - |

Details, and why J4/J5/J8 must be fitted, are in
[`docs/BRING_UP.md`](docs/BRING_UP.md#3-set-the-jumpers).

## VFD (J1) - 14-pin header

Drives a Noritake CU24025ECPB-W1J 24x2 character VFD directly over its
4-bit parallel bus (RS, E, DB4-7) - no I2C bridge, no second
microcontroller. R/W is hardwired to GND on the VFD module itself. See
the firmware repo's `PIN_MAP.md` for which GPIO drives which line.

## microSD (J11)

Hirose DM3AT-SF-PEJM5 push-push socket, fully SMD, on the ESP32-S3's
non-default SPI pins (CS/MOSI/MISO/SCK) - requires an explicit `SPIClass`
instance in firmware rather than the default `SD.begin(cs)`. See the
firmware repo's `PIN_MAP.md` for the exact GPIO assignment.

## Power regulation - U3, U5

Two D2PAK/TO-263-3 linear regulators supply the board's rails: **U3**
(5V) and **U5** (3.3V). Neither is GPIO-mapped.

**Both originally spec'd parts don't reliably match this board's 3-lead
footprint** - a real, datasheet-confirmed mismatch, not a JLCPCB-preview
quirk. Verified replacements (pin/tab-matched to the existing footprint,
confirmed against each manufacturer's own datasheet):

| Ref | Rail | Originally spec'd | Verified replacement | Package |
|---|---|---|---|---|
| U3 | 5V | HGSEMI LM7805S2/TR | onsemi **MC7805CD2TR4G** | D2PAK-3, CASE 936 |
| U5 | 3.3V | ST LD1086D2T33TR | TI **LM1086CSX-3.3/NOPB** (qualified board: **LM1086IS-3.3/NOPB**, same package/pinout, in stock when CSX was not) | DDPAK/TO-263, package KTT |

- **U3** (onsemi MC7800 datasheet): Pin 1=Input, Pin 2=Ground=Tab, Pin
  3=Output.
- **U5** (TI LM1086 datasheet): Pin 1=ADJ/GND, Pin 2+Tab=Output, Pin
  3=Input - matches this board's design intent, which wires the tab pad
  to Output rather than the more common Ground.

Full sourcing detail (why the originals were excluded, JLCPCB PCBA
status) is in [`manufacturing/BOM_full.csv`](manufacturing/BOM_full.csv)
and the README's [manufacturing section](README.md#files).
