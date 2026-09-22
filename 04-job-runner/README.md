# COMP 310 — Operating Systems
## Week 4 — Starter: job runner

The starting point for **A4**. Four TODOs; `make test` goes **4 → 12**.

```bash
make          # build
make test     # 4 of 12 pass before you start
./runner      # try it by hand: sleep 3 &   then   sleep 30  and Ctrl-C
```

## The four TODOs

| # | Where | What |
|---|---|---|
| 1 | `on_sigchld` | reap every finished child with `waitpid(-1, &status, WNOHANG)` **in a loop**; report each with `write_pid_line("[done]", pid)`; save and restore `errno` |
| 2 | `on_sigint` | write a newline, and nothing else |
| 3 | `install` | fill in a `struct sigaction` and call `sigaction` |
| 4 | `run_background` | in the child, `setpgid(0, 0)`; in the parent, print `[bg] <pid>` and do **not** wait |

Do them in that order. TODO 3 is what makes TODOs 1 and 2 run at all.

**Read `run_foreground` before TODO 4.** It is already written, and TODO 4 is
the same function with three changes. Its comment explains why `SIGCHLD` is
blocked while the shell waits.

## The output the checks look for

```
csh> sleep 3 &
[bg] 4213
csh> 
[done] 4213
```

Exactly `[bg] <pid>` when a job starts and `[done] <pid>` when it is reaped, one
per line. Keep that shape.

## The checks, by TODO

| Check | Fails until |
|---|---|
| `'sleep 4 &' returns at once` | TODO 4 |
| `prints '[bg] <pid>'` | TODO 4 |
| `a finished background job is reported` | TODOs 1, 3, 4 |
| `leaves no zombie` | TODOs 1, 3, 4 |
| `four jobs finishing together are all reaped` | TODO 1 uses a **loop** |
| `the shell survives Ctrl-C` | TODOs 2, 3 |
| `Ctrl-C ends the foreground command` | TODO 2 is a **handler**, not `SIG_IGN` |
| `a background job survives Ctrl-C` | TODO 4 calls `setpgid` |

**Two of them cannot be passed by accident.** "Four jobs finishing together"
arranges for four children to exit while `SIGCHLD` is blocked, so the kernel
delivers **one** signal for four exits. A handler that calls `waitpid` once
reaps one job and leaves three zombies.

"Ctrl-C ends the foreground command" is the one `SIG_IGN` fails. Ignoring
`SIGINT` keeps the shell alive, but every command the shell starts inherits the
ignore, so Ctrl-C stops working on all of them.

## How the tests press Ctrl-C

A terminal does not signal one process when you press Ctrl-C. It sends `SIGINT`
to every process in its **foreground process group**. The tests do the same:
`tests/newpgrp.c` starts the runner as the leader of its own group, and

```sh
kill -s INT -- -4100      # a NEGATIVE number means "the whole process group"
```

signals the runner and its foreground command together. `run-tests.sh` builds
the helper in a temporary directory; you do not need to touch it.

## Before you submit

Read section 4 of `handouts/a4-handout.pdf` — *How it is graded* — before you
start rather than after. It also explains the `## Answers` block in
`REPORT.md`.

## Build

*How is it built? Give the exact command.*

## Run

*How do you run it? Give the exact command.*

## File map

*One line per file: what it is for.*

## Notes

*Anything a reader should know — what is unfinished, what you would do next, anything that surprised you.*
