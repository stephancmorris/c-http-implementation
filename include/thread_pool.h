/*
 * NanoServe - Thread Pool
 * Pool of worker threads for concurrent request processing
 */

#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include <pthread.h>
#include <stdbool.h>
#include "task_queue.h"

/* Default number of worker threads */
#define DEFAULT_THREAD_POOL_SIZE 10

/* Thread pool managing worker threads */
typedef struct {
    pthread_t *threads;     /* Array of worker thread IDs */
    int num_threads;        /* Number of threads in pool */
    task_queue_t *queue;    /* Shared task queue */
    bool shutdown;          /* Shutdown flag */
} thread_pool_t;

/*
 * Initialize thread pool
 * num_threads: Number of worker threads to create
 * queue: Pointer to shared task queue
 * Returns 0 on success, -1 on error
 */
int thread_pool_init(thread_pool_t *pool, int num_threads, task_queue_t *queue);

/*
 * Start all worker threads in the pool
 * Returns 0 on success, -1 on error
 */
int thread_pool_start(thread_pool_t *pool);

/*
 * Shutdown thread pool gracefully
 * Waits for all worker threads to finish
 * Returns 0 on success, -1 on error
 */
int thread_pool_shutdown(thread_pool_t *pool);

/*
 * Destroy thread pool and free resources
 * Should be called after shutdown completes
 */
void thread_pool_destroy(thread_pool_t *pool);

#endif /* THREAD_POOL_H */
