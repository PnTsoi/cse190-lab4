#include "./pshm.h"

#include <ctype.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>

#include <iostream>

using namespace std;

int main(int argc, char* argv[]) {

    int fd;
    char *shmpath;
    struct shmbuf *shmp;

    if(argc != 2) {
        cout << "2 ARGUMENTS" << '\n';
        exit(EXIT_FAILURE);
    }

    shmpath = argv[1]; // get shared memory name

    /* Create shared memory object and set its size to the size
    of our structure. */

    fd = shm_open(shmpath, O_CREAT | O_EXCL | O_RDWR, 0600);
    if (fd == -1)
        errExit("shm_open");

    if (ftruncate(fd, sizeof(struct shmbuf)) == -1)
        errExit("ftruncate");

    /* Map the object into the caller's address space. */

    shmp = mmap(NULL, sizeof(*shmp), PROT_READ | PROT_WRITE,
                MAP_SHARED, fd, 0);
    if (shmp == MAP_FAILED)
        errExit("mmap");

    /* Initialize semaphores as process-shared, with value 0. */

    if (sem_init(&shmp->in_sync, 1, 0) == -1)
        errExit("sem_init-in_sync");

    /* Starts writing into shared memory the first 8 cache-line to create
    a slip space before waking up the reader */

    for(int i = 0; i < SLIP_SIZE * CACHE_SIZE; i++) // 
        shmp->buf[i] = 255;

    /* Wait for 'sem1' to be posted by peer before touching
        shared memory. */

    if (sem_wait(&shmp->in_sync) == -1)
        errExit("sem_wait");

    /* Starts writing to the end of buffer */

    for(int i = SLIP_SIZE * CACHE_SIZE; i < BUFF_SIZE; i++) // 
        shmp->buf[i] = 255;


    shm_unlink(shmpath); // unlink the shared memory


    return 0;
}


