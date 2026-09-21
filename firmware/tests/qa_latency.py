import socket, threading, time, statistics
from qa_common import *
PC_IP, PORT_TCP = "192.168.1.140", 2323
srv = socket.socket(); srv.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
srv.bind(("0.0.0.0", PORT_TCP)); srv.listen(1); srv.settimeout(1.0)
st = {"stop": False}
def serve():
    while not st["stop"]:
        try: c, a = srv.accept()
        except socket.timeout: continue
        c.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, 1); c.settimeout(0.2)
        while not st["stop"]:
            try:
                d = c.recv(4096)
                if not d: break
                c.sendall(d)
            except socket.timeout: continue
            except OSError: break
        c.close()
threading.Thread(target=serve, daemon=True).start()
m = M(115200); m.sync()
m.s.reset_input_buffer()
m.s.write(("ATD%s:%d\r" % (PC_IP, PORT_TCP)).encode()); m.s.flush()
print("dial:", repr(m.drain(6.0)[-16:]))
lat = []
for i in range(60):
    m.s.reset_input_buffer()
    t = time.perf_counter(); m.s.write(b"K"); m.s.flush()
    while time.perf_counter() - t < 2:
        if m.s.read(1) == b"K": lat.append((time.perf_counter() - t) * 1000); break
    time.sleep(0.05)
print("keystroke round trip via TCP echo: n=%d  median %.1f ms  p95 %.1f ms  max %.1f ms" %
      (len(lat), statistics.median(lat), sorted(lat)[int(len(lat)*.95)-1], max(lat)))
time.sleep(1.5); m.s.write(b"+++"); m.s.flush(); time.sleep(1.5); m.drain(1.0)
m.send(b"ATH", 2.0); print("post-hangup sync:", m.sync(), "CD=", m.s.cd)
st["stop"] = True; time.sleep(0.4); m.close(); srv.close()
