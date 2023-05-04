#include "os_threadpool.h"
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

/* === TASK === */

/* Creates a task that thread must execute */
os_task_t *task_create(void *arg, void (*f)(void *)) {
    os_task_t *task = malloc(sizeof(os_task_t));
    task->argument = arg;
    task->task = f;
    return task;
}

/* Add a new task to threadpool task queue */
void add_task_in_queue(os_threadpool_t *tp, os_task_t *t) {
    pthread_mutex_lock(&tp->taskLock);
    os_task_queue_t *task = malloc(sizeof(os_task_queue_t));
    task->task = t;
    task->next = tp->tasks;
    tp->tasks = task;
    pthread_mutex_unlock(&tp->taskLock);
}

/* Get the head of task queue from threadpool */
os_task_t *get_task(os_threadpool_t *tp) {
    if (tp->tasks == NULL) return NULL;
    os_task_queue_t *task = tp->tasks;
    tp->tasks = task->next;
    os_task_t *t = task->task;
    free(task);
    return t;
}

/* === THREAD POOL === */

/* Initialize the new threadpool */
os_threadpool_t *threadpool_create(unsigned int nThreads) {
    os_threadpool_t *pool = calloc(1, sizeof(os_threadpool_t));
    pool->num_threads = nThreads;
    pthread_mutex_init(&pool->taskLock, NULL);

    pool->threads = malloc(sizeof(pthread_t) * nThreads);
    int i, r;
    for (i = 0; i < nThreads; ++i) {
        r = pthread_create(&pool->threads[i], NULL, thread_loop_function, pool);
        if (r) {
            puts("Error creating pthread");
            exit(-1);
        }
    }

    return pool;
}

struct timespec tenMillis = { 0, 10000000L };

/* Loop function for threads */
void *thread_loop_function(void *args) {
    os_threadpool_t *tp = args;
    while (!tp->should_stop) {
        // Try to grab a new task
        pthread_mutex_lock(&tp->taskLock);
        os_task_t *task = get_task(tp);
        pthread_mutex_unlock(&tp->taskLock);
        // If there is no task, wait 10 millis before trying to grab task again
        if (task == NULL) nanosleep(&tenMillis, NULL);
        else {
            // Run task function
            task->task(task->argument);
        }
    }
    return NULL;
}

/* Stop the thread pool once a condition is met */
void threadpool_stop(os_threadpool_t *tp, int (*processingIsDone)(os_threadpool_t *)) {
    // Wait for processing to complete
    if (processingIsDone != NULL) {
        while (!processingIsDone(tp)) nanosleep(&tenMillis, NULL);
    }
    // Call for stop and wait for all threads to finish
    tp->should_stop = 1;
    int i, r;
    for (i = 0; i < tp->num_threads; ++i) {
        r = pthread_join(tp->threads[i], NULL);
        if (r) {
            puts("Error joining pthread");
            exit(-1);
        }
    }
    // Free threadpool
    free(tp->threads);
    pthread_mutex_destroy(&tp->taskLock);
    free(tp);
}
