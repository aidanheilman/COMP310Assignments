#!/bin/sh
# YOUR check for handout section 2.6 (the 3.0 challenge). Optional for 2.5.
#
# CONTRACT
#   sh tests/my-check.sh <file.c>
#
#   Build <file.c> with -DA5_STRESS=<a width you chose and measured> and run it
#   as many times as you decided. Every program it is given prints
#   "<counter> <expected>", like src/counter.c and the two suspects.
#
#   Exit 0 if no run lost anything; exit 1 if any run did; exit 2 if the file
#   would not build. Finish within 60 seconds, and remove anything you create.
#
# `make test` runs it on both files in tests/suspects/. When your work is read it
# is run on your netid's two suspects under other names, and on other programs of
# the same shape you have not seen, so it has to decide by RUNNING the program,
# never by reading its source, its name or its suspect number.
echo "  my-check: not written yet (handout section 2.6)"
exit 2
