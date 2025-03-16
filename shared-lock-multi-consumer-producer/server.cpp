#include "./pshm.h"

#include <ctype.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fstream>

#include <iostream>


using namespace std;

int child_process(int cpu_id) {

    int process_id = fork();
    if(process_id == 0) { // child
        int            fd;
        char           *shmpath;
        cpu_set_t     set;
        size_t         len;
        struct shmbuf  *shmp;

        // lock process onto CPU
        CPU_SET(cpu_id, &set);
        if (sched_setaffinity(getpid(), sizeof(set), &set) == -1) return -1;

        shmpath = (char*) malloc(strlen(SHM_PATH)+1);
        if(shmpath == NULL) return -1;
        strcpy(shmpath, SHM_PATH);

        /* Open the existing shared memory object and map it
        into the caller's address space. */
        fd = shm_open(shmpath, O_RDWR, 0);
        if (fd == -1) return -1; 

        shmp = static_cast<shmbuf*>(mmap(NULL, sizeof(*shmp), PROT_READ | PROT_WRITE,
            MAP_SHARED, fd, 0));
        if (shmp == MAP_FAILED) return -1;

        pthread_barrier_wait(&shmp->barrier); // synchronization barrier

        int cnt = 0;
        int caught = 0;
        if(cpu_id % 2 == 0) { // producer
            while(cnt < NUM_MSG) {
                    pthread_mutex_lock(&shmp->mutex); // get lock
                    if(shmp->head == shmp->tail && shmp->buf[shmp->tail] == 255) { // queue is full
                        caught++;
                        pthread_mutex_unlock(&shmp->mutex);
                        continue;
                    }
                    for(int i = shmp->tail; i < shmp->tail+CACHE_SIZE; i++) shmp->buf[i] = 255; // produce, or write
                    shmp->tail = (shmp->tail+CACHE_SIZE)%BUF_SIZE;
                    cnt++;
                    pthread_mutex_unlock(&shmp->mutex);
            }
        } else { // consumer
            while(cnt < NUM_MSG) {
                    pthread_mutex_lock(&shmp->mutex); // get lock
                    if(shmp->head == shmp->tail && shmp->buf[shmp->head] == 0) { // queue is empty 
                        caught++;
                        pthread_mutex_unlock(&shmp->mutex);
                        continue;
                    }
                    for(int i = shmp->head; i < shmp->head+CACHE_SIZE; i++) shmp->buf[i] = 0; // consume
                    shmp->head = (shmp->head+CACHE_SIZE)%BUF_SIZE;
                    cnt++;
                    pthread_mutex_unlock(&shmp->mutex);
            }
        }
        // cout<<cnt<<'\n';
        // cout<<caught<<'\n';
        exit(0);
    }
    else return process_id; // parent
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

    // lock process onto CPU
    CPU_SET(SERVER_CPU, &set);
    if (sched_setaffinity(getpid(), sizeof(set), &set) == -1) {return -1;}


    /* Create shared memory object and set its size to the size
    of our structure. */

    fd = shm_open(shmpath, O_CREAT | O_EXCL | O_RDWR, 0600);
    if (fd == -1) {cout<<"wher\n";return -1;}
        // errExit("shm_open");

    if (ftruncate(fd, sizeof(struct shmbuf)) == -1) {return -1;}
        // errExit("ftruncate");

    /* Map the object into the caller's address space. */

    shmp = static_cast<shmbuf*>(mmap(NULL, sizeof(*shmp), PROT_READ | PROT_WRITE,
                MAP_SHARED, fd, 0));


    if (shmp == MAP_FAILED) return -1;
        // errExit("mmap");


    pthread_barrierattr_t attr;
    pthread_barrierattr_init(&attr);
    pthread_barrierattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);

    pthread_mutexattr_t m_attr;
    pthread_mutexattr_init(&m_attr);
    pthread_mutexattr_setpshared(&m_attr, PTHREAD_PROCESS_SHARED);
    pthread_mutex_init(&(shmp->mutex), &m_attr);
    pthread_mutexattr_destroy(&m_attr);

    unsigned long start, end1, sum;
    unsigned cycles_low, cycles_high, cycles_low1, cycles_high1;

    for(int iter = 0; iter < NUM_MEASUREMENTS; iter++) {

        //re init
        shmp->head = 0;
        shmp->tail = 0;
        for(int i = 0; i < BUF_SIZE; i++) shmp->buf[i] = 0;
        shmp->queue = 0;
        sum = 0;

        // Create children processes
        pthread_barrier_init(&(shmp->barrier), &attr, NUM_CPU+1); 

        int child_id[NUM_CPU] = {0};

        for(int i = 0; i < NUM_CPU; i++) {
            child_id[i] = child_process(i);
        }

        pthread_barrier_wait(&shmp->barrier);

        // start time
        asm volatile ("cpuid\n\t"
        "rdtsc\n\t"
        "mov %%edx, %0\n\t"
        "mov %%eax, %1\n\t"
        : "=r" (cycles_high), "=r" (cycles_low)
        :: "%rax", "%rbx", "%rcx", "%rdx");

        // call wait on all child processes
        while(wait(NULL) != -1);

        // end time
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

    pthread_mutex_destroy(&shmp->mutex);
    shm_unlink(shmpath); // unlink the shared memory


    return 0;
}


