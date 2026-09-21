import time, sys
from qa_common import *

RATES = [115200, 230400, 460800, 921600, 300, 1200, 2400, 9600, 19200, 38400, 57600, 115200]

m = M(115200)
print("waiting 5 s for the modem...", flush=True)
time.sleep(5)
check("start: link alive at 115200", m.sync())

for i, r in enumerate(RATES):
    label = "%6d" % r
    if i > 0:
        m.send(b"AT&K0", 0.4 + 100.0 / m.baud)      # never change baud with flow control on
        try:
            m.modem_baud(r)
        except Exception as e:
            check("%s baud: PC adapter accepts this rate" % label, False, repr(e))
            m.set_baud(RATES[i - 1])
            m.modem_baud(115200) if False else None
            continue
    if not m.sync():
        check("%s baud: modem answers AT" % label, False)
        # try to recover to 115200 so the sweep can continue
        for b in (115200, 9600, 1200, 300):
            m.set_baud(b)
            if m.sync(2):
                break
        continue
    check("%s baud: modem answers AT" % label, True)

    reply_t = 8 * 10.0 / r
    r1 = m.send(b"AT&K3", 0.5 + 12 * 10.0 / r)
    check("%s baud: AT&K3 accepted" % label, b"OK" in r1, repr(r1)[:40])

    m.s.rts = False
    time.sleep(0.2)
    m.s.reset_input_buffer()
    m.s.write(b"AT\r"); m.s.flush()
    held = m.drain(1.0 + 3 * 10.0 / r)
    check("%s baud: RTS low holds output" % label, len(held) == 0, "got %d bytes" % len(held))
    m.s.rts = True
    rel = m.drain(1.0 + reply_t + 5 * 10.0 / r)
    check("%s baud: RTS high releases it" % label, b"OK" in rel, "got %r" % rel[:20])

    m.send(b"AT&K0", 0.4 + 100.0 / r)

# leave the modem where the user had it: 115200, flow control off
m.send(b"AT&K0", 0.4)
ok = m.sync()
check("end: back at 115200, AT answers, flow control off", ok and m.baud == 115200)
m.close()
summary()
