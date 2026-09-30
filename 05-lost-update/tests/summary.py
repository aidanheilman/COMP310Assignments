#!/usr/bin/env python3
"""Summarize every measurement under bench/ (`make summary`).

For each trials file: the median and spread of the lost updates, and the median,
range and TRIMMED range of the elapsed time. The median of an even number of
values is the mean of the two middle ones. "Trimmed" means the single fastest and
the single slowest trial are dropped first (handout section 2.5, step 1).

It reports numbers; it does not apply the section 2.5 rule for you.
"""
import csv, sys
from pathlib import Path


def median(xs):
    s = sorted(xs)
    n = len(s)
    return s[n // 2] if n % 2 else (s[n // 2 - 1] + s[n // 2]) / 2


def fmt(x):
    """A median exactly as computed: 348978.5 stays 348978.5."""
    return str(int(x)) if x == int(x) else f"{x:.2f}".rstrip("0")


def trials(path: Path) -> None:
    rows = list(csv.DictReader(path.open()))
    if not rows:
        print(f"{path.name}: empty")
        return
    lost = [int(r["lost"]) for r in rows]
    ms = [float(r["ms"]) for r in rows]
    spins = sorted({r.get("spin", "?") for r in rows})
    exp = sorted({r["expected"] for r in rows})
    print(f"{path.name}: {len(rows)} trials, window width {', '.join(spins)}, "
          f"expected total {', '.join(exp)}")
    print(f"  lost updates   median {fmt(median(lost))}   lowest {min(lost)}   "
          f"highest {max(lost)}   runs with no loss {sum(1 for x in lost if x == 0)}")
    print(f"  elapsed ms     median {fmt(median(ms))}   range {min(ms):g} - {max(ms):g}")
    if len(ms) > 2:
        t = sorted(ms)[1:-1]
        print(f"  trimmed ms     median {fmt(median(t))}   range {t[0]:g} - {t[-1]:g}   "
              f"(dropped {min(ms):g} and {max(ms):g})")


def detect(path: Path) -> None:
    rows = list(csv.DictReader(path.open()))
    print(f"{path.name}: {len(rows)} measurement(s)")
    for r in rows:
        print(f"  suspect {r['suspect']}  stress {r['stress']:>9}  "
              f"lost in {r['lost_runs']} of {r['runs']} runs  (worst {r['max_lost']})")


def main() -> int:
    bench = Path(__file__).resolve().parent.parent / "bench"
    files = sorted(bench.glob("*.csv")) if bench.is_dir() else []
    if not files:
        print("nothing under bench/ yet - run `make trials` first")
        return 0
    for p in files:
        (detect if p.name == "detect.csv" else trials)(p)
        print()
    return 0


if __name__ == "__main__":
    sys.exit(main())
