/* COMP 310 - Week 4 - STARTER: background jobs and signals.
 *
 *     cmd          run in the foreground: the shell waits for it
 *     cmd &        run in the background: the shell does not wait
 *     Ctrl-C       should end the foreground command, and nothing else
 *
 * `&` must be a separate word at the end of the line: `sleep 3 &`.
 *
 * Four TODOs. Two are signal handlers, one installs them, and one makes `&`
 * actually run in the background. Everything else is already written --
 * including run_foreground(), which you should read: it blocks SIGCHLD while
 * it waits, and the comment says why.
 */

/* sigaction(), sigprocmask(), setpgid() and friends are POSIX, not ISO C.
 * -std=c11 is strict ISO, so glibc hides them without this. macOS exposes them
 * anyway. This must come before any #include. */
#define _POSIX_C_SOURCE 200809L

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define MAX_LINE 1024
#define MAX_ARGS 64

/* ------------------------------------------------------------------ */
/* output that is safe inside a signal handler                         */
/* ------------------------------------------------------------------ */

/* Write "<tag> <pid>\n" to stdout using nothing but write().
 *
 * printf is NOT async-signal-safe: it keeps a buffer and may take a lock, so a
 * handler that calls printf while the main program is part-way through its own
 * printf can corrupt the output or deadlock. This formats the number by hand
 * so it can be called from a handler. Use it in TODO 1.
 */
static void write_pid_line(const char *tag, pid_t pid) {
    char buf[64];
    size_t n = 0;
    while (tag[n] != '\0' && n < 32) {       /* copy the tag */
        buf[n] = tag[n];
        n++;
    }
    buf[n++] = ' ';

    char digits[24];                         /* digits come out backward */
    size_t d = 0;
    long v = (long) pid;
    do {
        digits[d++] = (char) ('0' + v % 10);
        v /= 10;
    } while (v > 0);
    while (d > 0) {
        buf[n++] = digits[--d];              /* reverse them into buf */
    }
    buf[n++] = '\n';

    ssize_t r = write(STDOUT_FILENO, buf, n);
    (void) r;                                /* nothing useful to do if it fails */
}

/* ------------------------------------------------------------------ */
/* the two handlers                                                    */
/* ------------------------------------------------------------------ */

/* SIGCHLD: one or more children have finished. */
static void on_sigchld(int signo) {
    (void) signo;
    (void) write_pid_line;                   /* delete this line once TODO 1 calls it */
    /* TODO 1. Reap EVERY finished child, and report each one.
     *
     *   - save errno at the top and restore it at the bottom
     *   - call waitpid(-1, &status, WNOHANG) in a WHILE loop, until it
     *     returns 0 (children still running) or -1 (no children left)
     *   - for each pid it returns, call write_pid_line("[done]", pid)
     *
     * Only async-signal-safe calls in here: waitpid and write are; printf
     * is not.
     */
}

/* SIGINT: Ctrl-C was pressed. */
static void on_sigint(int signo) {
    (void) signo;
    /* TODO 2. Start a fresh line, and do nothing else:
     *
     *   write(STDOUT_FILENO, "\n", 1)
     *
     * Store write's return value in a variable and cast it to void, as
     * write_pid_line does, or the Linux build warns.
     */
}

/* Install `handler` for `signo`, with SA_RESTART, or exit on failure. */
static void install(int signo, void (*handler)(int)) {
    (void) signo;
    (void) handler;
    /* TODO 3. Fill in a struct sigaction and call sigaction():
     *
     *   - memset it to zero
     *   - sa_handler = handler
     *   - sigemptyset(&sa.sa_mask)
     *   - sa_flags = SA_RESTART
     *   - if sigaction(signo, &sa, NULL) fails, perror and exit(1)
     *
     * Then delete the two (void) lines above.
     */
}

/* ------------------------------------------------------------------ */
/* blocking SIGCHLD                                                    */
/* ------------------------------------------------------------------ */

/* Block SIGCHLD, saving the previous mask in *prev.
 *
 * Blocking is not ignoring. A blocked signal is held PENDING and delivered the
 * moment it is unblocked; an ignored signal is thrown away.
 */
static void block_sigchld(sigset_t *prev) {
    sigset_t block;
    sigemptyset(&block);
    sigaddset(&block, SIGCHLD);
    sigprocmask(SIG_BLOCK, &block, prev);
}

static void restore_mask(const sigset_t *prev) {
    sigprocmask(SIG_SETMASK, prev, NULL);
}

/* ------------------------------------------------------------------ */
/* running commands                                                    */
/* ------------------------------------------------------------------ */

/* Split `line` in place into argv words. Returns the count; argv is
 * NULL-terminated so it can be handed straight to execvp. */
static int tokenize(char *line, char *argv[MAX_ARGS]) {
    int argc = 0;
    char *tok = strtok(line, " \t\r\n");
    while (tok != NULL && argc < MAX_ARGS - 1) {
        argv[argc++] = tok;
        tok = strtok(NULL, " \t\r\n");
    }
    argv[argc] = NULL;
    return argc;
}

/* In the child, after fork: put back the mask the shell started with, then
 * replace this process with the command. Never returns. */
static void exec_command(char *argv[], const sigset_t *prev) {
    restore_mask(prev);                      /* the command must not inherit a blocked SIGCHLD */
    execvp(argv[0], argv);
    fprintf(stderr, "%s: %s\n", argv[0], strerror(errno));
    _exit(127);
}

/* Run argv and wait for it. Already written -- read it.
 *
 * SIGCHLD is blocked from before the fork until the child has been reaped.
 * Without that, the child could exit and on_sigchld could reap it first --
 * its waitpid(-1, ...) takes ANY child -- and this waitpid would fail with
 * ECHILD. Blocked, the SIGCHLD waits; restore_mask() delivers it afterward,
 * when the only children left to reap are background ones.
 */
static void run_foreground(char *argv[]) {
    sigset_t prev;
    block_sigchld(&prev);

    fflush(stdout);                          /* do not let the child inherit buffered output */
    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        restore_mask(&prev);
        return;
    }
    if (pid == 0) {
        /* CHILD. It stays in the shell's process group, so Ctrl-C reaches it. */
        exec_command(argv, &prev);
    }

    int status;
    while (waitpid(pid, &status, 0) < 0 && errno == EINTR) {
        /* interrupted before the child finished: wait again */
    }
    restore_mask(&prev);                     /* pending background SIGCHLDs arrive now */
}

/* Run argv in the background: start it and return without waiting. */
static void run_background(char *argv[]) {
    /* TODO 4. Make `&` really run in the background. Until you do, this
     * just runs the command in the foreground.
     *
     * Model it on run_foreground() above, with three differences:
     *
     *   - in the CHILD, call setpgid(0, 0) before exec_command(). That moves
     *     the job out of the terminal's process group, so Ctrl-C does not
     *     reach it.
     *   - in the PARENT, do NOT call waitpid. on_sigchld reaps the job.
     *   - in the PARENT, print "[bg] <pid>" with printf, fflush(stdout), and
     *     only then restore_mask(&prev).
     *
     * Keep block_sigchld() before the fork, so a job that exits at once is
     * still reported in the order started, then done.
     */
    run_foreground(argv);
}

/* ------------------------------------------------------------------ */

int main(void) {
    install(SIGCHLD, on_sigchld);
    install(SIGINT, on_sigint);

    char line[MAX_LINE];
    char *argv[MAX_ARGS];

    while (1) {
        if (isatty(STDIN_FILENO)) {
            printf("csh> ");
            fflush(stdout);
        }
        if (fgets(line, sizeof(line), stdin) == NULL) {
            if (ferror(stdin) && errno == EINTR) {
                clearerr(stdin);             /* a signal, not end of input */
                continue;
            }
            break;                           /* EOF */
        }

        int argc = tokenize(line, argv);
        if (argc == 0) {
            continue;
        }
        if (strcmp(argv[0], "exit") == 0) {
            break;
        }

        if (strcmp(argv[argc - 1], "&") == 0) {
            argv[--argc] = NULL;             /* drop the "&" word */
            if (argc == 0) {
                fprintf(stderr, "syntax error near '&'\n");
                continue;
            }
            run_background(argv);
        } else {
            run_foreground(argv);
        }
    }
    return 0;
}
