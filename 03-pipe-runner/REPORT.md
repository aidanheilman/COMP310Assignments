# COMP 310 — Operating Systems
## Week 3 — Report

<!-- Replace the italic prompt under each heading with your own words. Keep the headings: the specification names them, and `make test` checks that they are here. -->

## What
Under What, answer this explicitly: what would break if the parent did not close the write
end, and why?.
What — how you wired redirection and the pipe. Name each descriptor and say which
process closes it. 

I wired redirection by using open() and assigning that to an int file descriptor, copying the child process's standard out to the file descriptor, and then closing the file decriptor. This is all done in the child for redirection.

For the pipe, in the parent an int array file descriptor holding 0 and 1 is declared, which is then put into the pipe(command), which creates a pipe with a read and write end. 

In the left child, the read end of the pipe (fd0) is closed, stdout is copied over to the write end (fd1), and then fd1 is closed to prevent a hang (two references to the write end exist after stdout is dup2'd). After this, the desired process is executed in the left child and it's output is "in the pipe".

In the right child, I did the same this but effectively backward. Close the write end (fd1), copy stdin to the read end (fd0), close the read end, execute desired process and the output goes to stdout.

Back in the parent after the children are taken care of and the pipe is configured/processes are executed, close both of the parent's fd references to the pipe. If the parent did not close the write end of the pipe, the process would hang indefinitely because the read end would indefinitely wait for an EOF, which the OS does not produce unless ALL write ends have been closed in ALL processes. If the read end were left open, the FD table would eventually fill up, and the pipe would "leak".

## Results
Individual command testing redirection & pipe
aidanheilman@Aidans-Laptop 03-pipe-runner % ./runner
csh> echo hello > f
csh> cat f
hello
csh> ls | wc -l
       8

Make test on MacOS
aidanheilman@Aidans-Laptop 03-pipe-runner % make test
cc -std=c11 -Wall -Wextra -O2 -o runner src/runner.c
== plain commands still work ==
  FAIL: plain command

== redirection: cmd > file ==
  FAIL: no file created
  FAIL: wrong contents
  FAIL: redirect leaked into the parent

== pipelines: cmdA | cmdB ==
  FAIL: no data through pipe
  FAIL: second stage wrong

== the two that catch a broken implementation ==
  PASS: pipeline terminates (reader sees EOF)
  FAIL: leaked descriptors: only 0 of 40 pipelines ran

== errors are reported, not silent ==
  PASS: unknown command reports why

== 2 passed, 7 failed ==
make: *** [test] Error 1

Make test on Linux

## Citations
Course material only.
Although I will note: in finishing the in-class assignment last week I did watch several videos and read through a man page or two and websites and such to understand the core of what was really going on, so I had that base of knowledge to conceptually pull from as I was writing this code.

