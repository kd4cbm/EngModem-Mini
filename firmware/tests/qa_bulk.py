import time, random, re, threading, sys
from qa_common import *

ALPHA = "BCDEFGHIJKLMNOPQRSUVWXYZ0123456789"   # no A/T so no line can ever contain "AT"
random.seed(20260921)

def rand_line(n=60):
    return "".join(random.choice(ALPHA) for _ in range(n)).encode()

m = M(115200)
print("waiting 3 s...", flush=True)
time.sleep(3)
check("start: link alive at 115200", m.sync())

# ------------------------------------------------------------------ DTR / RTS inputs
print("\n== DTR and RTS input paths via the firmware's own signal log (AT&O88) ==")
r = m.send(b"AT&O88", 1.0)
print("   log enable reply:", repr(r)[:80])
sig_re = re.compile(rb"Signals: DCRSTOI \(([hl]{7})\)")

def last_sig(secs=0.7):
    data = m.drain(secs)
    found = sig_re.findall(data)
    return found[-1].decode() if found else None

def settle_and_read():
    return last_sig(0.8)

m.s.dtr = True; m.s.rts = True
base = settle_and_read()
print("   baseline (DTR,RTS asserted):", base)
m.s.dtr = False; s_dtr_off = settle_and_read()
m.s.dtr = True;  s_dtr_on = settle_and_read()
m.s.rts = False; s_rts_off = settle_and_read()
m.s.rts = True;  s_rts_on = settle_and_read()
print("   DTR off:", s_dtr_off, " DTR on:", s_dtr_on, " RTS off:", s_rts_off, " RTS on:", s_rts_on)
# order D C R S T O I -> index: D0 C1 R2 S3 T4 O5 I6
def bit(s, i): return None if s is None else s[i]
check("DTR: firmware sees PC DTR drop (T bit low->high)",
      bit(s_dtr_off, 4) == "h" and bit(s_dtr_on, 4) == "l",
      "off=%s on=%s" % (bit(s_dtr_off, 4), bit(s_dtr_on, 4)))
check("RTS: firmware CTS-in pin sees PC RTS drop (C bit low->high)",
      bit(s_rts_off, 1) == "h" and bit(s_rts_on, 1) == "l",
      "off=%s on=%s" % (bit(s_rts_off, 1), bit(s_rts_on, 1)))
check("DSR asserted (S low), RI inactive (I high), DCD inactive (D high) at idle",
      base is not None and base[3] == "l" and base[6] == "h" and base[0] == "h", base)
m.s.rts = True; m.s.dtr = True
m.send(b"AT&O0", 1.0)
m.send(b"AT&O87", 0.6)
check("signal log closed, AT still answers", m.sync())

# ------------------------------------------------------------------ bulk integrity
def bulk(baud, cap):
    print("\n== bulk integrity at %d baud, RTS/CTS on both sides ==" % baud, flush=True)
    m.send(b"AT&K0", 0.4)
    if baud != m.baud:
        m.modem_baud(baud)
    if not m.sync():
        check("%d: modem answers before bulk" % baud, False); return
    # learn the exact reply format for one junk line with flow control off
    p = rand_line()
    m.s.reset_input_buffer()
    m.s.write(b"QQ" + p + b"\r"); m.s.flush()
    probe = m.drain(0.8 + 200.0 / baud * 10)
    if p not in probe:
        check("%d: probe line echoed" % baud, False, repr(probe)[:80]); return
    template = probe.replace(p, b"<P>")
    print("   reply template:", repr(template))
    per_line = 63 + len(template) - 3
    nlines = max(40, min(cap, int(15 * baud / 10 / per_line)))
    lines = [rand_line() for _ in range(nlines)]
    expected = b"".join(template.replace(b"<P>", l) for l in lines)

    r1 = m.send(b"AT&K3", 0.5 + 100.0 / baud)
    check("%d: AT&K3 accepted" % baud, b"OK" in r1, repr(r1)[:30])
    m.s.rtscts = True          # PC now honours the modem's CTS and drives RTS from its own buffer
    m.s.reset_input_buffer()

    got = bytearray()
    done = threading.Event()
    def writer():
        for l in lines:
            m.s.write(b"QQ" + l + b"\r")
        m.s.flush()
        done.set()
    t0 = time.time()
    th = threading.Thread(target=writer, daemon=True); th.start()
    last_rx = time.time()
    while True:
        d = m.s.read(65536)
        if d:
            got.extend(d); last_rx = time.time()
        if len(got) >= len(expected) and done.is_set():
            break
        if done.is_set() and time.time() - last_rx > 4.0:
            break
        if time.time() - t0 > 90:
            break
    dt = time.time() - t0
    th.join(2)
    m.s.rtscts = False
    m.s.rts = True
    same = bytes(got) == expected
    detail = "%d lines, sent %d B, got %d B (expected %d B), %.1f s, ~%d B/s" % (
        nlines, nlines * 63, len(got), len(expected), dt, len(got) / max(dt, 0.001))
    if not same:
        n = min(len(got), len(expected))
        first = next((i for i in range(n) if got[i] != expected[i]), n)
        detail += "; first mismatch at byte %d" % first
    check("%d: echoed stream matches byte-for-byte under flow control" % baud, same, detail)
    m.send(b"AT&K0", 0.5)
    check("%d: link healthy after bulk" % baud, m.sync())

bulk(9600, 200)
bulk(115200, 2500)
bulk(921600, 6000)

m.send(b"AT&K0", 0.4)
if m.baud != 115200:
    m.modem_baud(115200)
check("end: back at 115200, flow control off", m.sync() and m.baud == 115200)
m.close()
summary()
