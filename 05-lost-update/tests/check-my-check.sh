#!/bin/sh
# COMP 310 - Week 5 - A5 - does tests/my-check.sh tell the two suspects apart?
#
# Run by `make test` after the nine course checks. It never changes whether
# `make test` passes: section 2.6 is the 3.0 challenge, and 2.5 does not need it.
#
# It reports what your check said about each suspect, and requires that it
# passes one and fails the other. It does NOT know which suspect is the broken
# one - establishing that, from the code and from `make detect`, is what
# section 2.6 asks of you. When your work is read, your check is also run on
# programs you have not seen, and the broken suspect must be the one it fails.
#
# Exits 0 when your check passes exactly one suspect and fails the other, 1 when
# it does not, and 3 when my-check.sh has not been written yet.
set -u
echo
echo "== the 3.0 challenge: your own check (section 2.6) =="
python3 tests/config.py >/dev/null 2>&1       # the suspects, for this netid
r1=0; sh tests/my-check.sh tests/suspects/suspect-1.c >/dev/null 2>&1 || r1=$?
r2=0; sh tests/my-check.sh tests/suspects/suspect-2.c >/dev/null 2>&1 || r2=$?
if [ "$r1" -eq 2 ] && [ "$r2" -eq 2 ]; then
  echo "  not attempted: tests/my-check.sh exits 2 on both suspects"
  exit 3
fi
say() { case "$1" in 0) echo "passed it";; 1) echo "failed it";; *) echo "exited $1";; esac; }
echo "  suspect 1: your check $(say "$r1")"
echo "  suspect 2: your check $(say "$r2")"
if { [ "$r1" -eq 0 ] && [ "$r2" -eq 1 ]; } || { [ "$r1" -eq 1 ] && [ "$r2" -eq 0 ]; }; then
  if [ "$r1" -eq 1 ]; then bad=1; else bad=2; fi
  echo "  PASS: your check tells them apart - it fails suspect $bad."
  echo "        Make sure suspect $bad is the one your REPORT shows is broken."
  exit 0
fi
echo "  FAIL: a check has to pass one suspect and fail the other, every time"
echo "        you run it"
exit 1
