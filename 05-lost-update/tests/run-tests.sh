#!/bin/sh
# COMP 310 - Week 5 - A5 - the nine course checks.
#
# Two pass once your netid is set; the other seven are the assignment. Before you
# set it, the netid check fails first and all nine report failed.
#
# Check 9 is the one the fix is judged by: it runs YOUR finished program 30 times
# and requires zero lost updates, against the total YOUR netid derives.
set -u
pass=0; fail=0
ok()  { pass=$((pass+1)); printf '  PASS: %s\n' "$1"; }
bad() { fail=$((fail+1)); printf '  FAIL: %s\n' "$1"; }
R=$(python3 tests/config.py 2>/dev/null) || { echo "  FAIL: config.py did not run"; exit 1; }
# The placeholder builds, but it must not pass: every figure reported below is
# checked against the configuration your own netid derives.
if grep -q '"student"[[:space:]]*:[[:space:]]*"yournetid"' submission.json 2>/dev/null; then
  echo "  FAIL: submission.json still says \"yournetid\" - set your netid first"
  echo "== 0 passed, 9 failed =="
  exit 1
fi
THREADS=$(echo "$R" | awk '/^threads:/    {print $2}')
ITERS=$(echo "$R"   | awk '/^iterations:/ {print $2}')
SPIN=$(echo "$R"    | awk '/^spin:/       {print $2}')
EXPECT=$((THREADS * ITERS))
# config.py has just rewritten src/config.h; rebuild so ./counter is that program.
make -s counter >/dev/null 2>&1

echo "== it builds and runs =="
[ -x ./counter ] && ok "counter built" || bad "counter did not build"
out=$(./counter 2>/dev/null); prints=$(echo "$out" | cut -d' ' -f2)
[ "$prints" = "$EXPECT" ] && ok "it prints a total, and expects $EXPECT, your netid's $THREADS x $ITERS" \
  || bad "it expects '${prints:-nothing}', but your netid derives $THREADS x $ITERS = $EXPECT - run make"

echo
echo "== you measured the race (sections 2.2 and 2.4) =="
if [ -f bench/trials.csv ]; then
  rows=$(tail -n +2 bench/trials.csv | grep -c ',')
  other=$(tail -n +2 bench/trials.csv | cut -d, -f3 | grep -vc "^$EXPECT\$")
  if [ "$rows" -ge 30 ] && [ "$other" -eq 0 ]; then
    ok "bench/trials.csv holds $rows trials of your own configuration"
  else
    bad "bench/trials.csv: $rows trials, $other of them not expecting $EXPECT - re-run make trials"
  fi
  # A trials file where nothing was ever lost is not a measurement of a race.
  worst=$(tail -n +2 bench/trials.csv | cut -d, -f4 | sort -n | tail -1)
  [ -n "$worst" ] && [ "$worst" -gt 0 ] && ok "the recorded trials show lost updates" \
    || bad "no trial in bench/trials.csv lost anything - that is not the unfixed program"
else
  bad "no bench/trials.csv - run make trials before fixing"
  bad "no trials to check for lost updates"
fi
if [ -f bench/trials-intervention.csv ]; then
  rows=$(tail -n +2 bench/trials-intervention.csv | grep -c ',')
  w=$(tail -n +2 bench/trials-intervention.csv | tr -d '\r' | cut -d, -f6 | sort -u)
  if [ "$rows" -ge 30 ] && [ -n "$w" ] && [ "$w" != "$SPIN" ] && [ "$(echo "$w" | wc -l)" -eq 1 ]; then
    ok "bench/trials-intervention.csv holds $rows trials at window width $w (yours is $SPIN)"
  else
    bad "bench/trials-intervention.csv needs 30 trials at ONE width other than your $SPIN"
  fi
else
  bad "no bench/trials-intervention.csv - run make trials-intervention SPIN=<width> before fixing"
fi

echo
echo "== you wrote it up (the Answers block) =="
if [ -f REPORT.md ]; then
  miss=""
  for k in threads iterations lost_median read_line write_line intervention_spin fixed_lost_max; do
    # A key present but empty is the skeleton, not an answer.
    grep -qE "^$k: *[^ <]" REPORT.md || miss="$miss $k"
  done
  [ -z "$miss" ] && ok "## Answers has all seven keys of sections 2.1-2.5" || bad "## Answers missing:$miss"
  a_thr=$(awk -F': *' '/^threads:/ {print $2}' REPORT.md | tr -d ' ')
  a_it=$(awk -F': *'  '/^iterations:/ {print $2}' REPORT.md | tr -d ' ')
  if [ "$a_thr" = "$THREADS" ] && [ "$a_it" = "$ITERS" ]; then
    ok "reported config matches the one your netid derives"
  else
    bad "Answers say threads=$a_thr iterations=$a_it; your netid derives $THREADS/$ITERS"
  fi
  a_med=$(awk -F': *' '/^lost_median:/ {print $2}' REPORT.md | tr -d ' ,')
  csv_med=$(python3 tests/summary.py 2>/dev/null | awk '/^trials.csv:/ {f=1} f && /lost updates/ {print $4; exit}')
  if [ -n "$csv_med" ] && python3 -c "import sys; sys.exit(0 if abs(float('$a_med') - float('$csv_med')) <= 1 else 1)" 2>/dev/null; then
    ok "lost_median matches bench/trials.csv ($csv_med)"
  else
    bad "lost_median is '${a_med}', but make summary gives ${csv_med:-nothing} for bench/trials.csv"
  fi
else
  bad "no REPORT.md"; bad "no REPORT.md"; bad "no REPORT.md"
fi

echo
echo "== the fix holds under repetition (section 2.5) =="
# 30 runs, not one, and against YOUR total, not whatever the binary prints.
worst=0; i=1
while [ $i -le 30 ]; do
  out=$(./counter 2>/dev/null); got=$(echo "$out" | cut -d' ' -f1)
  [ -z "$got" ] && got=0
  lost=$((EXPECT - got)); [ "$lost" -lt 0 ] && lost=$((0 - lost))
  [ "$lost" -gt "$worst" ] && worst=$lost
  i=$((i + 1))
done
[ "$worst" -eq 0 ] && ok "30 runs, zero lost updates" \
  || bad "still losing updates: worst of 30 runs was off by $worst"

echo
echo "== $pass passed, $fail failed =="
[ "$fail" -eq 0 ]
