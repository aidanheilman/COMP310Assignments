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
#include <string.h>


/* `make trials-intervention SPIN=<n>` builds this file with
 * -DA5_SPIN_OVERRIDE=<n> (handout section 2.4). A normal build uses your own
 * A5_SPIN. */
#ifndef A5_SPIN_OVERRIDE
#define A5_SPIN_OVERRIDE A5_SPIN
#endif

/* File scope counter & mutex */
static long counter = 0;
static pthread_mutex_t counter_lock = PTHREAD_MUTEX_INITIALIZER;

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
        pthread_mutex_lock(&counter_lock);
        long seen = counter;
        spin(A5_SPIN_OVERRIDE);
        counter = seen + 1;
        pthread_mutex_unlock(&counter_lock);
    }
    return NULL;
}

int main(void)
{
    pthread_t t[A5_THREADS];

    for (int i = 0; i < A5_THREADS; i++) {
        int rc = pthread_create(&t[i], NULL, worker, NULL);
        if (rc != 0) {
            fprintf(stderr, "pthread create: %s\n", strerror(rc));
            exit(1);
        }
    }
    for (int i = 0; i < A5_THREADS; i++) {
        int rc = pthread_join(t[i], NULL);
        if (rc != 0) {
            fprintf(stderr, "pthread join: %s\n", strerror(rc));
            exit(1);
        }
    }

    printf("%ld %ld\n", counter, (long) A5_THREADS * A5_ITERATIONS);
    return 0;
}
