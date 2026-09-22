#!/bin/sh
# COMP 310 - Week 4 - checks for the job runner.
#
# These check behavior that must hold on any machine, not speed. Every run is
# bounded by `timeout` or a polling limit, so a runner that hangs FAILS a check
# instead of freezing the suite.
#
# Ctrl-C is simulated exactly: the runner is started as the leader of its own
# process group (tests/newpgrp.c), and `kill -s INT -- -PID` sends SIGINT to
# the whole group, which is what the terminal does when you press Ctrl-C.
set -u
pass=0; fail=0
ok()  { pass=$((pass+1)); printf '  PASS: %s\n' "$1"; }
bad() { fail=$((fail+1)); printf '  FAIL: %s\n' "$1"; }

T=$(mktemp -d)
RUNNER_PID=

# Background jobs outlive the runner by design. Kill every one a check
# started, so the suite leaves nothing running behind it.
kill_bg_jobs() {
  for f in "$T"/*.out; do
    [ -f "$f" ] || continue
    for p in $(sed -n 's/^\[bg\] \([0-9][0-9]*\)$/\1/p' "$f"); do
      kill -s KILL "$p" 2>/dev/null
    done
  done
}
cleanup() {
  [ -n "$RUNNER_PID" ] && kill -s KILL -- "-$RUNNER_PID" 2>/dev/null
  kill_bg_jobs
  rm -rf "$T"
}
trap cleanup EXIT

if ! ${CC:-cc} -std=c11 -o "$T/newpgrp" tests/newpgrp.c 2>"$T/cc.log"; then
  echo "could not build the test helper tests/newpgrp.c:"; cat "$T/cc.log"
  exit 1
fi

# alive PID: the process exists and is not a zombie.
alive() {
  s=$(ps -o stat= -p "$1" 2>/dev/null | tr -d ' ')
  [ -n "$s" ] || return 1
  case "$s" in Z*) return 1;; esac
  return 0
}

# zombies_of PID: how many zombie children PID has right now.
zombies_of() {
  ps -A -o ppid= -o stat= 2>/dev/null \
    | awk -v p="$1" '$1 == p && $2 ~ /^Z/ { n++ } END { print n + 0 }'
}

# A session keeps the runner alive while the script feeds it lines one at a
# time, so there is time to send a signal between them.
start_session() {                      # start_session NAME
  S="$T/$1"
  rm -f "$S.in"; mkfifo "$S.in"
  "$T/newpgrp" ./runner < "$S.in" > "$S.out" 2>&1 &
  RUNNER_PID=$!
  exec 3> "$S.in"                      # hold stdin open: no EOF until we close it
}
# If the runner has died, nothing reads the FIFO, and a write to it raises
# SIGPIPE, which would kill this script. Write from a subshell that ignores it.
send() { ( trap '' PIPE; printf '%s\n' "$1" >&3 ) 2>/dev/null; }
# wait_for PATTERN SECONDS: poll the session output until PATTERN appears.
wait_for() {
  i=0
  while [ $i -lt $(($2 * 10)) ]; do
    grep -q "$1" "$S.out" 2>/dev/null && return 0
    sleep 0.1; i=$((i+1))
  done
  return 1
}
end_session() {
  exec 3>&-                            # EOF: the runner leaves its loop
  i=0
  while alive "$RUNNER_PID" && [ $i -lt 30 ]; do sleep 0.1; i=$((i+1)); done
  kill -s KILL -- "-$RUNNER_PID" 2>/dev/null
  wait "$RUNNER_PID" 2>/dev/null
  RUNNER_PID=
}
bg_pids() { sed -n 's/^\[bg\] \([0-9][0-9]*\)$/\1/p' "$S.out"; }

# ---------------------------------------------------------------------------
echo "== the runner still runs commands =="

printf 'echo hello\nexit\n' | timeout 5 ./runner > "$T/plain.out" 2>&1
grep -q '^hello$' "$T/plain.out" && ok "runs an external command" \
  || bad "an external command did not run"

printf 'no-such-command-xyz\nexit\n' | timeout 5 ./runner > "$T/err.out" 2>&1
grep -qi 'no such file\|not found' "$T/err.out" && ok "an unknown command reports why" \
  || bad "an unknown command was silent"

printf 'sleep 1\necho first\n' > "$T/slow.sh"
printf "sh $T/slow.sh\necho second\nexit\n" | timeout 10 ./runner > "$T/fg.out" 2>&1
got=$(grep -v '^\[done\] ' "$T/fg.out" | tr '\n' ' ')
[ "$got" = "first second " ] && ok "a foreground command is waited for" \
  || bad "a foreground command was not waited for (output: $got)"

# run_foreground blocks SIGCHLD while it waits. Without that, on_sigchld can
# reap the foreground child first and report it as a finished background job.
# Whether that race is actually lost depends on the kernel: macOS loses it
# readily, Linux rarely. A pass here on Linux is not proof the block is there.
grep -q '^\[done\] ' "$T/fg.out" \
  && bad "a foreground command was reported as '[done]' - the SIGCHLD handler reaped it; is SIGCHLD blocked while the shell waits?" \
  || ok "a foreground command is not reported as '[done]'"

# ---------------------------------------------------------------------------
echo
echo "== background jobs: cmd & =="

start=$(date +%s)
printf 'sleep 4 &\necho quick\nexit\n' | timeout 10 ./runner > "$T/bg.out" 2>&1
elapsed=$(( $(date +%s) - start ))
if grep -q '^quick$' "$T/bg.out" && [ "$elapsed" -le 2 ]; then
  ok "'sleep 4 &' returns at once"
else
  bad "'sleep 4 &' held the shell for ${elapsed}s - the shell must not wait for it (nor block in a handler)"
fi
grep -Eq '^\[bg\] [0-9]+$' "$T/bg.out" && ok "prints '[bg] <pid>' for a background job" \
  || bad "no '[bg] <pid>' line"
kill_bg_jobs

# ---------------------------------------------------------------------------
echo
echo "== reaping: the SIGCHLD handler =="

start_session reap
send 'sleep 0.3 &'
sleep 1.5
BG=$(bg_pids | head -1)
Z=$(zombies_of "$RUNNER_PID")
if [ -z "$BG" ]; then
  bad "a finished background job is reported: no background job was started"
  bad "a finished background job leaves no zombie: no background job was started"
else
  grep -q "^\[done\] $BG\$" "$S.out" \
    && ok "a finished background job is reported as '[done] <pid>'" \
    || bad "no '[done] $BG' line - is the SIGCHLD handler installed, and does it write with write() rather than printf()?"
  [ "$Z" -eq 0 ] && ok "a finished background job leaves no zombie" \
    || bad "$Z zombie(s) left behind - the job was never reaped"
fi
end_session

# Four jobs finish while the shell waits for a foreground command. SIGCHLD is
# blocked during that wait, so the four signals collapse into ONE pending
# SIGCHLD. A handler that calls waitpid once reaps one job, not four.
start_session coalesce
send 'sleep 0.2 &'; send 'sleep 0.2 &'; send 'sleep 0.2 &'; send 'sleep 0.2 &'
send 'sleep 2'
send 'echo check'
wait_for '^check$' 10
sleep 0.5
N_BG=$(bg_pids | wc -l | tr -d ' ')
N_DONE=$(grep -c '^\[done\] ' "$S.out")
Z=$(zombies_of "$RUNNER_PID")
if [ "$N_BG" -ne 4 ]; then
  bad "four jobs finishing together are all reaped: $N_BG of 4 started in the background"
elif [ "$N_DONE" -eq 4 ] && [ "$Z" -eq 0 ]; then
  ok "four jobs finishing together are all reaped"
elif [ "$N_DONE" -gt 4 ]; then
  bad "four jobs finishing together: $N_DONE '[done]' lines for 4 jobs - foreground commands are being reaped by the handler"
else
  bad "four jobs finished together: $N_DONE reaped, $Z zombie(s) - reap in a loop, not once"
fi
end_session

# ---------------------------------------------------------------------------
echo
echo "== Ctrl-C: SIGINT to the foreground process group =="

start_session ctrlc
send 'sleep 30 &'
wait_for '^\[bg\] ' 3
BG=$(bg_pids | head -1)
send 'sleep 5'
sleep 1
kill -s INT -- "-$RUNNER_PID" 2>/dev/null     # what Ctrl-C does
t0=$(date +%s)
send 'echo alive'
if wait_for '^alive$' 8; then
  ok "the shell survives Ctrl-C"
  took=$(( $(date +%s) - t0 ))
  [ "$took" -le 2 ] && ok "Ctrl-C ends the foreground command" \
    || bad "Ctrl-C did not end the foreground command (it ran on for ${took}s) - is SIGINT ignored instead of handled?"
else
  bad "the shell survives Ctrl-C - the runner died, or stopped responding"
  bad "Ctrl-C ends the foreground command - cannot tell, the runner is not responding"
fi
if [ -z "$BG" ]; then
  bad "a background job survives Ctrl-C: no background job was started"
elif alive "$BG"; then
  ok "a background job survives Ctrl-C"
else
  bad "a background job was killed by Ctrl-C - it is still in the terminal's process group"
fi
end_session

echo
echo "== $pass passed, $fail failed =="
[ $fail -eq 0 ]
