#include "sharedmem.h"
#include <errno.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <cstring>

int main() {
    // Create shared memory region (if it doesn't already exist)
    int fd = shm_open("tekos", O_CREAT | O_EXCL | O_RDWR, 0600);
    while(fd == -1) {
        if(errno == EEXIST) {
            fd = shm_open("tekos", O_RDWR, 0600);
            continue;
        } else 
            errExit("shm open error")
    }

    // Create mmap
    struct shmbuf* shmap = (struct shmbuf*) mmap(NULL, sizeof(*shmap), 
            PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if(shmap == MAP_FAILED)
        errExit("mmap error")

    // Initialize semaphores (if they haven't already been initialized)
    int value;
    if(sem_getvalue(&shmap->sem, &value) == -1 && errno == EINVAL) {
        if(sem_init(&shmap->sem, 1, 0) == -1)
            errExit("sem_init-sem1");
    }

    // Wait for data
    if(sem_wait(&shmap->sem) == -1)
        errExit("sem_wait");

    // Copy into local memory
    char* buf = new char[BUF_SIZE];
    memcpy(buf, &shmap->buf, shmap->cnt);
    printf("%s\n", buf);

    // All done!
    shm_unlink("tekos");
    exit(EXIT_SUCCESS);
}
