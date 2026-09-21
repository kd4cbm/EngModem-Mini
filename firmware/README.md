# EngModem Mini firmware (Zimodem-VFD, `ENGMODEM_MINI_BOARD`)

This folder is the firmware that was qualified on the real EngModem Mini
hardware - see [`../docs/QUALIFICATION.md`](../docs/QUALIFICATION.md) for what
was tested and how. It is a fork of Bo Zimmerman's
[Zimodem](https://github.com/BoZimmerman/Zimodem) (Hayes AT-command modem
emulator / internet gateway), via the
[Zimodem-VFD](https://github.com/kd4cbm/Zimodem-VFD) fork that adds the 24x2 VFD
status display, further specialised for this board with the
`ENGMODEM_MINI_BOARD` build flag (already `#define`d near the top of
`zimodem/zimodem.ino`).

**Revision:** `firmware-v4-rev1` - Zimodem 4.0.3 base, sdk v5.5.5,
ESP32 Arduino core 3.3.11. Every change from upstream is listed in
[`CHANGES.md`](CHANGES.md); the four numbered fix rounds (v1-v4) are listed
there too. Apache-2.0, see [`LICENSE`](LICENSE) and [`NOTICE`](NOTICE) (the
*hardware* in the rest of this repo is CERN-OHL-S v2).

| Folder / file | What it is |
|---|---|
| [`zimodem/`](zimodem/) | Arduino sketch source (`zimodem.ino` + tabs, bundled `src/libssh2`) |
| [`bin/`](bin/) | The exact binaries that were flashed for qualification, with `SHA256SUMS.txt` |
| [`tests/`](tests/) | The Python hardware QA scripts used for qualification |
| [`PIN_MAP.md`](PIN_MAP.md) | ESP32-S3 GPIO to signal mapping this firmware expects |
| [`CHANGES.md`](CHANGES.md) | Full change list vs. upstream, with the reason for each |

## Quick start: flash the prebuilt firmware

You need Python and [esptool](https://github.com/espressif/esptool)
(`pip install esptool`; the commands below are esptool 5.x syntax - with 4.x
use `write_flash` instead of `write-flash`).

1. **Jumpers first** - the board will not talk RS-232 without them. See
   [`../docs/BRING_UP.md`](../docs/BRING_UP.md#3-set-the-jumpers). In short:
   **J9 (BOOT) must be OFF** to run the firmware, J4 = 2-3, J5 = 1-2,
   J8 = 1-2.
2. Connect USB (header **J6**) and find the port (Windows: Device Manager >
   Ports; Linux: `/dev/ttyACM0`).
3. Flash the four images. Run from inside `firmware/bin/`:

```bash
python -m esptool --chip esp32s3 -p COM7 -b 460800 \
  --before default-reset --after hard-reset \
  write-flash --flash-mode dio --flash-freq 80m --flash-size 16MB \
  0x0     zimodem.ino.bootloader.bin \
  0x8000  zimodem.ino.partitions.bin \
  0xe000  boot_app0.bin \
  0x10000 zimodem.ino.bin
```

Replace `COM7` with your port. The chip resets itself into the bootloader over
native USB, so no button presses were needed on the tested board. If it does
not enter the bootloader, fit the **J9 BOOT** jumper, briefly short **J10 (EN)**
or power-cycle, flash, then **remove J9 again** and reset.

A single merged 16 MB image (`zimodem.ino.merged.bin`, flash at `0x0`) is
attached to the matching GitHub Release. Its layout was checked byte-for-byte
against the four images above; the four-image command is what was run on the
tested hardware.

Verify what you flashed against [`bin/SHA256SUMS.txt`](bin/SHA256SUMS.txt)
(`sha256sum -c SHA256SUMS.txt` in `bin/`).

## Debug output (important)

Boot messages, the `AT&O88` signal log and the
`Flow control pins: CTS(in)=17 RTS(out)=18` line are printed on the
**debug UART - header J7** (pin 3 = board TX, pin 4 = board RX, pins 5/6 =
GND; 3.3 V logic, 115200 8N1), **not** over USB. A 3.3 V USB-serial adapter
on J7 is all you need. (Earlier text in the upstream fork's README, saying
USB-CDC is the debug path, is wrong for this code.)

A healthy boot on the QA'd board shows, among other lines:
`External SD card initialized.`, `Connected to <ssid> with IP <ip>.` and
`Flow control pins: CTS(in)=17 RTS(out)=18`.

## First use

- Factory default is **1200 baud, 8N1, flow control off**. The unit that was
  qualified was configured to 115200 (`ATB115200`, reconnect the terminal at
  115200, then `AT&W` to save).
- `AT+CONFIG` walks you through WiFi (2.4 GHz only) and other settings.
- `ATI` shows version, chip and current IP.
- `AT&K3` enables RTS/CTS flow control, `AT&K0` disables it. Flow control was
  **left off** in qualification except during the flow-control tests.
- `ATS63=2` makes the modem hang up when the host drops DTR (verified on this
  board with firmware v3 and later; not possible before).
- The VFD shows WiFi/IP, TX/RX activity, baud rate and flow-control mode.
  `AT$VFDB=0..4` sets brightness (0 = off, 4 = 100 %).
- Full command reference: upstream Zimodem's documentation, plus the
  `AT+SHELL` SD-card shell (`?` lists its commands).

## Building from source

Tested with `arduino-cli` 1.5.2 and the `esp32:esp32` core **3.3.11**. No extra
libraries are needed (libssh2 is bundled under `zimodem/src`).

```bash
arduino-cli compile \
  --fqbn esp32:esp32:esp32s3:FlashSize=16M,PSRAM=opi,PartitionScheme=default_8MB,CDCOnBoot=cdc \
  --build-path build zimodem
```

That produces the four images above under `build/` (39 % of program storage,
17 % of RAM). A different core version may change behaviour: the v4 serial-to-TCP
fix in particular exists because of how core 3.x's socket write retries behave
(see [`CHANGES.md`](CHANGES.md)). Rebuilding will not reproduce the released
binaries bit-for-bit (timestamps are embedded), which is why the exact
released binaries and their checksums are included.

## Known limits of this revision

- Serial-to-TCP data is buffered for up to 10 ms (2 ms after the last byte) to
  avoid the stall described in `CHANGES.md`. Keystroke round-trip through a TCP
  echo on a home network measured a median of 45.7 or 61.3 ms (v3, without the
  buffering: 45.6 ms), so at most one ~15 ms step is added. Both revisions show
  the same 365-430 ms worst-case tail, which is ESP32 WiFi power-save.
- In *command mode* the modem processes roughly 4.6 KB/s regardless of line
  speed; flow control keeps data intact above that, but it is a ceiling.
  In connected (stream) mode throughput measured ~10.5 KB/s at 115200.
- `vfdInit()` sends the VFD's initialisation sequence exactly once, ~0.25 s
  after boot. If the display is blank, check the J1 wiring first, then reset.
- The `tests/` scripts have bench-specific COM ports and IP addresses hard-coded
  at the top; edit them before use (see [`tests/README.md`](tests/README.md)).
