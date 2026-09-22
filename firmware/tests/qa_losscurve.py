# Loss-vs-size curve: sustained full-line-rate stream through a TCP connection with flow control OFF.
# Usage: QA_PORT=COM1 python qa_losscurve.py <label> <size:reps> [<size:reps> ...]
import subprocess, sys, re, os
label = sys.argv[1]
specs = [a.split(":") for a in sys.argv[2:]]
rows = []
env = dict(os.environ)
for size, reps in specs:
    for r in range(int(reps)):
        out = subprocess.run([sys.executable, "qa_stream2.py", "noflow", size, "115200"], capture_output=True, text=True, env=env).stdout
        m = re.search(r"sent (\d+), echoed (\d+), server got (\d+), ([\d.]+) s", out)
        if not m:
            print("%s size=%s rep=%d: NO RESULT LINE" % (label, size, r+1), flush=True); rows.append((int(size), None, None)); continue
        sent, echoed, got = int(m.group(1)), int(m.group(2)), int(m.group(3))
        print("%s size=%6d rep=%d: server got %6d (lost %5d = %4.1f%%)  echoed back %6d" %
              (label, sent, r+1, got, sent-got, 100.0*(sent-got)/sent, echoed), flush=True)
        rows.append((sent, got, echoed))
print("\nSUMMARY %s" % label)
for size, reps in specs:
    sel = [x for x in rows if x[0] == int(size) and x[1] is not None]
    if not sel: continue
    lost = [x[0]-x[1] for x in sel]
    print("  %6d B: %d runs, lost per run %s  (clean runs: %d/%d)" % (int(size), len(sel), lost, sum(1 for l in lost if l == 0), len(lost)))
