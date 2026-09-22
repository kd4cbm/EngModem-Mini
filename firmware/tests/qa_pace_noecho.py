import socket, threading, time, sys
from qa_common import *
PC_IP, PORT_TCP = "192.168.1.140", 2323
CHUNK = int(sys.argv[1]); GAP = float(sys.argv[2]); TOTAL = int(sys.argv[3])
srv = socket.socket(); srv.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
srv.bind(("0.0.0.0", PORT_TCP)); srv.listen(1); srv.settimeout(1.0)
st = {"rx": b"", "stop": False}
def serve():
    while not st["stop"]:
        try: c, a = srv.accept()
        except socket.timeout: continue
        c.settimeout(0.2)
        while not st["stop"]:
            try:
                d = c.recv(4096)
                if not d: break
                st["rx"] += d
            except socket.timeout: continue
            except OSError: break
        c.close()
threading.Thread(target=serve, daemon=True).start()
m = M(115200); m.sync()
m.s.reset_input_buffer()
m.s.write(("ATD%s:%d\r" % (PC_IP, PORT_TCP)).encode()); m.s.flush()
resp = m.drain(6.0); print("dial:", repr(resp[-20:]))
pat = b"ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789"
data = (pat * (TOTAL // len(pat) + 1))[:TOTAL]
m.s.reset_input_buffer(); got = b""; t0 = time.time(); sent = 0; last = t0
while (sent < TOTAL or len(st["rx"]) < TOTAL) and time.time() - t0 < 40:
    if sent < TOTAL:
        m.s.write(data[sent:sent+CHUNK]); sent += CHUNK
        tn = time.time() + GAP
        while time.time() < tn:
            d = m.s.read(4096)
            if d: got += d; last = time.time()
    else:
        d = m.s.read(4096)
        if d: got += d; last = time.time()
        if time.time() - last > 5 or len(st["rx"]) >= TOTAL: break
print("chunk=%d gap=%.3fs total=%d: sent=%d server_rx=%d echoed=%d exact=%s time=%.1fs" %
      (CHUNK, GAP, TOTAL, min(sent,TOTAL), len(st["rx"]), len(got), got == data, time.time() - t0))
st["stop"] = True; time.sleep(0.4)
m.close(); srv.close()
