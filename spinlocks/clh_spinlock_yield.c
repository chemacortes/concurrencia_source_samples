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

// Struct for CLH spinlock
struct clh_node {
    unsigned char locked;
    struct clh_node *prev;
};

struct clh_node lock_node; // point to an unowned node
struct clh_node *tail = &lock_node;

int counter = 0;

void lock(struct clh_node *node) {
    struct clh_node *predecessor;

    node->locked = 1;
    predecessor = node->prev = __atomic_exchange_n(&tail, node, __ATOMIC_SEQ_CST);
    while (predecessor->locked) {
        sched_yield();
    }
}

void unlock(struct clh_node **node) {
    struct clh_node *pred = (*node)->prev;
    struct clh_node *tmp = *node;

    *node = pred; // Take the previous node, we reuse it
    __atomic_store_n(&tmp->locked, 0, __ATOMIC_RELEASE);
}

void *count(void *ptr) {
    long i, max = MAX_COUNT/NUM_THREADS;
    int tid = ((struct tdata *) ptr)->tid;

    struct clh_node *node;
    node = malloc(sizeof(struct clh_node));

    for (i=0; i < max; i++) {
        lock(node);
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
