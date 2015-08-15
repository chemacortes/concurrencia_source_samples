#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>


#define NUM_THREADS      4
#define MAX_COUNT 10000000

// Just used to send the index of the id
struct tdata {
    int tid;
};

// Struct for MCS spinlock
struct mcs_spinlock {
    struct mcs_spinlock *next;
    unsigned char locked;
};

struct mcs_spinlock *tail = NULL;

int counter = 0;

void lock(struct mcs_spinlock *node) {
    struct mcs_spinlock *predecessor = node;
    node->next = NULL;
    node->locked = 1;
    predecessor = __atomic_exchange_n(&tail, node, __ATOMIC_RELAXED);
    if (predecessor != NULL) {
        predecessor->next = node;
        while (node->locked);
    }
    node->locked = 0;
}

void unlock(struct mcs_spinlock *node) {
    struct mcs_spinlock *last = node;

    if (! node->next) { // I'm the last in the queue
        if (__atomic_compare_exchange_n(&tail, &last, NULL, 0, __ATOMIC_RELAXED, __ATOMIC_RELAXED) ) {
            return;
        } else {
            // Another process executed exchange but
            // didn't asssign our next yet, so wait
            while (! node->next);
        }
    } else {
        // We force a memory barrier to ensure the critical section was executed before the next
        __atomic_thread_fence (__ATOMIC_RELEASE);
    }

    node->next->locked = 0;
}

void *count(void *ptr) {
    long i, max = MAX_COUNT/NUM_THREADS;
    int tid = ((struct tdata *) ptr)->tid;

    struct mcs_spinlock node;

    for (i=0; i < max; i++) {
        lock(&node);
        counter += 1;
        unlock(&node);
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
