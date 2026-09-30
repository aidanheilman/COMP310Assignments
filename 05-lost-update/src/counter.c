/* COMP 310 - Week 5 - A5: the lost update (STARTER)
 *
 * Every thread adds 1 to the same counter, A5_ITERATIONS times. The total
 * should be A5_THREADS * A5_ITERATIONS. It will not be.
 *
 * Your configuration comes from your netid: run `make config`.
 */
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
/* `config.h` is generated from your netid by `make`; it is not shipped, so an
 * editor underlining the include below means only that you have not built yet. */
#if defined(__has_include) && !__has_include("config.h")
#  error "src/config.h is not generated yet -- run `make config` first."
#endif
#include "config.h"

/* `make trials-intervention SPIN=<n>` builds this file with
 * -DA5_SPIN_OVERRIDE=<n> (handout section 2.4). A normal build uses your own
 * A5_SPIN. */
#ifndef A5_SPIN_OVERRIDE
#define A5_SPIN_OVERRIDE A5_SPIN
#endif

static long counter = 0;

/* Widens the gap between the read and the write. It does no work; it exists so
 * the race is large enough to measure rather than large enough to argue about. */
static void spin(int n)
{
    for (volatile int i = 0; i < n; i++)
        ;
}

static void *worker(void *arg)
{
    (void) arg;
    for (long k = 0; k < A5_ITERATIONS; k++) {
        /* TODO 1 -- make this update safe (handout section 2.5; C companion
         * 5.9 shows the calls).
         *
         * CONTRACT: after every thread has been joined, `counter` equals
         * A5_THREADS * A5_ITERATIONS on every run, for any window width. The
         * read, the spin and the write stay, in that order, and all three sit
         * inside ONE critical section.
         *
         * Leave these three lines exactly as they are until sections 2.2 and
         * 2.4 are measured: both measure this code unfixed. */
        long seen = counter;
        spin(A5_SPIN_OVERRIDE);
        counter = seen + 1;
    }
    return NULL;
}

int main(void)
{
    pthread_t t[A5_THREADS];

    for (int i = 0; i < A5_THREADS; i++) {
        /* TODO 2 -- start the thread, and check what pthread_create returns.
         * CONTRACT: on failure, report which call failed and why, and exit
         * non-zero. It returns an error NUMBER and does NOT set errno. The call
         * below currently ignores the return value. */
        pthread_create(&t[i], NULL, worker, NULL);
    }
    for (int i = 0; i < A5_THREADS; i++) {
        /* TODO 3 -- wait for it, and check the return the same way. */
        pthread_join(t[i], NULL);
    }

    printf("%ld %ld\n", counter, (long) A5_THREADS * A5_ITERATIONS);
    return 0;
}
