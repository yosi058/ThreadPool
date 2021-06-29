#include "threadPool.h"
#include "osqueue.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
// Yosef-Natan Berebi 208486365


// the function that all the threads get inside.
void *mission(void *tp) {
    ThreadPool *threadPool = (ThreadPool *) tp;
    //the first thread will lock the mutex.
    pthread_mutex_lock(&threadPool->m);
    while (threadPool->keep_do) {
        //if the queue is not empty.
        if (!osIsQueueEmpty(threadPool->queue_mission)) {
            function *func = osDequeue(threadPool->queue_mission);
            pthread_mutex_unlock(&threadPool->m);
            void (*computeFunc)(void *) = func->computeFunc;
            computeFunc(func->function_param);
            free(func);
            //lock the mutex again.
            pthread_mutex_lock(&threadPool->m);
            pthread_cond_signal(&threadPool->cond_threads);
        } else {
            pthread_cond_wait(&threadPool->cond_threads, &threadPool->m);
        }
    }
    // unlock the mutex.
    pthread_mutex_unlock(&threadPool->m);
    return NULL;
}
//init the thread-pool by dynamic allocate
ThreadPool *tpCreate(int numOfThreads) {
    ThreadPool *threadPool = (ThreadPool *) malloc(sizeof(ThreadPool));
    if (threadPool == NULL) {
        perror("Error in system call");
        free(threadPool);
        exit(-1);
    }
    threadPool->counter_threads = numOfThreads;
    threadPool->arr_threads = (pthread_t *) malloc((sizeof(pthread_t *)) * threadPool->counter_threads);
    if (threadPool->arr_threads == NULL) {
        perror("Error in system call");
        free(threadPool->arr_threads);
        free(threadPool);
        exit(-1);
    }
    // init the bool param
    threadPool->destroy_all = 0;
    threadPool->keep_do = 1;
    pthread_mutex_init(&threadPool->m, NULL);
    pthread_cond_init(&threadPool->cond_threads, NULL);
    threadPool->queue_mission = osCreateQueue();
    if (threadPool->queue_mission == NULL) {
        perror("Error in system call");
        osDestroyQueue(threadPool->queue_mission);
        free(threadPool->arr_threads);
        pthread_mutex_destroy(&threadPool->m);
        pthread_cond_destroy(&threadPool->cond_threads);
        free(threadPool);
        exit(-1);
    }
    int i = 0;
    //create the threads
    while (i < numOfThreads) {
        int thread = pthread_create(&(threadPool->arr_threads)[i], NULL, mission, (void *) threadPool);
        if (thread != 0) {
            perror("Error in system call");
        }
        i++;
    }
    return threadPool;
}


//insert a task to the queue
int tpInsertTask(ThreadPool *threadPool, void (*computeFunc)(void *), void *param) {
    pthread_mutex_lock(&threadPool->m);
    if (!threadPool->destroy_all) {
        pthread_mutex_unlock(&threadPool->m);
        function *func = (function *) malloc(sizeof(struct function));
        if (func == NULL) {
            perror("Error in system call");
            free(func);
            osDestroyQueue(threadPool->queue_mission);
            free(threadPool->arr_threads);
            pthread_mutex_destroy(&threadPool->m);
            pthread_cond_destroy(&threadPool->cond_threads);
            free(threadPool);
            exit(-1);
        }
        //insert to the struct function
        func->function_param = param;
        func->computeFunc = computeFunc;
        pthread_mutex_lock(&threadPool->m);
        osEnqueue(threadPool->queue_mission, func);
        pthread_mutex_unlock(&threadPool->m);
        //wake a single thread.
        pthread_cond_signal(&threadPool->cond_threads);
        return 0;
    }
    pthread_mutex_unlock(&threadPool->m);
    return -1;
}

//destory the thread poll
void tpDestroy(ThreadPool *threadPool, int shouldWaitForTasks) {
    pthread_mutex_lock(&threadPool->m);
    threadPool->destroy_all = 1;
    pthread_mutex_unlock(&threadPool->m);
    //if you should wait - so run all the missions.
    if (shouldWaitForTasks) {
        pthread_mutex_lock(&threadPool->m);
        while (!osIsQueueEmpty(threadPool->queue_mission)) {
            function *func = osDequeue(threadPool->queue_mission);
            pthread_mutex_unlock(&threadPool->m);
            void (*computeFunc)(void *) = func->computeFunc;
            computeFunc(func->function_param);
            free(func);
            pthread_mutex_lock(&threadPool->m);
        }
        threadPool->keep_do = 0;
        pthread_mutex_unlock(&threadPool->m);
    }

    pthread_mutex_lock(&threadPool->m);
    threadPool->keep_do = 0;
    pthread_mutex_unlock(&threadPool->m);
    pthread_cond_broadcast(&threadPool->cond_threads);
    int i;
    //wait for all the threads.
    for (i = 0; i < threadPool->counter_threads; i++) {
        pthread_join(threadPool->arr_threads[i], NULL);
    }
    pthread_mutex_lock(&threadPool->m);
    while (!osIsQueueEmpty(threadPool->queue_mission)) {
        function *func = (struct function *) osDequeue(threadPool->queue_mission);
        if (func != NULL) {
            free(func);
        }
    }
    //free all the memory.
    osDestroyQueue(threadPool->queue_mission);
    pthread_mutex_unlock(&threadPool->m);
    free(threadPool->arr_threads);
    pthread_mutex_destroy(&threadPool->m);
    pthread_cond_destroy(&threadPool->cond_threads);
    free(threadPool);
}



