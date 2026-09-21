import socket, threading, time
from qa_common import *
PC_IP, PORT_TCP = "192.168.1.140", 2323
srv = socket.socket(); srv.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
srv.bind(("0.0.0.0", PORT_TCP)); srv.listen(1); srv.settimeout(1.0)
st = {"stop": False, "closed_by_peer": None}
def serve():
    while not st["stop"]:
        try: c, a = srv.accept()
        except socket.timeout: continue
        c.settimeout(0.2)
        while not st["stop"]:
            try:
                d = c.recv(4096)
                if not d: st["closed_by_peer"] = True; break
                c.sendall(d)
            except socket.timeout: continue
            except OSError: break
        c.close()
threading.Thread(target=serve, daemon=True).start()
m = M(115200)
check("start: link alive", m.sync())
r = m.send(b"ATS63?", 0.5); print("   S63 initially:", repr(r))
r = m.send(b"ATS63=2", 0.5); check("ATS63=2 (hang up on DTR drop) accepted", b"OK" in r, repr(r))
for attempt in range(2):
    m.s.dtr = True; time.sleep(0.3)
    m.s.reset_input_buffer()
    m.s.write(("ATD%s:%d\r" % (PC_IP, PORT_TCP)).encode()); m.s.flush()
    resp = m.drain(6.0)
    check("attempt %d: CONNECT" % attempt, b"CONNECT" in resp, repr(resp[-30:]))
    check("attempt %d: DCD asserted" % attempt, m.s.cd is True, str(m.s.cd))
    m.s.write(b"HELLO"); m.s.flush(); e = m.drain(1.0)
    check("attempt %d: data flows before DTR drop" % attempt, e == b"HELLO", repr(e))
    m.s.reset_input_buffer()
    m.s.dtr = False
    t0 = time.time(); txt = b""
    while time.time() - t0 < 3.0:
        txt += m.s.read(4096)
    check("attempt %d: DTR drop -> NO CARRIER" % attempt, b"NO CARRIER" in txt, repr(txt))
    check("attempt %d: DCD deasserted after DTR hangup" % attempt, m.s.cd is False, str(m.s.cd))
    m.s.dtr = True; time.sleep(0.5)
    check("attempt %d: back in command mode" % attempt, m.sync())
r = m.send(b"ATS63=0", 0.5); check("ATS63=0 restored", b"OK" in r, repr(r))
# control: with S63=0, dropping DTR must NOT hang up
m.s.reset_input_buffer()
m.s.write(("ATD%s:%d\r" % (PC_IP, PORT_TCP)).encode()); m.s.flush(); resp = m.drain(6.0)
check("control: CONNECT with S63=0", b"CONNECT" in resp, repr(resp[-30:]))
m.s.dtr = False; time.sleep(2.0)
check("control: DTR drop with S63=0 leaves call up (DCD still on)", m.s.cd is True, str(m.s.cd))
m.s.dtr = True; time.sleep(1.5); m.s.write(b"+++"); m.s.flush(); time.sleep(1.5); m.drain(1.0)
m.send(b"ATH", 2.0); time.sleep(0.5)
check("cleanup: hung up, DCD off, link healthy", m.s.cd is False and m.sync(), str(m.s.cd))
st["stop"] = True; time.sleep(0.4); m.close(); srv.close(); summary()
