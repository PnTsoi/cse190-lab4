#ifndef PSHM_UCASE_H
#define PSHM_UCASE_H

#include <semaphore.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>


#define BUF_SIZE 4096   /* Maximum size for exchanged string */

#define CACHE_SIZE 64 // size of cache

#define SLIP_SIZE 8 // number of cache line for slip space

#define NUM_CPU 2 // x writers, x consumers

#define SERVER_CPU 6

#define SHM_PATH "/shmb"

#define MAX_QUEUE 150

#define NUM_MSG 1000

#define NUM_MEASUREMENTS 10000

struct channel {
    unsigned char buf[BUF_SIZE];
};
/* Define a structure that will be imposed on the shared
    memory object, this will include all message channels */
struct shmbuf {
    pthread_barrier_t barrier;
    channel channels[NUM_CPU];
};


#endif  // include guard
