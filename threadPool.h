#ifndef __THREAD_POOL__
#define __THREAD_POOL__

#include "osqueue.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

// Yosef-Natan Berebi 208486365
typedef struct thread_pool {
    OSQueue *queue_mission;
    pthread_mutex_t m;
    pthread_cond_t cond_threads;
    pthread_t *arr_threads;
    int counter_threads;
    int destroy_all;
    int keep_do;
} ThreadPool;

typedef struct function {
    void (*computeFunc)(void *);

    void *function_param;
} function;

ThreadPool *tpCreate(int numOfThreads);

void tpDestroy(ThreadPool *threadPool, int shouldWaitForTasks);

int tpInsertTask(ThreadPool *threadPool, void (*computeFunc)(void *), void *param);

#endif
