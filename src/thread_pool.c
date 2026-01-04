/*
 * NanoServe - Thread Pool
 * Pool of worker threads for concurrent request processing
 */

#include "thread_pool.h"
#include "connection.h"
#include "logger.h"
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

/*
 * Worker thread function
 * Continuously dequeues and processes tasks until shutdown
 */
static void *worker_thread(void *arg) {
    thread_pool_t *pool = (thread_pool_t *)arg;

    LOG_INFO(NULL, "Worker thread %lu started", (unsigned long)pthread_self());

    while (!pool->shutdown) {
        /* Dequeue next task (blocks until task available) */
        int client_fd = task_queue_dequeue(pool->queue);

        if (client_fd < 0) {
            /* Shutdown signal received or error */
            if (pool->shutdown) {
                LOG_INFO(NULL, "Worker thread %lu shutting down", (unsigned long)pthread_self());
                break;
            }
            /* Error but not shutdown - log and continue */
            LOG_WARN(NULL, "Worker thread %lu: dequeue failed", (unsigned long)pthread_self());
            continue;
        }

        /* Process the client connection */
        LOG_DEBUG(NULL, "Worker thread %lu processing client_fd=%d",
                  (unsigned long)pthread_self(), client_fd);

        connection_handle(client_fd);

        /* Close the client connection */
        close(client_fd);

        LOG_DEBUG(NULL, "Worker thread %lu completed client_fd=%d",
                  (unsigned long)pthread_self(), client_fd);
    }

    LOG_INFO(NULL, "Worker thread %lu exited", (unsigned long)pthread_self());
    return NULL;
}

/*
 * Initialize thread pool
 * num_threads: Number of worker threads to create
 * queue: Pointer to shared task queue
 * Returns 0 on success, -1 on error
 */
int thread_pool_init(thread_pool_t *pool, int num_threads, task_queue_t *queue) {
    if (pool == NULL || queue == NULL) {
        LOG_ERROR(NULL, "thread_pool_init: invalid parameters");
        return -1;
    }

    if (num_threads <= 0) {
        LOG_ERROR(NULL, "thread_pool_init: invalid num_threads=%d", num_threads);
        return -1;
    }

    /* Allocate thread array */
    pool->threads = (pthread_t *)malloc(sizeof(pthread_t) * num_threads);
    if (pool->threads == NULL) {
        LOG_ERROR(NULL, "Failed to allocate thread array: %s", strerror(errno));
        return -1;
    }

    pool->num_threads = num_threads;
    pool->queue = queue;
    pool->shutdown = false;

    /* Initialize thread IDs to zero */
    for (int i = 0; i < num_threads; i++) {
        pool->threads[i] = 0;
    }

    LOG_INFO(NULL, "Thread pool initialized with %d threads", num_threads);
    return 0;
}

/*
 * Start all worker threads in the pool
 * Returns 0 on success, -1 on error
 */
int thread_pool_start(thread_pool_t *pool) {
    if (pool == NULL || pool->threads == NULL) {
        LOG_ERROR(NULL, "thread_pool_start: invalid pool");
        return -1;
    }

    LOG_INFO(NULL, "Starting %d worker threads...", pool->num_threads);

    /* Create all worker threads */
    for (int i = 0; i < pool->num_threads; i++) {
        if (pthread_create(&pool->threads[i], NULL, worker_thread, pool) != 0) {
            LOG_ERROR(NULL, "Failed to create worker thread %d: %s", i, strerror(errno));

            /* Mark as failed but continue creating others */
            pool->threads[i] = 0;
        } else {
            LOG_DEBUG(NULL, "Created worker thread %d (tid=%lu)",
                      i, (unsigned long)pool->threads[i]);
        }
    }

    LOG_INFO(NULL, "Thread pool started");
    return 0;
}

/*
 * Shutdown thread pool gracefully
 * Waits for all worker threads to finish
 * Returns 0 on success, -1 on error
 */
int thread_pool_shutdown(thread_pool_t *pool) {
    if (pool == NULL) {
        return -1;
    }

    LOG_INFO(NULL, "Shutting down thread pool...");

    /* Set shutdown flag */
    pool->shutdown = true;

    /* Shutdown the task queue (wakes up all waiting workers) */
    task_queue_shutdown(pool->queue);

    /* Wait for all worker threads to finish */
    int joined = 0;
    for (int i = 0; i < pool->num_threads; i++) {
        if (pool->threads[i] != 0) {
            LOG_DEBUG(NULL, "Waiting for worker thread %d (tid=%lu)...",
                      i, (unsigned long)pool->threads[i]);

            if (pthread_join(pool->threads[i], NULL) == 0) {
                joined++;
                LOG_DEBUG(NULL, "Worker thread %d joined successfully", i);
            } else {
                LOG_WARN(NULL, "Failed to join worker thread %d: %s", i, strerror(errno));
            }
        }
    }

    LOG_INFO(NULL, "Thread pool shutdown complete (%d/%d workers joined)",
             joined, pool->num_threads);

    return 0;
}

/*
 * Destroy thread pool and free resources
 * Should be called after shutdown completes
 */
void thread_pool_destroy(thread_pool_t *pool) {
    if (pool == NULL) {
        return;
    }

    /* Free thread array */
    if (pool->threads != NULL) {
        free(pool->threads);
        pool->threads = NULL;
    }

    pool->num_threads = 0;
    pool->queue = NULL;

    LOG_INFO(NULL, "Thread pool destroyed");
}
