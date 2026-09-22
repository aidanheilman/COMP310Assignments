/* COMP 310 - Week 4 - test helper: start a program the way a terminal would.
 *
 *     newpgrp ./runner
 *
 * It makes the program the leader of its own process group, with SIGINT and
 * SIGCHLD at their default actions and nothing blocked. After that,
 *
 *     kill -s INT -- -<pid>
 *
 * does exactly what pressing Ctrl-C does: it sends SIGINT to every process in
 * that group. The tests need this because a background job started from a
 * shell script begins life with SIGINT IGNORED, which would hide a runner that
 * never handles Ctrl-C at all.
 *
 * run-tests.sh compiles this into a temporary directory. You do not need to
 * build or edit it.
 */
#define _POSIX_C_SOURCE 200809L

#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "usage: newpgrp program [args...]\n");
        return 2;
    }

    struct sigaction sa;
    memset(&sa, 0, sizeof sa);
    sa.sa_handler = SIG_DFL;                 /* undo any inherited "ignore" */
    sigemptyset(&sa.sa_mask);
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGCHLD, &sa, NULL);
    sigaction(SIGPIPE, &sa, NULL);

    sigset_t none;
    sigemptyset(&none);
    sigprocmask(SIG_SETMASK, &none, NULL);   /* nothing blocked */

    if (setpgid(0, 0) < 0) {                 /* a new group; its id is our pid */
        perror("setpgid");
        return 1;
    }
    execvp(argv[1], argv + 1);
    perror(argv[1]);
    return 127;
}
