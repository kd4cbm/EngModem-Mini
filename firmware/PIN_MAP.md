# ESP32-S3-16R8 Pin Map (DevKitC-1 v1.1 / EngModem Mini)

Target board: originally ESP32-S3-DevKitC-1 v1.1 (ESP32-S3-WROOM-1-N16R8 module,
16MB flash, 8MB octal PSRAM) for breadboard prototyping. Verified against
Espressif's official J1/J3 header tables and GPIO reference docs. This firmware
fork (`ENGMODEM_MINI_BOARD` in `zimodem.ino`) targets the EngModem Mini custom
PCB, which deliberately reuses this exact GPIO layout - so the table below still
applies pin-for-pin - but is a different physical board: no onboard RGB LED (see
GPIO38 below), and it adds 3 direct-drive status LEDs the dev board never had
(see GPIO10/11/12 below).

**A note on the `J1-N`/`J3-N` notation below:** that's the row-position number from
Espressif's own header table (1-22 per header, matching their schematic) - it is
**not** what's printed on the physical board. The silkscreen next to each pin shows
the bare GPIO number only. So `J1-12` in this doc means "GPIO8, the 12th row down on
the left header" - when wiring, go by the GPIO number, not the position count.
Orientation: with both USB connectors facing you at the bottom and the WROOM module
at the top, **J1 is the left-side header, J3 is the right-side header**.

## Hard constraints (confirmed from Espressif documentation)

| Pins | Reason | Treatment |
|---|---|---|
| GPIO0, 3, 45, 46 | Strapping pins (boot mode, JTAG source, VDD_SPI voltage) | Never used |
| GPIO26-37 | Flash + octal PSRAM (26-32 shared flash/PSRAM, 33-37 octal-PSRAM-specific data/DQS lines) | Never used, even where physically broken out on the header |
| GPIO43, 44 | Fixed UART0 (TX/RX) - used by the onboard USB-serial bridge for flashing/monitor on the dev board. **On the EngModem Mini** they are the firmware's debug UART, brought out on header J7 (pin 3 = TXD, pin 4 = RXD, pins 5/6 = GND), 115200 8N1 | Reserved, not used for peripherals |
| GPIO19, 20 | USB D-/D+ | Reserved, not used for peripherals |
| GPIO47, 48 | SUBSPI differential clock pair (SPICLK_P/SPICLK_N) used to reach the in-package flash/octal PSRAM at its 1.8V domain on N16R8 modules. Espressif docs say "strongly not recommended to reconfigure." | Never used |
| GPIO38 | On the DevKitC-1 dev board: onboard addressable RGB LED (Espressif J3 header docs), cleared to off at boot. **On EngModem Mini: no RGB LED exists here** - `ENGMODEM_MINI_BOARD` skips that boot-time write (see `CHANGES.md`), and this pin is instead broken out as a general-purpose expansion header pin. | Dev board: never used for peripherals. Mini board: available (wired to the expansion header) |

## Pin assignments

| J-header position | GPIO | Signal | Notes |
|---|---|---|---|
| J1-4 | GPIO4 | DTR | |
| J1-5 | GPIO5 | DCD | |
| J1-6 | GPIO6 | RI | |
| J1-7 | GPIO7 | DSR | |
| J1-8 | GPIO15 | RXD | |
| J1-9 | GPIO16 | TXD | |
| J1-10 | GPIO17 | **CTS (firmware input)** | On the EngModem Mini this net (`/TTL-RTS`) is the MAX3237 ROUT2 output carrying the *PC's* RTS. Firmware v2 uses it as the UART CTS input. |
| J1-11 | GPIO18 | **RTS (firmware output)** | On the EngModem Mini this net (`/TTL-CTS`) drives MAX3237 DIN3, out to the *PC's* CTS. Firmware v2 uses it as the UART RTS output. |
| J1-12 | GPIO8 | VFD DB6 | |
| *(J1-13/14 = GPIO3, 46 - strapping pins, skipped)* | | | |
| J1-15 | GPIO9 | VFD DB7 | |
| J1-16 | GPIO10 | LED AA (auto-answer) | direct-drive, EngModem Mini only; spare on the bare dev board |
| J1-17 | GPIO11 | LED HS (high-speed) | direct-drive, EngModem Mini only; spare on the bare dev board |
| J1-18 | GPIO12 | LED OH (off-hook) | direct-drive, EngModem Mini only - lit while any connection (telnet/SSH/FTP/IRC/print) is open, driven from `checkOpenConnections()`, protocol-agnostic - see `CHANGES.md` |
| J1-19 | GPIO13 | SD CS | |
| J1-20 | GPIO14 | SD MOSI | |
| J3-4 | GPIO1 | SD MISO | spillover onto J3 |
| J3-5 | GPIO2 | SD SCK | spillover onto J3 |
| J3-6 | GPIO42 | VFD RS | direct parallel drive |
| J3-7 | GPIO41 | VFD E | |
| J3-8 | GPIO40 | VFD DB4 | |
| J3-9 | GPIO39 | VFD DB5 | |
| J3-18 | GPIO21 | *(spare)* | last spare GPIO on the board |

## Power regulation - not GPIO-mapped

The board's 5V and 3.3V rails come from two D2PAK/TO-263-3 linear
regulators (U3, U5) - neither is wired to any ESP32-S3 GPIO, so they
don't appear in the pin table above. Worth flagging here anyway since a
builder sourcing parts from this doc should know: the originally spec'd
parts for both don't reliably match the board's 3-lead footprint (a real
footprint mismatch, confirmed against the manufacturers' own datasheets,
not just a JLCPCB-preview quirk). Verified replacements: onsemi
**MC7805CD2TR4G** (U3, 5V) and TI **LM1086CSX-3.3/NOPB** (U5, 3.3V) - see
`CHANGES.md` and the hardware repo's `manufacturing/BOM_full.csv` for the
full pin/tab verification.

## VFD - direct parallel drive, no I2C bridge

The ESP32-S3 drives the VFD's 4-bit parallel bus directly (RS, E, DB4-7). R/W is
hardwired to GND on the VFD board itself, so it's not an MCU pin. This avoids needing
a separate bridge microcontroller, its own firmware/power/wiring, and a failure point.

## SD card - non-default SPI pins, requires an explicit SPIClass

The SD card's MOSI/MISO/SCK are **not** on the ESP32-S3 Arduino core's default SPI
bus pins, so `SD.begin(cs)` alone is not sufficient - it needs an explicitly
configured `SPIClass` instance:

```cpp
SPIClass sdSPI(FSPI);
sdSPI.begin(SD_SCK, SD_MISO, SD_MOSI, SD_CS);
SD.begin(SD_CS, sdSPI);
```

This is already handled in `zbrowser.ino`'s `initSDShell()`.

## Why SD and VFD both spill onto J3

Modem control (8 signals) + SD (4: CS/MOSI/MISO/SCK) + VFD (6: RS/E/DB4-7) = 18
signals. The two clean contiguous safe runs on J1 (positions 4-12 and 15-20) hold 15.
SD's MISO and SCK land on J3's first two safe pins (GPIO1/2), physically the very top
of that header, right next to TX/RX. VFD's RS/E/DB4/DB5 land on J3's next free run
(positions 6-9, four consecutive pins).

VFD's other two lines, DB6/DB7, do **not** follow onto J3 - the two pins that would
have continued that run (GPIO47/48) are the module's reserved SUBSPI clock pair (see
Hard constraints above). They land instead on J1-12/J1-15 (GPIO8/9). This splits the
VFD's six signal lines across two headers (RS/E/DB4/DB5 on J3, DB6/DB7 on J1).
