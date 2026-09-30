# LED polarity/position test

A small sketch that lights each front-panel LED in turn, steadily, for 3 seconds while the VFD names it
and shows its position and PCB designator, so you can confirm the LEDs sit in the right order (left to
right: **MR, TR, SD, RD, OH, CD, AA, HS** / designators **D8** to **D1**) and light with the correct
polarity after the U4 replacement (see [`../../docs/ERRATA.md`](../../docs/ERRATA.md#e11-most-front-panel-leds-read-inverted)).

**It replaces the modem firmware while it runs.** When you are done, re-flash the firmware from
[`../../firmware/bin/`](../../firmware/bin/). If you compile it with the same board options as the
firmware, the partition table is identical, so the modem's saved settings (baud rate, WiFi) survive.

```bash
arduino-cli compile --fqbn esp32:esp32:esp32s3:FlashSize=16M,PSRAM=opi,PartitionScheme=default_8MB,CDCOnBoot=cdc led_position_test
```

Flash the resulting application image at `0x10000` (see the firmware README for the esptool command).

## What you will see
Each slot shows `Pos N/8 D<n>` and ON/OFF (or the live pin level, for the two slots below) on the top
row, and the LED's label and function on the bottom row.

- **MR, SD, OH, CD, AA, HS** are driven by the ESP32, so exactly one of them lights in its slot, steadily,
  for 3 seconds. If the wrong LED lights, the LEDs are in the wrong physical order.
- **TR (position 2, D7) and RD (position 4, D5)** are driven by the MAX3237, not the ESP32 - forcing them
  would fight the transceiver - so they are **not driven**. They just stay lit or dark during the test and
  the display shows the live pin level. To identify them: open a terminal on the modem serial port
  (your PC asserts DTR - **TR goes dark**) and hold a break on the transmit line (**RD goes dark**).

## Polarity

`U4_INVERTING` (near the top of the sketch) is set to `1`: U4 has been replaced with an inverting
CD74HCT640, so MR, SD and CD light when the ESP32 pin is driven LOW, and the sketch accounts for that.
This is the first hardware confirmation run of that swap - if MR/SD/CD light in the wrong sense here (or
stay dark when the position is "ON"), the swap did not behave as predicted from the datasheet alone (see
ERRATA E11) and needs a closer look before trusting the front panel.

The sketch touches only GPIO and the VFD: no WiFi, no saved settings.
