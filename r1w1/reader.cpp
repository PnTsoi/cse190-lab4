#include <fcntl.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#include "pshm.h"

#include <iostream>

using namespace std;

int main(int argc, char* argv[]) {

    int            fd;
    char           *shmpath, *string;
    size_t         len;
    struct shmbuf  *shmp;
    struct readbuf read;

    if (argc != 3) {
        fprintf(stderr, "Usage: %s /shm-path string\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    shmpath = argv[1];
    string = argv[2];
    len = strlen(string);

    if (len > BUF_SIZE) {
        fprintf(stderr, "String is too long\n");
        exit(EXIT_FAILURE);
    }

    /* Open the existing shared memory object and map it
        into the caller's address space. */

    fd = shm_open(shmpath, O_RDWR, 0);
    if (fd == -1)
        errExit("shm_open");

    shmp = mmap(NULL, sizeof(*shmp), PROT_READ | PROT_WRITE,
                MAP_SHARED, fd, 0);
    if (shmp == MAP_FAILED)
        errExit("mmap");


    /* Tell peer that it can now access shared memory. */

    if (sem_post(&shmp->sem1) == -1)
        errExit("sem_post");

    // Starts reading
    for(int i = CACHE_SIZE-1; i < BUF_SIZE; i += CACHE_SIZE) {
        while (shmp->buf[i] == 0) {
            // spin, wait for writer to catch up, creating a slip space between the two
        }

        for(int j = i-CACHE_SIZE+1; j < CACHE_SIZE; j++)
            read->buf[i] = shmp->buf[i];
    }



    return 0;
}