import time
from qa_common import *
m = M(115200)
check("start: link alive at 115200", m.sync())
m.s.rts = True
time.sleep(0.2)
check("baseline (flow off, boot): PC sees CTS asserted", m.s.cts is True, str(m.s.cts))
for cycle in range(3):
    r = m.send(b"AT&K3", 0.5)
    time.sleep(0.2)
    check("cycle %d: &K3 accepted, CTS asserted" % cycle, b"OK" in r and m.s.cts is True, "cts=%s %r" % (m.s.cts, r))
    m.s.rtscts = False
    r = m.send(b"AT&K0", 0.5)
    time.sleep(0.2)
    check("cycle %d: after &K0 CTS still asserted (was False on v2)" % cycle, b"OK" in r and m.s.cts is True, "cts=%s %r" % (m.s.cts, r))
    check("cycle %d: link alive" % cycle, m.sync())
# TX must not be gated by PC RTS once flow control is off
m.s.rts = False
time.sleep(0.2)
r = m.send(b"AT", 0.6)
check("flow off: modem still answers with PC RTS deasserted", b"OK" in r, repr(r))
m.s.rts = True
# &K3 again with RTS low should gate TX, then release
m.send(b"AT&K0", 0.4)
check("end: flow off, link alive", m.sync())
m.close()
summary()
