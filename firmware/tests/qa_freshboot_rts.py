# Fresh-boot regression test for ERRATA E13 (../../docs/ERRATA.md): resets the board via esptool,
# then - BEFORE anything else has ever touched AT&K - lowers PC RTS and checks whether the modem
# still answers with flow control at its default (OFF). As of firmware-v7-rev1 this step is EXPECTED
# TO FAIL (no response within the timeout) - that is the open issue, not a bug in this script. It
# exists so a future fix attempt has a repeatable way to prove it actually works on a genuinely
# virgin boot, which every earlier "14/14" rtscts_verify.py pass did not exercise: each one happened
# to run after some other test in the same boot session had already touched AT&K3 first.
import subprocess, time, os
import serial
QA_PORT = os.environ.get("QA_PORT", "COM15")
ESPTOOL_PORT = os.environ.get("ESPTOOL_PORT", "COM7")
results = []
def check(name, ok, detail=""):
    results.append((name, ok))
    print("%-58s %s %s" % (name, "PASS" if ok else "FAIL", detail), flush=True)

print("resetting the board (esptool run) for a genuinely fresh boot...", flush=True)
subprocess.run(["python", "-m", "esptool", "--chip", "esp32s3", "-p", ESPTOOL_PORT,
                 "--before", "default-reset", "--after", "hard-reset", "run"], capture_output=True)
time.sleep(9)   # boot + WiFi join, matching rtscts_verify.py's own wait

s = serial.Serial()
s.port = QA_PORT; s.baudrate = 115200; s.timeout = 0.05
s.rtscts = False; s.dsrdtr = False; s.xonxoff = False
s.open(); s.dtr = True; s.rts = True
time.sleep(0.3); s.reset_input_buffer()

def timed_at(label, timeout=6.0):
    s.reset_input_buffer()
    t0 = time.perf_counter()
    s.write(b"AT\r"); s.flush()
    got = b""
    while time.perf_counter() - t0 < timeout:
        d = s.read(64)
        if d:
            got += d
            if b"OK" in got: break
    dt = time.perf_counter() - t0
    check(label, b"OK" in got, "RTS=%s -> %.3fs %r" % (s.rts, dt, got))
    return dt

timed_at("fresh boot, RTS high (control, never touched AT&K)")
s.rts = False; time.sleep(0.15)
timed_at("fresh boot, RTS LOW, never touched AT&K yet (ERRATA E13 - expected FAIL)")
s.rts = True; time.sleep(0.15)

# now confirm K3/K0 still work normally, and RTS-low still answers after (the documented workaround)
s.write(b"AT&K3\r"); s.flush(); time.sleep(0.4)
k3 = s.read(64); check("AT&K3 accepted", b"OK" in k3, repr(k3))
s.write(b"AT&K0\r"); s.flush(); time.sleep(0.4)
k0 = s.read(64); check("AT&K0 accepted", b"OK" in k0, repr(k0))
s.rts = False; time.sleep(0.15)
timed_at("RTS low after a K3/K0 cycle (should also be fast - confirms the workaround)")
s.rts = True; time.sleep(0.15)

s.close()
bad = [n for n, ok in results if not ok]
print("\n%d checks, %d failed" % (len(results), len(bad)))
for n in bad: print("  FAILED:", n)
print("\nA failure on the 'RTS LOW, never touched AT&K yet' line alone is the known,")
print("documented ERRATA E13 behaviour, not a regression - see docs/ERRATA.md.")
