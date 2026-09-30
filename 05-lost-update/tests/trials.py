#!/usr/bin/env python3
"""Run a counter binary N times, recording the result AND the elapsed time of each run.

    python3 tests/trials.py <N> <file under bench/> <binary> [window width]

Both numbers come from the same runs on purpose: the loss figures and the cost
figures are then measurements of one experiment rather than two, and their
spreads are directly comparable.

The output file is an argument so that no step can overwrite an earlier step's
evidence: `make trials`, `make trials-intervention` and `make trials-fixed` each
write their own file. The `spin` column records the window width the binary was
built with, so the intervention's file says which change it measured.

Timing lives here rather than in the Makefile because `date` has no portable
sub-second format across macOS and Linux.
"""
import csv, re, subprocess, sys, time
from pathlib import Path


def own_spin(root: Path) -> str:
    m = re.search(r"#define\s+A5_SPIN\s+(\d+)", (root / "src" / "config.h").read_text())
    return m.group(1) if m else "?"


def main() -> int:
    if len(sys.argv) < 4:
        print(__doc__.strip().splitlines()[2].strip(), file=sys.stderr)
        return 2
    n, out_name, binary_name = int(sys.argv[1]), sys.argv[2], sys.argv[3]
    root = Path(__file__).resolve().parent.parent
    spin = sys.argv[4] if len(sys.argv) > 4 else own_spin(root)
    binary = root / binary_name
    if not binary.is_file():
        print(f"build first: {binary_name} does not exist", file=sys.stderr)
        return 2
    (root / "bench").mkdir(exist_ok=True)
    out = root / "bench" / out_name
    with out.open("w", newline="") as f:
        w = csv.writer(f, lineterminator="\n")
        w.writerow(["trial", "counter", "expected", "lost", "ms", "spin"])
        for i in range(1, n + 1):
            t0 = time.perf_counter()
            r = subprocess.run([str(binary)], capture_output=True, text=True)
            ms = (time.perf_counter() - t0) * 1000.0
            parts = (r.stdout or "").split()
            if len(parts) != 2:
                print(f"trial {i}: unexpected output {r.stdout!r}", file=sys.stderr)
                return 1
            got, exp = int(parts[0]), int(parts[1])
            w.writerow([i, got, exp, exp - got, f"{ms:.1f}", spin])
    print(f"wrote bench/{out_name} ({n} trials, window width {spin})")
    return 0


if __name__ == "__main__":
    sys.exit(main())
