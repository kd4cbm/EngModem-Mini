import serial, time, threading

import os
PORT = os.environ.get("QA_PORT", "COM15")      # e.g. set QA_PORT=COM1 to test a different port
results = []

def check(name, ok, detail=""):
    results.append((name, ok))
    print("%-62s %s %s" % (name, "PASS" if ok else "FAIL", detail), flush=True)

def summary():
    bad = [n for n, ok in results if not ok]
    print("\n%d checks, %d failed" % (len(results), len(bad)))
    for n in bad:
        print("  FAILED:", n)

class M:
    def __init__(self, baud=115200):
        s = serial.Serial()
        s.port = PORT; s.baudrate = baud; s.bytesize = 8; s.parity = "N"; s.stopbits = 1
        s.timeout = 0.05; s.rtscts = False; s.dsrdtr = False; s.xonxoff = False
        s.open()
        s.dtr = True
        s.rts = True
        self.s = s
        self.baud = baud

    def close(self):
        self.s.rts = True
        self.s.close()

    def drain(self, secs):
        t0 = time.time(); got = b""
        while time.time() - t0 < secs:
            d = self.s.read(4096)
            if d:
                got += d
        return got

    def send(self, cmd, wait=1.0):
        self.s.reset_input_buffer()
        self.s.write(cmd + b"\r")
        self.s.flush()
        return self.drain(wait)

    def set_baud(self, b):
        self.s.baudrate = b
        self.baud = b

    def modem_baud(self, new):
        """Tell the modem to change baud, then follow it with the PC port."""
        # Make sure the PC port accepts the rate BEFORE the modem is told to switch, so an
        # unsupported rate can never strand the modem at a speed the PC cannot follow.
        old = self.s.baudrate
        try:
            self.s.baudrate = new
        except Exception as e:
            try: self.s.baudrate = old
            except Exception: pass
            raise RuntimeError("PC port %s does not accept %d baud (%s)" % (self.s.port, new, str(e)[:60]))
        self.s.baudrate = old
        self.s.reset_input_buffer()
        self.s.write(("ATB%d\r" % new).encode())
        self.s.flush()
        time.sleep(0.5 + 200.0 / self.baud)
        self.set_baud(new)
        time.sleep(0.3)
        self.s.reset_input_buffer()

    def sync(self, tries=5):
        for _ in range(tries):
            self.s.write(b"\r")
            self.drain(0.15 + 100.0 / self.baud)
            r = self.send(b"AT", 0.6 + 200.0 / self.baud)
            if b"OK" in r:
                return True
        return False
