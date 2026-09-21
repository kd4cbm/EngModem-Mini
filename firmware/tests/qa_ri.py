import socket, threading, time
from qa_common import *
MODEM_IP, PORT_TCP = "192.168.1.149", 2323
m = M(115200)
check("start: link alive", m.sync())
r = m.send(b"ATS0=0", 0.5); check("ATS0=0 (manual answer)", b"OK" in r, repr(r))
r = m.send(b"ATA%d" % PORT_TCP, 1.0); check("ATA2323 listener opened", b"OK" in r, repr(r))
check("idle: RI off at PC", m.s.ri is False, str(m.s.ri))

cli = {"s": None, "rx": b"", "err": None}
def connect():
    try:
        s = socket.create_connection((MODEM_IP, PORT_TCP), timeout=5)
        s.settimeout(0.2); cli["s"] = s
        while cli["s"] is not None:
            try:
                d = s.recv(4096)
                if not d: break
                cli["rx"] += d
            except socket.timeout: continue
            except OSError: break
    except OSError as e:
        cli["err"] = e
threading.Thread(target=connect, daemon=True).start()

t0 = time.time(); ri_trace = []; last_ri = m.s.ri; text = b""
while time.time() - t0 < 9.0:
    d = m.s.read(4096)
    if d: text += d
    ri = m.s.ri
    if ri != last_ri:
        ri_trace.append((round(time.time() - t0, 2), ri)); last_ri = ri
print("   serial text while ringing:", repr(text))
print("   RI transitions (t, level):", ri_trace)
check("TCP connection into modem accepted", cli["err"] is None and cli["s"] is not None, str(cli["err"]))
check("RING reported on serial", b"RING" in text, repr(text))
check("RI line toggled at PC (went active at least once)", any(v is True for _, v in ri_trace))
check("DCD still off before answering", m.s.cd is False, str(m.s.cd))

m.s.reset_input_buffer()
m.s.write(b"ATA\r"); m.s.flush()
resp = m.drain(3.0)
print("   ATA response:", repr(resp))
check("ATA answers -> CONNECT", b"CONNECT" in resp, repr(resp))
time.sleep(0.5)
check("DCD asserted after answer", m.s.cd is True, str(m.s.cd))
check("RI inactive after answer", m.s.ri is False, str(m.s.ri))
cli["s"].sendall(b"FROMPC12345"); time.sleep(1.0)
got = m.drain(1.0)
check("TCP client -> modem serial", got == b"FROMPC12345", repr(got))
m.s.write(b"TOPC67890"); m.s.flush(); time.sleep(1.0)
check("modem serial -> TCP client", cli["rx"] == b"TOPC67890", repr(cli["rx"]))

time.sleep(1.5); m.s.write(b"+++"); m.s.flush(); time.sleep(1.5)
esc = m.drain(1.0); check("+++ returns OK", b"OK" in esc, repr(esc))
r = m.send(b"ATH", 2.5); check("ATH hangs up", b"OK" in r or b"NO CARRIER" in r, repr(r))
time.sleep(0.5)
check("DCD deasserted", m.s.cd is False, str(m.s.cd))
check("RI inactive after hangup", m.s.ri is False, str(m.s.ri))
try: cli["s"].close()
except OSError: pass
cli["s"] = None
r = m.send(b"ATN", 0.6); check("ATN closes listener", b"OK" in r, repr(r))
r = m.send(b"ATS0=1", 0.6); check("ATS0=1 restored", b"OK" in r, repr(r))
check("link healthy", m.sync())
m.close(); summary()
