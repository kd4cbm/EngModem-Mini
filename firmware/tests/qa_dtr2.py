import time, serial
from qa_common import *

dbg = serial.Serial("COM5", 115200, timeout=0.05)
m = M(115200)
m.sync()

def grab(secs):
    t0 = time.time(); data = b""
    while time.time() - t0 < secs:
        d = dbg.read(4096)
        if d: data += d
    return data.decode("utf-8", "replace")

m.s.dtr = True; m.s.rts = True
dbg.reset_input_buffer()
print("enable ->", repr(m.send(b"AT&O88", 0.5)))
print("[after enable]   ", repr(grab(1.0)))
for i in range(3):
    dbg.reset_input_buffer(); m.s.dtr = False
    print("[DTR off #%d]      " % (i + 1), repr(grab(0.8)), " pc lines: DSR=%s CTS=%s" % (m.s.dsr, m.s.cts))
    dbg.reset_input_buffer(); m.s.dtr = True
    print("[DTR on  #%d]      " % (i + 1), repr(grab(0.8)))
dbg.reset_input_buffer(); m.s.rts = False
print("[RTS off]        ", repr(grab(0.8)))
dbg.reset_input_buffer(); m.s.rts = True
print("[RTS on]         ", repr(grab(0.8)))
m.send(b"AT&O0", 0.6); m.send(b"AT&O87", 0.5)
m.close(); dbg.close()
