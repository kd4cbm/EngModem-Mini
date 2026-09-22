# EngModem Mini - builder's bring-up guide (Rev5)

Everything here was learned bringing up a real, assembled Rev5 board in
September 2026. Where something was *measured or observed* it says so; where
it is inferred from the schematic/datasheets it says that instead. The test
evidence is in [`QUALIFICATION.md`](QUALIFICATION.md); known problems are in
[`ERRATA.md`](ERRATA.md).

**Read section 3 (jumpers) before applying power.** Three of the boards'
jumpers have no pull resistors behind them; leaving them off gives a board
that boots but has dead RS-232, which looks exactly like a software problem.

## 1. What you need

| Item | Notes |
|---|---|
| Assembled Rev5 board | [`manufacturing/`](../manufacturing/) has Gerbers, BOM, CPL. SMD parts were JLCPCB-assembled on the tested board. |
| **U3 and U5 hand-soldered** | Excluded from PCBA on purpose - see [ERRATA](ERRATA.md#e2-u3-and-u5-are-hand-soldered-and-u5-was-substituted). Tested: U3 onsemi MC7805CD2TR4G, U5 TI LM1086IS-3.3/NOPB. |
| DC supply | Into **J3** (pin 1 = DC-IN, pin 2 = GND). U3 is a 7805-type linear regulator, so it needs enough headroom above 5 V (see its datasheet). The qualification did **not** characterise the supply range or record rail voltages. |
| USB-C via header **J6** (the USB-C signal breakout header - wire a USB-C breakout or panel jack to it) | For flashing. Pin 4 = D-, pin 5 = D+, pins 1/3/6 = GND. Pin 2 is the board's +5 V rail net (`/VFD-+5V`): **check the schematic before connecting a host's VBUS to it** while the board is powered from J3. |
| 3.3 V USB-serial adapter | For the debug UART on **J7** (see 5). Optional but very useful. |
| Serial cable / adapter from **J12** to a DE-9 | **Read [`J12_CABLE_GUIDE.md`](J12_CABLE_GUIDE.md) first** - most off-the-shelf header-to-DE-9 cables will not work. |
| 24x2 Noritake CU24025ECPB-W1J VFD | On header **J1** (14-pin). |
| microSD card | Optional; enables the `AT+SHELL` SD shell. |
| PC with a terminal program | ZOC, Tera Term, PuTTY, minicom... |

## 2. Inspect before power

1. Check for solder bridges on U1 (MAX3237, TSSOP) and the U2 module pads.
2. Confirm U3 and U5 orientation and that the **tabs are soldered**. On this
   board the U5 tab is wired to the *output* (3.3 V), not ground; U3's tab is
   ground. Mixing the two regulators up shorts a rail.
3. Fit the VFD only after the rails check out (see section 4).

## 3. Set the jumpers

Pin 1 is the pin marked/square on each header; on the three MAX3237 jumpers
**pin 1 = VCC (3.3 V), pin 2 = the signal, pin 3 = GND**.

| Ref | Function | Set to | Why |
|---|---|---|---|
| **J4** | MAX3237 `EN` (receiver enable, active low) | **2-3** (to GND) | Receivers on. Not fitted = floating input, RS-232 receive is unreliable/dead. |
| **J5** | MAX3237 `SHDN` (active low) | **1-2** (to VCC) | Device on. Not fitted or 2-3 = MAX3237 shut down. |
| **J8** | MAX3237 `MBAUD` | **1-2** (to VCC) | MegaBaud mode, up to ~1 Mbps. 2-3 = normal mode (250 kbps max). |
| **J9** | ESP32 `BOOT` (GPIO0) | **OFF** to run | GPIO0 low at reset = ROM download mode (the board will not run the firmware). Fit it only to force flashing. Held for >5 s *while running* it triggers a factory reset that **erases saved settings** (see [ERRATA E10](ERRATA.md#e10-saved-configuration-survives-reflashing)). |
| **J10** | ESP32 `EN` | open | Short briefly to GND to reset the module. |

The tested configuration was **J4 2-3, J5 1-2, J8 1-2**. J8 in 2-3
(normal mode) was **not** tested, so the 115200-and-below behaviour there is
inferred from the MAX3237 datasheet, not measured. The three MAX3237 lines
have no pull resistors on the board - they must be fitted.

Other headers, for reference (from the Rev5 netlist):

| Ref | Pins |
|---|---|
| **J3** DC in | 1 = `/DC-IN`, 2 = GND |
| **J6** USB | 1 GND, 2 +5 V net, 3 GND, 4 D-, 5 D+, 6 GND |
| **J7** expansion + debug UART (2x4) | 1 GPIO21, 2 GPIO38, **3 DEBUG-TXD (GPIO43)**, **4 DEBUG-RXD (GPIO44)**, 5 GND, 6 GND, 7 3.3 V, 8 +5 V |
| **J12** RS-232 (2x5) | 1 DCD, 2 RXD, 3 TXD, 4 DTR, 5 GND, 6 DSR, 7 RTS, 8 CTS, 9 RI, 10 NC |

## 4. First power (no VFD, no cables)

With J9 **off**, apply power to J3 and measure:

- 5 V at U3's output and 3.3 V at U5's output, before plugging in anything
  else. (The qualification did not record these voltages - measuring them is
  the first thing a new builder should do.)
- Supply current with only the ESP32 running should be modest; the VFD adds a
  noticeable load (its supply current increase is how the tested unit's
  builder first confirmed the VFD had power).

## 5. Flash the firmware

Follow [`../firmware/README.md`](../firmware/README.md#quick-start-flash-the-prebuilt-firmware).
Chip identified on the tested board: ESP32-S3 (QFN56), 8 MB PSRAM, 16 MB flash.

Connect the 3.3 V USB-serial adapter to **J7** (adapter RX to J7 pin 3, adapter
TX to J7 pin 4, GND to pin 5 or 6), 115200 8N1, to watch boot. Boot output is
**not** on USB.

Expected boot lines on the tested board:

```
External SD card initialized.
Connected to <your ssid> with IP <address>.
Flow control pins: CTS(in)=17 RTS(out)=18
```

(the last one confirms you are running v2-or-later firmware with the correct
RTS/CTS pin roles for this board).

## 6. Connect the VFD, LEDs, SD

- **VFD (J1)**: check every wire before power - the one failure seen during
  bring-up was a **loose VFD wire**: the display stayed blank although the
  supply current showed it had power. J1 was verified pin-for-pin against the
  VFD datasheet. The firmware sends the display's init sequence once, ~0.25 s
  after boot; if it is ever missed the display stays blank until the next reset.
- **LEDs**: left to right on the front panel MR, TR, SD, RD, OH, CD, AA, HS -
  see [`enclosure/LED_LABELS.md`](../enclosure/LED_LABELS.md) for what each
  one is wired to. Five are driven by RS-232/TTL signals through the 74HCT245
  (U4); OH, AA and HS are direct GPIOs (12, 10, 11). **Check the order at
  assembly** with [`tools/led_position_test/`](../tools/led_position_test/): on the
  tested unit two LEDs had been fitted swapped. **Expect an inverted-looking panel
  on Rev5 as built** - MR, TR, SD, RD and CD go through U4 from active-low signals
  and read inverted; OH, AA and HS are corrected in firmware v5+. At idle you should
  see MR off, TR on, SD on, RD on, OH off, CD on, AA off, HS on. Details and the
  planned fix (an inverting U4) are in [ERRATA E11](ERRATA.md#e11-most-front-panel-leds-read-inverted).
- **SD**: any microSD card in the Hirose socket; the tested board initialised
  it at boot and the SD shell worked.

## 7. Connect the serial cable and talk to it

1. Wire J12 to a DE-9 as described in [`J12_CABLE_GUIDE.md`](J12_CABLE_GUIDE.md).
   The board is wired as a **DCE** (a modem), so use a straight-through cable
   to a PC's COM port - not a null-modem.
2. Terminal at **1200 baud 8N1, no flow control** (the factory default).
3. Type `AT` then Enter: you should see `OK`. `ATI` prints version and
   network status. `AT+CONFIG` sets up WiFi (2.4 GHz only).
4. To run faster: `ATB115200`, reconnect the terminal at 115200, then `AT&W`
   to save. The board was qualified at every rate from 300 to 921600 baud.
   For rates above 250 kbps, **J8 must be 1-2** (MegaBaud).
5. Optional: `AT&K3` enables RTS/CTS flow control - both the terminal and the
   cable (DE-9 pins 7 and 8) must support it.

## 8. Quick checks that cover the whole board

| Check | How | Pass if |
|---|---|---|
| Serial | `AT` | `OK` |
| WiFi | `ATI` after `AT+CONFIG` | shows an IP address |
| VFD | look at it | shows WiFi/IP status, baud, flow mode |
| SD | `AT+SHELL`, then `?` | shell prompt / command list |
| LEDs | idle, then make a connection (`ATD<host>:<port>`) and watch them | Rev5 as built: idle panel MR off, TR on, SD on, RD on, OH off, CD on, AA off, HS on. During a connection expect **OH to turn on and CD to go dark** (CD reads inverted); TR goes dark when a terminal asserts DTR (seen on the tested unit); HS is expected to be dark below 38400 baud. See [ERRATA E11](ERRATA.md#e11-most-front-panel-leds-read-inverted). |
| Modem lines | terminal's status display, or the [QA scripts](../firmware/tests/README.md) | DSR/CTS asserted at idle; DCD asserts on connect; RI pulses on an incoming call |

## 9. Troubleshooting

| Symptom | Likely cause |
|---|---|
| Nothing on the serial port at all | Jumpers **J4/J5/J8** not fitted as in section 3, or a **DTK/Intel-wired** header cable (see the cable guide), or terminal at the wrong baud (default 1200). |
| Garbage characters | Baud mismatch - the board defaults to 1200 unless you saved another rate. |
| Board seems dead, no boot log, USB shows a download-mode device | **J9 (BOOT) still fitted.** Remove it and reset. |
| No boot messages on USB | By design - they are on the **J7** debug UART, not USB. |
| VFD blank | Loose/incorrect J1 wire (the one real case seen); then power-cycle so the init sequence is re-sent. |
| Terminal hangs after enabling `AT&K3` | The PC/cable is not driving RTS or honouring CTS; use `AT&K0`, or check DE-9 pins 7/8. |
| No serial after `ATB<rate>` | The terminal must be changed to the new rate too (the `OK` is already sent at the new rate). |
| Cannot flash | Use a data-capable USB cable; fit J9, pulse J10 or power-cycle, flash, then remove J9. |
