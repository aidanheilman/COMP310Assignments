/* COMP 310 - Week 3 - STARTER: add redirection and one pipe to the runner.
 *
 * Week 1's runner ran one command. This one adds the two things that make a
 * shell a shell:
 *
 *     cmd > file      redirect stdout      (open + dup2)
 *     cmdA | cmdB     connect two commands (pipe + fork + dup2)
 *
 * The interesting part is not dup2. It is CLOSING: every descriptor that is
 * still open somewhere it is not needed either leaks or, for a pipe, stops the
 * reader ever seeing EOF. See close_all_pipe_ends() and the comments at each
 * call site.
 */

/* PATH_MAX, fork(), dup2() and friends are POSIX, not ISO C. -std=c11 is strict
 * ISO, so glibc hides them without this. macOS exposes them anyway.
 * This must come before any #include. */
#define _POSIX_C_SOURCE 200809L

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define MAX_LINE 1024
#define MAX_ARGS 64

/* ------------------------------------------------------------------ */
/* tokenizing                                                          */
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

/* Find `sep` in argv and split it into two argv arrays.
 *
 * Returns 1 if the separator was found, 0 if not. On success `left` and `right`
 * point into the SAME storage as argv -- we only overwrite the separator slot
 * with NULL, which terminates the left side in place. No copying.
 */
static int split_on(char *argv[], const char *sep, char ***left, char ***right) {
    for (int i = 0; argv[i] != NULL; i++) {
        if (strcmp(argv[i], sep) == 0) {
            if (i == 0 || argv[i + 1] == NULL) {
                return -1;              /* nothing on one side: a syntax error */
            }
            argv[i] = NULL;             /* terminate the left command here */
            *left = argv;
            *right = &argv[i + 1];
            return 1;
        }
    }
    return 0;
}

/* ------------------------------------------------------------------ */
/* redirection: cmd > file                                             */
/* ------------------------------------------------------------------ */

/* Run argv with stdout sent to `path`.
 *
 * The child does the rewiring, never the parent: dup2 in the parent would
 * redirect the SHELL's own stdout and every later command with it.
 */
static int run_redirected(char *argv[], const char *path) {
    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        return -1;
    }

    if (pid == 0) {
        /* CHILD */
        /* TODO 1. Send this child's stdout to `path`:
         *   - open(path, O_WRONLY | O_CREAT | O_TRUNC, 0644)
         *   - check it, then dup2 it onto STDOUT_FILENO
         *   - close the original descriptor: the duplicate is enough
         * Do this HERE, in the child. Doing it in the parent would redirect
         * the shell's own output and every command after this one.
         */
        // Assign file descriptor to var fd
        int fd;
        fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        // If fd is less than 0, file was not initialized correctly (I/O table starts at index 0)
        // Terminate child process immediately, without clearing buffers, and throw error
        if (fd < 0) { 
            perror("open"); 
            _exit(1); 
        }
        // On success, redirect standard out to the file using dup2
        dup2(fd, STDOUT_FILENO);
        // Now, there are two pointers to the file - STDOUT and fd, close fd.
        close(fd);
        // Execute child process
        execvp(argv[0], argv);
        // If the process in argv did not execute, an error message is printed and the child terminates
        // Note that the child process only gets this far if execvp() fails
        fprintf(stderr, "%s: %s\n", argv[0], strerror(errno));
        _exit(127);
    }

    int status;
    if (waitpid(pid, &status, 0) < 0) {
        perror("waitpid");
        return -1;
    }
    return WIFEXITED(status) ? WEXITSTATUS(status) : -1;
}

/* ------------------------------------------------------------------ */
/* pipelines: cmdA | cmdB                                              */
/* ------------------------------------------------------------------ */

/* Run `left | right`.
 *
 * Three processes are involved and each has a different job with the two pipe
 * descriptors. Getting this table right IS the assignment:
 *
 *     process   pipe read end (fd[0])   pipe write end (fd[1])
 *     -------   ---------------------   ----------------------
 *     left      close                   dup2 to stdout, then close
 *     right     dup2 to stdin, close    close
 *     parent    close                   close
 *
 * The parent closing both is the step people miss. While the parent still holds
 * the write end open, the kernel cannot tell the reader that no writer remains,
 * so `right` blocks in read() forever and the shell hangs.
 */
static int run_pipeline(char *left[], char *right[]) {
    int fd[2];
    if (pipe(fd) < 0) {
        perror("pipe");
        return -1;
    }

    pid_t p1 = fork();
    if (p1 < 0) {
        perror("fork");
        close(fd[0]);
        close(fd[1]);
        return -1;
    }
    if (p1 == 0) {
        /* LEFT: writes into the pipe */
        /* TODO 2. This child writes, so it must:
         *   - close the READ end (it never reads)
         *   - dup2 the WRITE end onto STDOUT_FILENO
         *   - close the write end afterwards
         */
        // Close the read end first
        close(fd[0]);
        // Copy stdout of process to the write-end of the pipe
        dup2(fd[1], STDOUT_FILENO);
        // Close the write end of the pipe now that stdout is pointing to it
        close(fd[1]);
        // Execute child (left end of pipe) process
        execvp(left[0], left);
        // If execvp fails, throw error & exit
        fprintf(stderr, "%s: %s\n", left[0], strerror(errno));
        _exit(127);
    }

    pid_t p2 = fork();
    if (p2 < 0) {
        perror("fork");
        close(fd[0]);
        close(fd[1]);
        return -1;
    }
    if (p2 == 0) {
        /* RIGHT: reads from the pipe */
        /* TODO 3. The mirror image of TODO 2: close the WRITE end, dup2 the
         * READ end onto STDIN_FILENO, then close it.
         */
        // Close the write end of the pipe
        close(fd[1]);
        // Copy stdin of process to read-end of the pipe
        dup2(fd[0], STDIN_FILENO);
        // Close read end of pipe now that stdin is pointing to it
        close(fd[0]);
        // Execute child (right end of pipe) process
        execvp(right[0], right);
        // If execvp fails, throw error & exit
        fprintf(stderr, "%s: %s\n", right[0], strerror(errno));
        _exit(127);
    }

    /* TODO 4. The parent needs NEITHER end -- close both.
     *
     * This is the one people commonly leave out, and it does not look like a bug: the
     * pipeline simply hangs. While the parent still holds the write end open,
     * the kernel cannot tell the reader that every writer has gone, so the
     * reader waits for an EOF that never arrives. `make test` catches it.
     */
    // Close parent read end, close parent write end
    close(fd[0]);
    close(fd[1]);
    // All references to pipe from both children and the parent are closed now
    int status, rc = -1;
    waitpid(p1, &status, 0);                 /* reap both, in order */
    if (waitpid(p2, &status, 0) >= 0 && WIFEXITED(status)) {
        rc = WEXITSTATUS(status);            /* a pipeline's status is the LAST */
    }
    return rc;
}

/* ------------------------------------------------------------------ */
/* plain command                                                       */
/* ------------------------------------------------------------------ */

static int run_external(char *argv[]) {
    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        return -1;
    }
    if (pid == 0) {
        execvp(argv[0], argv);
        fprintf(stderr, "%s: %s\n", argv[0], strerror(errno));
        _exit(127);
    }
    int status;
    if (waitpid(pid, &status, 0) < 0) {
        perror("waitpid");
        return -1;
    }
    return WIFEXITED(status) ? WEXITSTATUS(status) : -1;
}

/* ------------------------------------------------------------------ */

int main(void) {
    char line[MAX_LINE];
    char *argv[MAX_ARGS];

    while (1) {
        if (isatty(STDIN_FILENO)) {
            printf("csh> ");
            fflush(stdout);
        }
        if (fgets(line, sizeof(line), stdin) == NULL) {
            break;                           /* EOF */
        }

        int argc = tokenize(line, argv);
        if (argc == 0) {
            continue;
        }
        if (strcmp(argv[0], "exit") == 0) {
            break;
        }

        char **left, **right;

        /* A pipeline is checked first: `a | b > f` is a pipeline whose right
         * side redirects, not a redirect of `a | b`. */
        int piped = split_on(argv, "|", &left, &right);
        if (piped < 0) {
            fprintf(stderr, "syntax error near '|'\n");
            continue;
        }
        if (piped) {
            run_pipeline(left, right);
            continue;
        }

        int redir = split_on(argv, ">", &left, &right);
        if (redir < 0) {
            fprintf(stderr, "syntax error near '>'\n");
            continue;
        }
        if (redir) {
            run_redirected(left, right[0]);
            continue;
        }

        run_external(argv);
    }
    return 0;
}
