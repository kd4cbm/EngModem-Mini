# LED position test

A small sketch that blinks each front-panel LED in turn for 3 seconds while the VFD names it, so you can
confirm the LEDs sit in the right order (left to right: **MR, TR, SD, RD, OH, CD, AA, HS**).

**It replaces the modem firmware while it runs.** When you are done, re-flash the firmware from
[`../../firmware/bin/`](../../firmware/bin/). If you compile it with the same board options as the
firmware, the partition table is identical, so the modem's saved settings (baud rate, WiFi) survive.

```bash
arduino-cli compile --fqbn esp32:esp32:esp32s3:FlashSize=16M,PSRAM=opi,PartitionScheme=default_8MB,CDCOnBoot=cdc led_position_test
```

Flash the resulting application image at `0x10000` (see the firmware README for the esptool command).

## What you will see
Each slot shows `Position N of 8` on the top row and the LED's label and function on the bottom row.

- **MR, SD, OH, CD, AA, HS** are driven by the ESP32, so exactly one of them blinks in its slot.
  If the wrong LED blinks, the LEDs are in the wrong physical order (on the tested unit the LEDs at
  positions 3 and 4 had been fitted swapped).
- **TR (position 2) and RD (position 4)** are driven by the MAX3237, not the ESP32 - forcing them would
  fight the transceiver - so they are **not blinked**. They just stay lit (or dark) during the test and
  the display shows the live pin level. To identify them: open a terminal on the modem serial port
  (your PC asserts DTR - **TR goes dark**) and hold a break on the transmit line (**RD goes dark**).
  Check the serial cable and the DTR lead first if nothing changes; on the tested unit a slipped
  connector pin looked exactly like a dead LED.

## Polarity
On Rev5 as built, the LEDs that go through U4 read inverted (see
[`../../docs/ERRATA.md`](../../docs/ERRATA.md#e11-most-front-panel-leds-read-inverted)), but this sketch
drives the *pins*, so it still blinks the right LEDs. After U4 has been replaced with an inverting buffer,
set `U4_INVERTING` to `1` near the top of the sketch so MR, SD and CD blink lit-on-dark again. That
setting is **untested on hardware** (the swap had not been done when this was written).

The sketch touches only GPIO and the VFD: no WiFi, no saved settings.
