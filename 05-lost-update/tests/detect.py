#!/usr/bin/env python3
"""How often does a suspect lose updates, at one stress width? (handout section 2.6)

    make detect SUSPECT=<1|2> STRESS=<width>        # 30 runs
    python3 tests/detect.py <1|2> <width> [runs]

Builds tests/suspects/suspect-<N>.c with -DA5_STRESS=<width>, runs it, and adds
one row to bench/detect.csv:

    suspect,stress,runs,lost_runs,max_lost

`lost_runs` is how many of the runs lost anything. Rows are appended, never
replaced, so every width you tried stays on record.
"""
import csv, os, subprocess, sys, tempfile
from pathlib import Path


def main() -> int:
    if len(sys.argv) < 3 or sys.argv[1] not in ("1", "2") or not sys.argv[2].isdigit():
        print("usage: make detect SUSPECT=<1|2> STRESS=<width>", file=sys.stderr)
        return 2
    suspect, stress = sys.argv[1], int(sys.argv[2])
    runs = int(sys.argv[3]) if len(sys.argv) > 3 else 30
    root = Path(__file__).resolve().parent.parent
    src = root / "tests" / "suspects" / f"suspect-{suspect}.c"
    cc = os.environ.get("CC", "cc")
    with tempfile.TemporaryDirectory() as tmp:
        exe = Path(tmp) / "suspect"
        b = subprocess.run([cc, "-std=c11", "-Wall", "-Wextra", "-O0", "-pthread",
                            f"-DA5_STRESS={stress}", "-o", str(exe), str(src)],
                           capture_output=True, text=True)
        if b.returncode != 0:
            print(b.stderr, file=sys.stderr)
            return 1
        lost_runs, max_lost = 0, 0
        for _ in range(runs):
            parts = subprocess.run([str(exe)], capture_output=True, text=True).stdout.split()
            if len(parts) != 2:
                print(f"unexpected output {parts!r}", file=sys.stderr)
                return 1
            lost = int(parts[1]) - int(parts[0])
            lost_runs += lost > 0
            max_lost = max(max_lost, lost)
    (root / "bench").mkdir(exist_ok=True)
    out = root / "bench" / "detect.csv"
    new = not out.exists()
    with out.open("a", newline="") as f:
        w = csv.writer(f, lineterminator="\n")
        if new:
            w.writerow(["suspect", "stress", "runs", "lost_runs", "max_lost"])
        w.writerow([suspect, stress, runs, lost_runs, max_lost])
    print(f"suspect {suspect} at stress {stress}: lost updates in {lost_runs} of {runs} runs "
          f"(worst run lost {max_lost}) -> bench/detect.csv")
    return 0


if __name__ == "__main__":
    sys.exit(main())
