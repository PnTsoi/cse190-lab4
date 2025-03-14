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
    cpu_set_t     set;
    size_t         len;
    struct shmbuf  *shmp;
    struct readbuf read;
    int head = 0;

    // lock process onto CPU
    CPU_SET(READER_CPU, &set);
    if (sched_setaffinity(getpid(), sizeof(set), &set) == -1) return -1;

    shmpath = (char*) malloc(strlen(SHM_PATH)+1);
    if(shmpath == NULL) return -1;
    strcpy(shmpath, SHM_PATH);


    /* Open the existing shared memory object and map it
        into the caller's address space. */

    fd = shm_open(shmpath, O_RDWR, 0);
    if (fd == -1)
        // errExit("shm_open");
        return -1;

    shmp = static_cast<shmbuf*>(mmap(NULL, sizeof(*shmp), PROT_READ | PROT_WRITE,
                MAP_SHARED, fd, 0));
    if (shmp == MAP_FAILED)
        // errExit("mmap");
        return -1;


    /* Tell peer that it can now write shared memory. */

    if (sem_post(&shmp->in_sync) == -1)
        // errExit("sem_post");
        return -1;
    
    bool cnt_check = false;
    int cnt = 0;

    unsigned char data[CACHE_SIZE] = {0};

    // Starts consuming
    while (cnt < 1000) {
        while(shmp->buf[head+CACHE_SIZE-1] == 0); // spin

        for(int i = head; i < head+CACHE_SIZE; i++) data[i-head] = shmp->buf[i]; // consume

        shmp->buf[head+CACHE_SIZE-1] = 0; // set entry as empty, aka dequeue

        cnt++;
        cout<<cnt<<' ';

        head = (head+CACHE_SIZE)%BUF_SIZE; // move head
    }


    shm_unlink(shmpath);

    return 0;
}