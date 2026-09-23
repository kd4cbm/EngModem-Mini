# Hardware QA scripts

These are the scripts used to qualify the EngModem Mini Rev5 board
([`../../docs/QUALIFICATION.md`](../../docs/QUALIFICATION.md)). They drive the
board from a PC over its real serial port; there is no mocking. They are
shipped as they were run, so **bench-specific values are hard-coded** and you
must edit them for your setup:

| What | Where | As run |
|---|---|---|
| Modem serial port (J12 / DE-9) | `QA_PORT` environment variable (default in `qa_common.py`) | `COM15` (also run on the motherboard's native `COM1`) |
| Debug UART port (J7) | `serial.Serial("COM5", ...)` in `qa_dtr2.py`, `qa_dcd.py` | `COM5` |
| PC's LAN address (TCP tests, modem dials the PC) | `PC_IP` in `qa_dcd.py`, `qa_dtrhangup.py`, `qa_latency.py`, `qa_pace_noecho.py`, `qa_stream2.py` (`qa_losscurve.py` runs `qa_stream2.py`) | `192.168.1.140` |
| Modem's LAN address (incoming-call test) | `MODEM_IP` in `qa_ri.py` | `192.168.1.149` |
| TCP test port | `PORT_TCP` | `2323` |

Requirements: Python 3, `pip install pyserial`. The modem must be configured
for **115200 baud** at the DE-9 (the scripts start there and put it back), be
on the same LAN as the PC for the TCP tests, and have flow control **off**
(`AT&K0`) at the start. Tests that need a listening PC socket open one
themselves; allow it through the PC's firewall if the modem cannot connect.
Close any terminal program first - the scripts need exclusive use of the port.

| Script | Covers | Checks |
|---|---|---|
| `rtscts_verify.py` | Idle line levels; modem holds output while PC RTS is low and releases it; modem drops CTS when its RX buffer fills and recovers | 14 |
| `qa_sweep.py` | Baud sweep 300-921600: AT answers, RTS holds/releases output at each rate. Rates the PC port rejects (a native 16550 tops out at 115200) are skipped and listed, so the modem is never switched to a speed the PC cannot follow | 50 (38 on a 115200-limited port) |
| `qa_ctsoff.py` | PC-side CTS stays asserted across `AT&K3`/`AT&K0` cycles | 13 |
| `qa_dtr2.py` | DTR and RTS input paths, read from the firmware's own `AT&O88` signal log on the debug UART | prints the `DCRSTOI` bits |
| `qa_dtrhangup.py` | `ATS63=2` hang-up on DTR drop, twice, with an `ATS63=0` control | 18 |
| `qa_bulk.py` | Command-mode echo integrity under RTS/CTS at 9600/115200/921600. Its DTR/RTS section at the top reads the signal log from the *modem* port, but the log is printed on the *debug* port, so those 3 checks always fail - use `qa_dtr2.py` for them | 12 (+3 known-bad) |
| `qa_dcd.py` | Outbound call: DCD at the PC and in the firmware log, data, `+++`, `ATH`. Run on v3, where its 3600-byte stream section failed because of the serial-to-TCP stall that v4 fixes (that section also leaves the signal log running and flow control off, so it is a harsh test); use `qa_stream2.py` for stream integrity | - |
| `qa_stream2.py` | TCP stream integrity: `python qa_stream2.py flow` or `noflow`, then `BYTES` and optional `BAUD`, byte-exact echo, `+++`, `ATH`, DCD | ~10 |
| `qa_ri.py` | Incoming call: RING text, RI pulsing, `ATA`, DCD, data, hang-up. Uses runtime-only `ATS0=0` and `ATA2323`, reverted with `ATN` / `ATS0=1` | 20 |
| `qa_latency.py` | Keystroke round-trip time through a TCP echo | prints statistics |
| `qa_losscurve.py` | Data loss vs stream size with flow control OFF, full-duplex through a PC echo server: `python qa_losscurve.py LABEL 8000:4 20000:3 ...` (size:repeats). This is the test that exposed the receive-buffer bug fixed in v6b | prints lost bytes per run |
| `qa_pace_noecho.py` | One-way upload: PC sends, the PC-side server only counts: `python qa_pace_noecho.py CHUNK GAP_S TOTAL` | prints bytes received |
| `qa_common.py` | Shared helpers (`M` class: open the modem port, `send`, `sync`, `modem_baud`, `check`, `summary`) | - |

Notes:

- If a TCP test stalls the modem (the firmware v3 behaviour that v4 fixes), the
  modem can be left holding a dead connection with a large input backlog;
  reset the board (`J10` EN, or `esptool ... run`) rather than waiting.
- None of the scripts run `AT&W`; settings they change are runtime-only. Each
  script tries to leave the modem at 115200 with flow control off.
