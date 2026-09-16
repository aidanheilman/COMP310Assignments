# COMP 310 — Operating Systems
## Week 3 — Starter: pipe runner

The starting point for **A3**. Four TODOs; `make test` goes **6 → 9**.

```bash
make          # build
make test     # 6 of 9 pass before you start
./runner      # try it by hand
```

## The four TODOs

| # | Where | What |
|---|---|---|
| 1 | `run_redirected` | `open` the file, `dup2` it onto stdout, close the original |
| 2 | `run_pipeline`, left child | close the read end, `dup2` the write end to stdout, close it |
| 3 | `run_pipeline`, right child | close the write end, `dup2` the read end to stdin, close it |
| 4 | `run_pipeline`, parent | close **both** ends |

TODO 4 is one line and it is the one people often leave out. Without it the pipeline
hangs — with no error message, no crash, and nothing in any log.

## The table to get right

| Process | `fd[0]` read end | `fd[1]` write end |
|---|---|---|
| left | close | `dup2` to stdout, then close |
| right | `dup2` to stdin, then close | close |
| parent | close | close |

Every cell ends in a close.

## Two tests you cannot pass by guessing

**`pipeline terminates`** bounds every run with `timeout 5`. If your pipeline
hangs, this reports it in five seconds instead of freezing your terminal.

**`40 pipelines in one session`** exists because a descriptor leak works the
first time. One pipeline that leaks looks perfect; the fortieth does not.

Run `make test` before you submit. If those two pass, your descriptor work is
right.

## Before you submit

Read section 4 of `handouts/a3-handout.pdf` — *How it is graded* — **before** you
start rather than after. The commonest lost mark is TODO 4.

## Build

Make

## Run

./runner

## File map

/submission
    /src
        runner.c - main c file to be compiled with make, contains main and all sub functions
    /tests
        check-submission-spec.sh - checks submission formatting
        make-submission.sh - zips everything and validates submission formatting 
        run-tests.sh - tests code functionality
    Makefile - compiles runner.c under make, make test runs test scripts, make submit zips
    README.md - this file, provides valuable information about codebase
    REPORT.md - report file, covers what was made, test results, & citations
    submission.json - specially formatted json file for grading/professor organization

## Notes

Straightforward if the underlying concepts are well understood. 

Interesting note: I usually write and test the code on MacOS before doing additional testing in my Linux VM. In this case, I ran make test after finishing runner.c and the test suite said I only passed 2 tests. After scrutinizing my code 3 or 4 times, I found nothing wrong. When I ran the test suite in Linux it worked perfectly. So, I believe something in run-tests.sh must be Linux dependent, given that runner.c should be POSIX compliant and therefore run on MacOS.
