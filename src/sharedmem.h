#ifndef PSHM_UCASE_H
#define PSHM_UCASE_H

#include <semaphore.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#define BUF_SIZE 1024

#define errExit(msg) { perror(msg); exit(EXIT_FAILURE); }

struct shmbuf {
    sem_t sem;
    size_t cnt;
    char buf[BUF_SIZE];
};

#endif