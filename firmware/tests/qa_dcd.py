import socket, threading, time, re, serial
from qa_common import *

PC_IP, PORT_TCP = "192.168.1.140", 2323
srv = socket.socket(); srv.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
srv.bind(("0.0.0.0", PORT_TCP)); srv.listen(1); srv.settimeout(1.0)
state = {"conn": None, "rx": b"", "stop": False}
def serve():
    while not state["stop"]:
        try:
            c, addr = srv.accept()
        except socket.timeout:
            continue
        state["conn"] = c; state["addr"] = addr
        c.settimeout(0.2)
        while not state["stop"]:
            try:
                d = c.recv(4096)
                if not d: break
                state["rx"] += d; c.sendall(d)      # echo
            except socket.timeout:
                continue
            except OSError:
                break
        try: c.close()
        except OSError: pass
        state["conn"] = None
th = threading.Thread(target=serve, daemon=True); th.start()

dbg = serial.Serial("COM5", 115200, timeout=0.05)
def grab(secs):
    t0 = time.time(); data = b""
    while time.time() - t0 < secs:
        d = dbg.read(4096)
        if d: data += d
    return data.decode("utf-8", "replace")
sig = re.compile(r"Signals: DCRSTOI \(([hl]{7})\)")

m = M(115200)
check("start: link alive", m.sync())
check("idle: PC sees DCD off", m.s.cd is False, str(m.s.cd))
m.send(b"AT&O88", 0.5); grab(0.5)
dbg.reset_input_buffer()

m.s.reset_input_buffer()
m.s.write(("ATD%s:%d\r" % (PC_IP, PORT_TCP)).encode()); m.s.flush()
resp = m.drain(8.0)
print("   dial response:", repr(resp))
check("dial: CONNECT received", b"CONNECT" in resp, repr(resp))
time.sleep(0.5)
check("DCD asserted at PC while connected", m.s.cd is True, str(m.s.cd))
check("server saw the inbound connection", state["conn"] is not None, str(state.get("addr")))
dlog = grab(0.5)
sigs = sig.findall(dlog)
print("   signal log while connected:", sigs[-3:])
check("firmware signal log: D bit low (DCD asserted)", bool(sigs) and sigs[-1][0] == "l", str(sigs[-1:] ))
check("RI stays inactive during outbound call (PC RI False)", m.s.ri is False, str(m.s.ri))

# data pass-through, alphanumerics only
payload = b"HELLO0123456789WORLD"
m.s.reset_input_buffer()
m.s.write(payload); m.s.flush()
echo = m.drain(1.5)
check("data modem->server arrives intact", state["rx"] == payload, repr(state["rx"]))
check("data server->modem echoes back intact", echo == payload, repr(echo))

# bigger stream, both directions, with flow control on
m.s.rtscts = False
big = (b"ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789" * 100)   # 3600 B
state["rx"] = b""
m.s.reset_input_buffer()
m.s.write(big); m.s.flush()
got = b""; t0 = time.time()
while time.time() - t0 < 15 and len(got) < len(big):
    got += m.s.read(4096)
check("3600 B stream through TCP and back byte-exact", got == big and state["rx"] == big,
      "tx=%d rx=%d server_rx=%d" % (len(big), len(got), len(state["rx"])))

# escape to command mode and hang up
time.sleep(1.2); m.s.write(b"+++"); m.s.flush(); time.sleep(1.2)
esc = m.drain(1.0)
print("   after +++:", repr(esc))
check("+++ escape returns OK", b"OK" in esc, repr(esc))
check("DCD still asserted in command mode with call up", m.s.cd is True, str(m.s.cd))
r = m.send(b"ATH", 2.0)
print("   ATH:", repr(r))
check("ATH -> NO CARRIER/OK", b"NO CARRIER" in r or b"OK" in r, repr(r))
time.sleep(0.5)
check("DCD deasserted after hangup", m.s.cd is False, str(m.s.cd))
sigs2 = sig.findall(grab(0.8))
print("   signal log after hangup:", sigs2[-2:])
check("firmware signal log: D bit high after hangup", bool(sigs2) and sigs2[-1][0] == "h", str(sigs2[-1:]))
check("link healthy", m.sync())

m.send(b"AT&O0", 0.6); m.send(b"AT&O87", 0.5)
check("log closed, AT answers", m.sync())
state["stop"] = True
m.close(); dbg.close(); srv.close()
summary()
