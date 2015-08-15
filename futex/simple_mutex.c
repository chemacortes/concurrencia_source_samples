#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <linux/futex.h>
#include <sys/syscall.h>

#define NUM_THREADS      4
#define MAX_COUNT 10000000

// Just used to send the index of the id
struct tdata {
    int tid;
};

/* Simple FUTEX mutex implementation */

typedef struct simple_futex {
    int locked; // it takes 0 or 1
    int waiters; // The number of processes in the futex queue
} simple_futex_t;

void lock(simple_futex_t *futex) {
    int local;

    while (1) {
        local = __atomic_exchange_n(&futex->locked, 1, __ATOMIC_SEQ_CST);
        if (local == 0) return; // No contention

        __atomic_fetch_add(&futex->waiters, 1, __ATOMIC_SEQ_CST);
        syscall(__NR_futex, &futex->locked, FUTEX_WAIT, 1, NULL, 0, 0);
        __atomic_fetch_sub(&futex->waiters, 1, __ATOMIC_SEQ_CST);
    }
}

void unlock(simple_futex_t *futex) {
    __atomic_store_n(&futex->locked, 0, __ATOMIC_RELEASE);
    if (futex->waiters > 0) {
        syscall(__NR_futex, &futex->locked, FUTEX_WAKE, 1, NULL, 0, 0);
    }
}
/* END FUTEX */

simple_futex_t mutex;
int counter = 0;

void *count(void *ptr) {
    long i, max = MAX_COUNT/NUM_THREADS;
    int tid = ((struct tdata *) ptr)->tid;

    for (i=0; i < max; i++) {
        lock(&mutex);
        counter += 1;
        unlock(&mutex);
    }
    printf("End %d counter: %d\n", tid, counter);
}

int main (int argc, char *argv[]) {
    pthread_t threads[NUM_THREADS];
    int rc, i;
    struct tdata id[NUM_THREADS];

    for(i=0; i<NUM_THREADS; i++){
        id[i].tid = i;
        rc = pthread_create(&threads[i], NULL, count, (void *) &id[i]);
    }

    for(i=0; i<NUM_THREADS; i++){
        pthread_join(threads[i], NULL);
    }

    printf("Counter value: %d Expected: %d\n", counter, MAX_COUNT);
    return 0;
}
