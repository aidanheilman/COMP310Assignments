# COMP 310 — Operating Systems
## Week 5 — A5: The lost update

## Build

```bash
make config     # the configuration your netid derives
make            # build
```

## Run

```bash
make trials                        # 2.2: 30 trials -> bench/trials.csv
make trials-intervention SPIN=<n>  # 2.4: 30 trials at another width
make trials-fixed                  # 2.5: 30 trials after the fix
make summary                       # medians, spreads, trimmed ranges
make test                          # the 9 course checks, then your own check
```

## File map

| Path | What it is |
|---|---|
| `src/counter.c` | the counter; the three TODOs are yours |
| `src/config.h` | generated from your netid by `make` — do not edit |
| `bench/` | every measurement, one file per step |
| `REPORT.md` | the write-up; `## Prediction` comes first, before results |
| `tests/run-tests.sh` | the 9 checks `make test` runs |
| `tests/suspects/` | two proposed fixes for section 2.6, generated from the netid by `make` |
| `tests/my-check.sh` | your own check for section 2.6 |

## Notes

*Anything a reader should know: what you changed, what you could not get
working, what you would do next.*
