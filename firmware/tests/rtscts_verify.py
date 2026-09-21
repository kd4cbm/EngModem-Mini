import serial, time, threading, sys

PORT, BAUD = "COM15", 115200
results = []

def check(name, ok, detail=""):
    results.append((name, ok))
    print("%-58s %s %s" % (name, "PASS" if ok else "FAIL", detail))

s = serial.Serial()
s.port = PORT; s.baudrate = BAUD; s.bytesize = 8; s.parity = "N"; s.stopbits = 1
s.timeout = 0.1; s.rtscts = False; s.dsrdtr = False; s.xonxoff = False
s.open()
s.dtr = True
s.rts = True

def drain(seconds):
    t0 = time.time(); got = b""
    while time.time() - t0 < seconds:
        d = s.read(512)
        if d: got += d
    return got

def send(cmd, wait=1.0):
    s.reset_input_buffer()
    s.write(cmd + b"\r"); s.flush()
    return drain(wait)

print("waiting 8 s for the board to finish booting / joining WiFi...")
time.sleep(8)
s.reset_input_buffer()

# --- T0: modem-driven lines, flow control off -------------------------------
time.sleep(0.3)
print("idle lines (PC RTS asserted): CTS=%s DSR=%s RI=%s CD=%s" % (s.cts, s.dsr, s.ri, s.cd))
check("T0 idle: CTS asserted, DSR asserted, RI/CD off", s.cts and s.dsr and not s.ri and not s.cd)
r = send(b"AT")
check("T0 AT -> OK at 115200", b"OK" in r, repr(r))

# --- T1: with flow control OFF the modem must ignore PC RTS ------------------
s.rts = False; time.sleep(0.3)
r = send(b"AT")
check("T1 &K0: PC RTS low, modem still answers", b"OK" in r, repr(r))
s.rts = True; time.sleep(0.2)

# --- T2: enable RTS/CTS -------------------------------------------------------
r = send(b"AT&K3")
check("T2 AT&K3 accepted (RTS asserted)", b"OK" in r, repr(r))
r = send(b"AT")
check("T2 AT still answers with RTS asserted", b"OK" in r, repr(r))

# --- T3: PC RTS low must gate the modem's transmit ---------------------------
for cmd, label in ((b"AT", "short reply"), (b"ATI", "long reply")):
    s.rts = False; time.sleep(0.3)
    s.reset_input_buffer()
    s.write(cmd + b"\r"); s.flush()
    held = drain(1.5)
    check("T3 %-5s RTS low: modem holds its output (%s)" % (cmd.decode(), label), len(held) == 0, "got %d bytes %r" % (len(held), held[:40]))
    s.rts = True
    released = drain(1.5)
    check("T3 %-5s RTS high: held output is released" % cmd.decode(), b"OK" in released, "got %d bytes" % len(released))

# --- T4: modem's CTS output must drop when its RX buffer fills ---------------
cts_samples = []
stop = False
def sampler():
    while not stop:
        cts_samples.append((time.time(), s.cts))
        time.sleep(0.002)
th = threading.Thread(target=sampler, daemon=True)
s.reset_input_buffer()
s.write(b"ATW\r"); s.flush()          # WiFi scan blocks the modem's loop for a few seconds
time.sleep(0.05)
th.start()
t0 = time.time()
sent = 0
while time.time() - t0 < 3.0:         # keep the line flooded; modem cannot drain while scanning
    s.write(b"A" * 512)
    sent += 512
    time.sleep(0.03)
time.sleep(0.5)
stop = True; th.join(1)
vals = [v for _, v in cts_samples]
dropped = (False in vals)
check("T4 modem CTS drops when its RX buffer fills", dropped,
      "(sent %d bytes, %d samples, %d low)" % (sent, len(vals), vals.count(False)))
if not dropped:
    print("   note: inconclusive if the modem loop kept draining during the scan; not proof of failure")

# let it recover, then check the link is healthy again
drain(6)
for _ in range(3):
    s.write(b"\r"); time.sleep(0.2)
drain(1)
r = send(b"AT")
check("T4 link recovers after the flood", b"OK" in r, repr(r))
time.sleep(0.5)
check("T4 CTS asserted again once drained", s.cts)

# --- restore: flow control off (not saved) -----------------------------------
s.rts = True
r = send(b"AT&K0")
check("T5 AT&K0 restores no flow control", b"OK" in r, repr(r))
r = send(b"AT")
check("T5 final AT -> OK", b"OK" in r, repr(r))

s.close()
bad = [n for n, ok in results if not ok]
print("\n%d checks, %d failed" % (len(results), len(bad)))
for n in bad: print("  FAILED:", n)
