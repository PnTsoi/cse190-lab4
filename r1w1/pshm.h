#ifndef PSHM_UCASE_H
#define PSHM_UCASE_H

#include <semaphore.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>


#define BUF_SIZE 4096   /* Maximum size for exchanged string */

#define CACHE_SIZE 64 // size of cache

#define SLIP_SIZE 8 // number of cache line for slip space


/* Define a structure that will be imposed on the shared
    memory object */
struct shmbuf {
    sem_t  in_sync;            /* semaphore to wake reader once slip space is made */
    size_t cnt;             /* Number of bytes used in 'buf' */
    unsigned char   buf[BUF_SIZE];   /* Data being transferred */
};

struct readbuf {
    unsigned char buf[BUF_SIZE];
}

#endif  // include guard
