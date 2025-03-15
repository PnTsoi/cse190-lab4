#include "./pshm.h"

#include <ctype.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#include <iostream>


using namespace std;

void child_process(int cpu_id) {

    int process_id = fork();
    if(process_id == 0) { // child
        int            fd;
        char           *shmpath;
        cpu_set_t     set;
        size_t         len;
        struct shmbuf  *shmp;

        // lock process onto CPU
        CPU_SET(cpu_id, &set);
        if (sched_setaffinity(getpid(), sizeof(set), &set) == -1) return;

        shmpath = (char*) malloc(strlen(SHM_PATH)+1);
        if(shmpath == NULL) return;
        strcpy(shmpath, SHM_PATH);

        /* Open the existing shared memory object and map it
        into the caller's address space. */
        fd = shm_open(shmpath, O_RDWR, 0);
        if (fd == -1) return; 

        shmp = static_cast<shmbuf*>(mmap(NULL, sizeof(*shmp), PROT_READ | PROT_WRITE,
            MAP_SHARED, fd, 0));
        if (shmp == MAP_FAILED) return;

        pthread_barrier_wait(&shmp->barrier); // synchronization barrier

        int tail = 0;

        int cnt = 0;
        while(cnt < NUM_MSG) {
            while(shmp->channels[cpu_id].buf[tail+CACHE_SIZE-1] != 0); // spin
            // for(int i = tail; i < tail+CACHE_SIZE; i++) shmp->channels[cpu_id].buf[i] = 255; // produce, or write
            shmp->channels[cpu_id].buf[tail+CACHE_SIZE-1] = 255; // if only writing the last word test
            tail = (tail+CACHE_SIZE) % BUF_SIZE; // move head
            //TODO: perhaps add delay here to simulate doing work
            cnt++;
        }
        exit(0);
    }
    else return; // parent
}


int main(int argc, char* argv[]) {

    int fd;
    char *shmpath;
    cpu_set_t     set;
    struct shmbuf *shmp;

    shmpath = (char*) malloc(strlen(SHM_PATH)+1); 
    if(shmpath == NULL) return -1;
    strcpy(shmpath, SHM_PATH); // get shared memory name

    cout<<"here"<<'\n';
    // lock process onto CPU
    CPU_SET(SERVER_CPU, &set);
    if (sched_setaffinity(getpid(), sizeof(set), &set) == -1)


    /* Create shared memory object and set its size to the size
    of our structure. */

    cout<<"here"<<'\n';
    fd = shm_open(shmpath, O_CREAT | O_EXCL | O_RDWR, 0600);
    if (fd == -1)
        // errExit("shm_open");
        return -1;

    if (ftruncate(fd, sizeof(struct shmbuf)) == -1)
        // errExit("ftruncate");
        return -1;

    cout<<"here"<<'\n';
    /* Map the object into the caller's address space. */

    shmp = static_cast<shmbuf*>(mmap(NULL, sizeof(*shmp), PROT_READ | PROT_WRITE,
                MAP_SHARED, fd, 0));


    if (shmp == MAP_FAILED)
        // errExit("mmap");
        return -1;

    cout<<"here\n";

    pthread_barrierattr_t attr;
    pthread_barrierattr_init(&attr);
    pthread_barrierattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
    pthread_barrier_init(&shmp->barrier, &attr, NUM_CPU+1); // Adjust count as needed

    // Create children processes

    for(int i = 0; i < 6; i++) {
        child_process(i);
    }

    pthread_barrier_wait(&shmp->barrier);
    pthread_barrier_destroy(&shmp->barrier);
    cout<<"here\n";

    // iterate over all channels until it gets enough 6000 messages.

    int cnt = 0;

    // since the server side is the consumer (receiver) of messages, it will hold heads.
    int head[NUM_CPU] = {0};

    // for sake of simplicity, the actual consumer producer "queue" will be a variable for now.
    int queue = 0;
    int total_msg_cnt = NUM_MSG * NUM_CPU;
    int caught = 0;

    while(cnt < total_msg_cnt) {
        for(int i = 0; i < NUM_CPU; i++) {
            int cur = head[i];
            if(shmp->channels[i].buf[cur+CACHE_SIZE-1] == 0) {caught++;continue;} // this queue not ready yet

            if (i%2 == 0 && queue == MAX_QUEUE) continue; // producer
            if (i%2 == 1 && queue == 0) continue; // consumer

            shmp->channels[i].buf[cur+CACHE_SIZE-1] = 0; // set entry as empty, aka dequeue
            queue += (i%2==0) ? 1 : -1; // if producer, produce. if consumer, consume
            cnt++;
            head[i] = (cur+CACHE_SIZE)%BUF_SIZE; // move head
        }
    }

    cout<<"end\n";
    cout<<cnt<<'\n';
    cout<<caught<<'\n';

    shm_unlink(shmpath); // unlink the shared memory


    return 0;
}


