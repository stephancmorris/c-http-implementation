/*
 * NanoServe - Thread Pool
 * Pool of worker threads for processing HTTP requests
 */

#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <pthread.h>
#include "queue.h"

/* Thread pool structure */
typedef struct {
    pthread_t *threads;         /* Array of worker threads */
    size_t num_threads;         /* Number of worker threads */
    queue_t *task_queue;        /* Shared task queue */
    int shutdown;               /* Shutdown flag */
} threadpool_t;

/*
 * Create thread pool with specified number of threads
 * num_threads: Number of worker threads to create
 * task_queue: Shared task queue
 * Returns pointer to thread pool on success, NULL on error
 */
threadpool_t *threadpool_create(size_t num_threads, queue_t *task_queue);

/*
 * Worker thread function (runs in each thread)
 * Continuously processes tasks from queue
 */
void *worker_thread(void *arg);

/*
 * Shutdown thread pool and wait for all threads to finish
 */
void threadpool_shutdown(threadpool_t *pool);

/*
 * Destroy thread pool and free resources
 */
void threadpool_destroy(threadpool_t *pool);

#endif /* THREADPOOL_H */
