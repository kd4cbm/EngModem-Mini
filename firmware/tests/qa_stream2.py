import socket, threading, time, sys
from qa_common import *

PC_IP, PORT_TCP = "192.168.1.140", 2323
FLOW = (sys.argv[1] == "flow") if len(sys.argv) > 1 else True
NBYTES = int(sys.argv[2]) if len(sys.argv) > 2 else 3600
BAUD = int(sys.argv[3]) if len(sys.argv) > 3 else 115200
print("mode: flow control %s, %d bytes" % ("ON (&K3)" if FLOW else "OFF (&K0)", NBYTES))
srv = socket.socket(); srv.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
srv.bind(("0.0.0.0", PORT_TCP)); srv.listen(1); srv.settimeout(1.0)
state = {"rx": b"", "stop": False, "conn": None}
def serve():
    while not state["stop"]:
        try: c, a = srv.accept()
        except socket.timeout: continue
        state["conn"] = c; c.settimeout(0.2)
        while not state["stop"]:
            try:
                d = c.recv(4096)
                if not d: break
                state["rx"] += d; c.sendall(d)
            except socket.timeout: continue
            except OSError: break
        try: c.close()
        except OSError: pass
        state["conn"] = None
threading.Thread(target=serve, daemon=True).start()

m = M(115200)
check("start: link alive", m.sync())
if BAUD != 115200:
    m.send(b"AT&K0", 0.3); m.modem_baud(BAUD); check("modem at %d" % BAUD, m.sync())
if FLOW:
    r = m.send(b"AT&K3", 0.5); check("&K3 accepted", b"OK" in r, repr(r))
    m.s.rtscts = True
m.s.reset_input_buffer()
m.s.write(("ATD%s:%d\r" % (PC_IP, PORT_TCP)).encode()); m.s.flush()
resp = m.drain(8.0)
check("CONNECT", b"CONNECT" in resp, repr(resp))
time.sleep(0.5)
check("DCD asserted", m.s.cd is True, str(m.s.cd))

pat = b"ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789"
big = (pat * (NBYTES // len(pat) + 1))[:NBYTES]
state["rx"] = b""
m.s.reset_input_buffer()
def writer():
    m.s.write(big); m.s.flush()
threading.Thread(target=writer, daemon=True).start()
got = b""; t0 = time.time(); last = t0
while time.time() - t0 < 120 and len(got) < len(big):
    d = m.s.read(4096)
    if d: got += d; last = time.time()
    if time.time() - last > 10: break
dt = time.time() - t0
check("stream echoed byte-exact through TCP", got == big,
      "sent %d, echoed %d, server got %d, %.1f s, ~%d B/s" % (len(big), len(got), len(state["rx"]), dt, len(got)/max(dt,.001)))
if not FLOW: pass
m.s.rtscts = False; m.s.rts = True
time.sleep(1.5)
m.s.write(b"+++"); m.s.flush(); time.sleep(1.5)
esc = m.drain(1.5)
check("+++ returns OK", b"OK" in esc, repr(esc))
r = m.send(b"ATH", 2.5)
check("ATH hangs up", b"NO CARRIER" in r or b"OK" in r, repr(r))
time.sleep(0.5)
check("DCD deasserted", m.s.cd is False, str(m.s.cd))
m.send(b"AT&K0", 0.5)
if BAUD != 115200:
    m.modem_baud(115200)
check("flow off, back at 115200, link healthy", m.sync())
state["stop"] = True; time.sleep(0.5)
m.close(); srv.close()
summary()
