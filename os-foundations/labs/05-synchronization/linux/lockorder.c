/*
 * lockorder.c - Stage 05 companion for Linux: an AB-BA lock order, the
 * deadlock lockdep exists to report. Linux's lockdep runs inside the
 * kernel; for user space, ThreadSanitizer has the same kind of check.
 *
 * Build:  gcc -O1 -g -Wall -pthread -o lockorder lockorder.c
 *         gcc -O1 -g -Wall -pthread -fsanitize=thread -o lockorder-tsan lockorder.c
 * Run:    ./lockorder              one thread takes A then B and finishes,
 *                                  then a second thread takes B then A.
 *                                  Nothing goes wrong on this run.
 *         ./lockorder-tsan         the same run under ThreadSanitizer, which
 *                                  remembers the order A->B and reports
 *                                  B->A as a potential deadlock.
 *         ./lockorder together     both threads at once, so the deadlock
 *                                  really happens. The second lock is taken
 *                                  with a 1 s timeout so the program can
 *                                  report it and exit.
 */
#define _GNU_SOURCE
#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

static pthread_mutex_t A = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t B = PTHREAD_MUTEX_INITIALIZER;
static pthread_barrier_t both_hold_first;
static int together;

struct order {
    const char *name;
    pthread_mutex_t *first, *second;
    const char *first_name, *second_name;
};

static void *take(void *arg)
{
    struct order *o = arg;
    pthread_mutex_lock(o->first);
    if (together)
        pthread_barrier_wait(&both_hold_first); /* each holds its first lock */

    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts); /* timedlock uses CLOCK_REALTIME */
    ts.tv_sec += 1;
    int r = pthread_mutex_timedlock(o->second, &ts);
    if (r == 0) {
        printf("  %s: took %s then %s\n", o->name, o->first_name, o->second_name);
        pthread_mutex_unlock(o->second);
    } else {
        printf("  %s: holds %s, gave up waiting 1 s for %s (%s): deadlock\n", o->name,
               o->first_name, o->second_name, strerror(r));
    }
    pthread_mutex_unlock(o->first);
    return NULL;
}

int main(int argc, char **argv)
{
    together = argc > 1 && strcmp(argv[1], "together") == 0;
    setvbuf(stdout, NULL, _IONBF, 0);
    pthread_barrier_init(&both_hold_first, NULL, 2);

    struct order ab = {"thread 1", &A, &B, "A", "B"};
    struct order ba = {"thread 2", &B, &A, "B", "A"};
    pthread_t t1, t2;
    printf("%s\n", together ? "Both threads at once:" : "One thread after the other:");
    pthread_create(&t1, NULL, take, &ab);
    if (!together)
        pthread_join(t1, NULL);
    pthread_create(&t2, NULL, take, &ba);
    if (together)
        pthread_join(t1, NULL);
    pthread_join(t2, NULL);
    return 0;
}
