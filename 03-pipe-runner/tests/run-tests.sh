#!/bin/sh
# COMP 310 - Week 3 - soundness checks for the reference runner.
#
# These check behavior that must hold on any machine, not timings. The two
# that matter most are the LAST two: a pipeline must terminate, and it must not
# leak descriptors. Both are things a broken implementation gets wrong while
# still looking fine on a quick manual try.
set -u
pass=0; fail=0
ok()  { pass=$((pass+1)); printf '  PASS: %s\n' "$1"; }
bad() { fail=$((fail+1)); printf '  FAIL: %s\n' "$1"; }
T=$(mktemp -d)

echo "== plain commands still work =="
printf 'echo hello\nexit\n' | timeout 5 ./runner > "$T/plain" 2>&1
grep -q hello "$T/plain" && ok "runs an external command" || bad "plain command"

echo
echo "== redirection: cmd > file =="
printf "echo redirected > $T/out.txt\nexit\n" | timeout 5 ./runner >/dev/null 2>&1
[ -f "$T/out.txt" ] && ok "creates the file" || bad "no file created"
grep -q redirected "$T/out.txt" 2>/dev/null && ok "writes the output there" \
  || bad "wrong contents"

# The shell's OWN stdout must survive: dup2 belongs in the child.
printf "echo one > $T/a.txt\necho two\nexit\n" | timeout 5 ./runner > "$T/after" 2>&1
grep -q two "$T/after" && ok "shell stdout survives a redirect" \
  || bad "redirect leaked into the parent"

echo
echo "== pipelines: cmdA | cmdB =="
printf 'echo piped | cat\nexit\n' | timeout 5 ./runner > "$T/pipe" 2>&1
grep -q piped "$T/pipe" && ok "data crosses the pipe" || bad "no data through pipe"

printf 'printf "a\\nb\\nc\\n" | wc -l\nexit\n' | timeout 5 ./runner > "$T/count" 2>&1
grep -qE '^[[:space:]]*3' "$T/count" && ok "both stages run (wc sees 3 lines)" \
  || bad "second stage wrong"

echo
echo "== the two that catch a broken implementation =="
# A pipeline whose reader never sees EOF hangs forever. Bound it.
printf 'echo eof-test | cat\nexit\n' | timeout 5 ./runner >/dev/null 2>&1
if [ $? -eq 124 ]; then
  bad "pipeline HANGS - a pipe end is still open somewhere (usually the parent)"
else
  ok "pipeline terminates (reader sees EOF)"
fi

# Descriptor leak: run many pipelines in one session. If ends are not closed,
# the shell runs out of descriptors and later commands start failing.
{ i=0; while [ $i -lt 40 ]; do echo 'echo x | cat'; i=$((i+1)); done; echo exit; } \
  | timeout 20 ./runner > "$T/many" 2>&1
if [ "$(grep -c '^x$' "$T/many")" -eq 40 ]; then
  ok "40 pipelines in one session, no descriptor leak"
else
  bad "leaked descriptors: only $(grep -c '^x$' "$T/many") of 40 pipelines ran"
fi

echo
echo "== errors are reported, not silent =="
printf 'no-such-command-xyz\nexit\n' | timeout 5 ./runner > "$T/err" 2>&1
grep -qi 'no such file\|not found' "$T/err" && ok "unknown command reports why" \
  || bad "unknown command was silent"

rm -rf "$T"
echo
echo "== $pass passed, $fail failed =="
[ $fail -eq 0 ]
