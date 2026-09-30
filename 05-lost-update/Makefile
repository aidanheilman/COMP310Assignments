# COMP 310 - Week 5 - A5: the lost update (STARTER)
CC     ?= cc
# -O0 is load-bearing, exactly as in the race demo (C companion 5.7). At -O2 the
# compiler may read the counter once before the loop and write it once after, so
# the per-increment read and write you are asked to locate would no longer exist
# in the program you measure.
CFLAGS ?= -std=c11 -Wall -Wextra -O0 -pthread
TRIALS ?= 30

.PHONY: all config trials trials-intervention trials-fixed detect summary test clean submit

SUSPECTS = tests/suspects/suspect-1.c tests/suspects/suspect-2.c

all: src/config.h counter $(SUSPECTS)

# Rebuilt whenever submission.json changes, so nothing is ever measured with a
# configuration - or suspects - that are not your netid's.
src/config.h $(SUSPECTS): submission.json tests/config.py
	@python3 tests/config.py >/dev/null

config:
	@python3 tests/config.py

counter: src/counter.c src/config.h
	$(CC) $(CFLAGS) -o counter src/counter.c

# One run is not a measurement; each target below writes 30 into its OWN file
# under bench/, so no later step can overwrite an earlier one's evidence.
trials: all                           # 2.2 - the unfixed program
	@python3 tests/trials.py $(TRIALS) trials.csv counter

trials-intervention: src/config.h     # 2.4 - still unfixed, another window width
	@[ -n "$(SPIN)" ] || { echo "usage: make trials-intervention SPIN=<width>"; exit 2; }
	$(CC) $(CFLAGS) -DA5_SPIN_OVERRIDE=$(SPIN) -o counter-intervention src/counter.c
	@python3 tests/trials.py $(TRIALS) trials-intervention.csv counter-intervention $(SPIN)

trials-fixed: all                     # 2.5 - after the fix
	@python3 tests/trials.py $(TRIALS) trials-fixed.csv counter

detect: $(SUSPECTS)                   # 2.6 - how often a suspect loses, at one stress width
	@[ -n "$(SUSPECT)" ] && [ -n "$(STRESS)" ] || { echo "usage: make detect SUSPECT=<1|2> STRESS=<width>"; exit 2; }
	@python3 tests/detect.py $(SUSPECT) $(STRESS) $(TRIALS)

summary:                              # median, spread and trimmed range of every bench/ file
	@python3 tests/summary.py

test: all
	@sh tests/run-tests.sh; rc=$$?; \
	 sh tests/check-my-check.sh; \
	 if command -v python3 >/dev/null 2>&1 && [ -f tests/check_submission_ready.py ]; then \
	   echo; python3 tests/check_submission_ready.py || true; \
	 fi; \
	 exit $$rc

clean:
	@rm -f counter counter-* src/config.h $(SUSPECTS)

submit:
	@sh tests/make-submission.sh
