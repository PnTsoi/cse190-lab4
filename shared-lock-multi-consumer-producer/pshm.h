#ifndef PSHM_UCASE_H
#define PSHM_UCASE_H

#include <semaphore.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>



#define CACHE_SIZE 64 // size of cache

#define SLIP_SIZE 8 // number of cache line for slip space

#define NUM_CPU 2 // x writers, x consumers

#define SERVER_CPU 6

#define SHM_PATH "/shmd"

#define MAX_QUEUE 150

#define NUM_MSG 1000

#define NUM_MEASUREMENTS 10000

#define BUF_SIZE 64*CACHE_SIZE

/* Define a structure that will be imposed on the shared
    memory object, this will include all message channels */
struct shmbuf {
    pthread_barrier_t barrier;
    pthread_mutex_t mutex;
    int head;
    int tail;
    int queue; // fake producer consumer queue for easy implementation
    unsigned char buf[BUF_SIZE]; // actual producer consumer queue
};


#endif  // include guard
