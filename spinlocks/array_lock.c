#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <stdint.h>


#define NUM_THREADS      4
#define MAX_COUNT 10000000

// Just used to send the index of the id
struct tdata {
    int tid;
};


#define PADDING  32
#define SIZE (NUM_THREADS * PADDING)
uint16_t tail;
uint8_t flag[SIZE];

int counter = 0;

void lock(uint16_t *my_index) {
    uint16_t slot = __atomic_fetch_add(&tail, 1, __ATOMIC_RELAXED);
    *my_index = (slot % NUM_THREADS) * PADDING;
    while(! flag[*my_index]);
    flag[*my_index] = 0;
}

void unlock(uint16_t *my_index) {
    __atomic_store_n(&flag[(*my_index+PADDING) % SIZE], 1, __ATOMIC_RELEASE);
}

void *count(void *ptr) {
    long i, max = MAX_COUNT/NUM_THREADS;
    int tid = ((struct tdata *) ptr)->tid;

    uint16_t my_index;

    for (i=0; i < max; i++) {
        lock(&my_index);
        counter += 1;
        unlock(&my_index);
    }
    printf("End %d counter: %d\n", tid, counter);
}

int main (int argc, char *argv[]) {
    pthread_t threads[NUM_THREADS];
    int rc, i;
    struct tdata id[NUM_THREADS];

    flag[0] = 1;

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
