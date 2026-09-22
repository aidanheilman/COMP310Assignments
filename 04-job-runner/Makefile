# COMP 310 — Week 4 — job runner (STARTER)
CC     ?= gcc
CFLAGS ?= -std=c11 -Wall -Wextra -O2
.PHONY: all test clean submit
all: runner
runner: src/runner.c
	$(CC) $(CFLAGS) -o runner src/runner.c
# `make test` runs BOTH your code's checks and the submission-spec
# check. Work that does not meet its stated interface is not finished,
# so both must pass.
test: all
	@CC="$(CC)" sh tests/run-tests.sh
	@sh tests/check-submission-spec.sh
	@python3 tests/check_submission_ready.py || true
clean:
	@rm -f runner

# `make submit` builds the zip you upload. It names the file, lays it out under
# submission/, and keeps your scripts executable. It ALWAYS builds - unfinished
# work is reported as a warning, not a refusal.
submit:
	@sh tests/make-submission.sh
