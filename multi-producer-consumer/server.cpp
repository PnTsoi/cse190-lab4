#include "./pshm.h"

#include <ctype.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
#include <fstream>

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
    std::ofstream out("time.txt");

    shmpath = (char*) malloc(strlen(SHM_PATH)+1); 
    if(shmpath == NULL) return -1;
    strcpy(shmpath, SHM_PATH); // get shared memory name

    cout<<"here"<<'\n';
    // lock process onto CPU
    CPU_SET(SERVER_CPU, &set);
    if (sched_setaffinity(getpid(), sizeof(set), &set) == -1) return -1;


    /* Create shared memory object and set its size to the size
    of our structure. */

    fd = shm_open(shmpath, O_CREAT | O_EXCL | O_RDWR, 0600);
    if (fd == -1)
        return -1;

    if (ftruncate(fd, sizeof(struct shmbuf)) == -1)
        return -1;

    cout<<"here"<<'\n';
    /* Map the object into the caller's address space. */

    shmp = static_cast<shmbuf*>(mmap(NULL, sizeof(*shmp), PROT_READ | PROT_WRITE,
                MAP_SHARED, fd, 0));


    if (shmp == MAP_FAILED)
        // errExit("mmap");
        return -1;


    pthread_barrierattr_t attr;
    pthread_barrierattr_init(&attr);
    pthread_barrierattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);

    unsigned long start, end1, sum;
    unsigned cycles_low, cycles_high, cycles_low1, cycles_high1;

    for(int iter = 0; iter < NUM_MEASUREMENTS; iter++) {

        //re init
        pthread_barrier_init(&shmp->barrier, &attr, NUM_CPU+1); 
        for(int i = 0; i < NUM_CPU; i++) {
            for(int j = 0; j < BUF_SIZE; j++) shmp->channels[i].buf[j] = 0;
        }
        sum = 0;
        int cnt = 0;

        // Create children processes
        for(int i = 0; i < 6; i++) {
            child_process(i);
        }

        pthread_barrier_wait(&shmp->barrier);


        // start time
        asm volatile ("cpuid\n\t"
        "rdtsc\n\t"
        "mov %%edx, %0\n\t"
        "mov %%eax, %1\n\t"
        : "=r" (cycles_high), "=r" (cycles_low)
        :: "%rax", "%rbx", "%rcx", "%rdx");

        // since the server side is the consumer (receiver) of messages, it will hold heads.
        int queue = 0;
        int caught = 0;
        int head[NUM_CPU] = {0};
        int total_msg_cnt = NUM_MSG * NUM_CPU;

        int qhead = 0;
        int qtail = 0;
        int q[BUF_SIZE] = {0};

        while(cnt < total_msg_cnt) {
            for(int i = 0; i < NUM_CPU; i++) {
                int cur = head[i];
                if(shmp->channels[i].buf[cur+CACHE_SIZE-1] == 0) {caught++;continue;} // this channel not ready yet

                // -- SIMPLE VERSION --
                // if (i%2 == 0 && queue == MAX_QUEUE) continue; // producer
                // if (i%2 == 1 && queue == 0) continue; // consumer

                // queue += (i%2==0) ? 1 : -1; // if producer, produce. if consumer, consume
                // shmp->channels[i].buf[cur+CACHE_SIZE-1] = 0; // set entry as empty, aka dequeue

                // --- ACTUAL QUEUE VERSION ---
                if(i%2 == 0) { // producer
                    if(qhead == qtail && q[qtail+CACHE_SIZE-1] == 255) continue; // queue is full
                    //produce
                    for(int j = qtail; j < qtail+CACHE_SIZE; j++) q[j] = 255;
                    qtail = (qtail+CACHE_SIZE)%BUF_SIZE;
                } else {
                    if(qhead == qtail && q[qhead+CACHE_SIZE-1] == 0) continue; // queue is empty
                    //consume
                    for(int j = qhead; j < qhead+CACHE_SIZE; j++) q[j] = 0;
                    qhead = (qhead+CACHE_SIZE)%BUF_SIZE;
                }

                cnt++;
                shmp->channels[i].buf[cur+CACHE_SIZE-1] = 0; // set entry as empty, aka dequeue
                head[i] = (cur+CACHE_SIZE)%BUF_SIZE; // move head
            }
        }

        asm volatile ("rdtscp\n\t"
        "mov %%edx, %0\n\t"
        "mov %%eax, %1\n\t"
        "cpuid\n\t"
        : "=r" (cycles_high1), "=r" (cycles_low1)
        :: "%rax", "%rbx", "%rcx", "%rdx");

        start = ( ((unsigned long)cycles_high << 32) | cycles_low );
        end1 = ( ((unsigned long)cycles_high1 << 32) | cycles_low1 );

        sum += end1-start;
        out<<sum<<'\n';

        pthread_barrier_destroy(&shmp->barrier);

    }



    // for sake of simplicity, the actual consumer producer "queue" will be a variable for now.



    shm_unlink(shmpath); // unlink the shared memory


    return 0;
}


