/*
 * NanoServe - Thread-Safe Task Queue
 * FIFO queue for distributing client connections to worker threads
 */

#include "task_queue.h"
#include "logger.h"
#include <stdlib.h>
#include <errno.h>
#include <string.h>

/*
 * Initialize task queue
 * max_size: Maximum number of tasks (0 = unlimited)
 * Returns 0 on success, -1 on error
 */
int task_queue_init(task_queue_t *queue, int max_size) {
    if (queue == NULL) {
        LOG_ERROR(NULL, "task_queue_init: queue is NULL");
        return -1;
    }

    /* Initialize queue structure */
    queue->head = NULL;
    queue->tail = NULL;
    queue->count = 0;
    queue->max_size = max_size;
    queue->shutdown = false;

    /* Initialize mutex */
    if (pthread_mutex_init(&queue->mutex, NULL) != 0) {
        LOG_ERROR(NULL, "Failed to initialize queue mutex: %s", strerror(errno));
        return -1;
    }

    /* Initialize condition variable */
    if (pthread_cond_init(&queue->cond, NULL) != 0) {
        LOG_ERROR(NULL, "Failed to initialize queue condition variable: %s", strerror(errno));
        pthread_mutex_destroy(&queue->mutex);
        return -1;
    }

    LOG_DEBUG(NULL, "Task queue initialized (max_size=%d)", max_size);
    return 0;
}

/*
 * Enqueue a new task (client connection)
 * Blocks if queue is full (when max_size > 0)
 * Returns 0 on success, -1 on error
 */
int task_queue_enqueue(task_queue_t *queue, int client_fd) {
    if (queue == NULL) {
        LOG_ERROR(NULL, "task_queue_enqueue: queue is NULL");
        return -1;
    }

    /* Allocate new task */
    task_t *task = (task_t *)malloc(sizeof(task_t));
    if (task == NULL) {
        LOG_ERROR(NULL, "Failed to allocate task: %s", strerror(errno));
        return -1;
    }

    task->client_fd = client_fd;
    task->next = NULL;

    /* Lock mutex */
    pthread_mutex_lock(&queue->mutex);

    /* Check if shutdown */
    if (queue->shutdown) {
        pthread_mutex_unlock(&queue->mutex);
        free(task);
        LOG_WARN(NULL, "Cannot enqueue task: queue is shutting down");
        return -1;
    }

    /* Wait if queue is full (only when max_size > 0) */
    while (queue->max_size > 0 && queue->count >= queue->max_size) {
        LOG_DEBUG(NULL, "Queue full (%d/%d), waiting...", queue->count, queue->max_size);
        pthread_cond_wait(&queue->cond, &queue->mutex);

        /* Check shutdown again after waking up */
        if (queue->shutdown) {
            pthread_mutex_unlock(&queue->mutex);
            free(task);
            LOG_WARN(NULL, "Cannot enqueue task: queue is shutting down");
            return -1;
        }
    }

    /* Add task to queue (FIFO: add to tail) */
    if (queue->tail == NULL) {
        /* Queue is empty */
        queue->head = task;
        queue->tail = task;
    } else {
        /* Queue has items */
        queue->tail->next = task;
        queue->tail = task;
    }

    queue->count++;

    LOG_DEBUG(NULL, "Task enqueued (client_fd=%d, queue_size=%d)", client_fd, queue->count);

    /* Signal one waiting worker thread */
    pthread_cond_signal(&queue->cond);

    /* Unlock mutex */
    pthread_mutex_unlock(&queue->mutex);

    return 0;
}

/*
 * Dequeue a task (blocking)
 * Blocks until a task is available or shutdown is signaled
 * Returns client_fd on success, -1 on shutdown/error
 */
int task_queue_dequeue(task_queue_t *queue) {
    if (queue == NULL) {
        LOG_ERROR(NULL, "task_queue_dequeue: queue is NULL");
        return -1;
    }

    /* Lock mutex */
    pthread_mutex_lock(&queue->mutex);

    /* Wait for task or shutdown */
    while (queue->count == 0 && !queue->shutdown) {
        LOG_DEBUG(NULL, "Queue empty, worker waiting...");
        pthread_cond_wait(&queue->cond, &queue->mutex);
    }

    /* Check if shutdown with no tasks remaining */
    if (queue->shutdown && queue->count == 0) {
        pthread_mutex_unlock(&queue->mutex);
        LOG_DEBUG(NULL, "Queue shutdown, worker exiting");
        return -1;
    }

    /* Dequeue task from head (FIFO) */
    task_t *task = queue->head;
    queue->head = task->next;

    if (queue->head == NULL) {
        /* Queue is now empty */
        queue->tail = NULL;
    }

    queue->count--;
    int client_fd = task->client_fd;

    LOG_DEBUG(NULL, "Task dequeued (client_fd=%d, queue_size=%d)", client_fd, queue->count);

    /* Signal that queue has space (for bounded queues) */
    if (queue->max_size > 0) {
        pthread_cond_signal(&queue->cond);
    }

    /* Unlock mutex */
    pthread_mutex_unlock(&queue->mutex);

    /* Free task memory */
    free(task);

    return client_fd;
}

/*
 * Get current queue size (thread-safe)
 * Returns number of tasks in queue
 */
int task_queue_size(task_queue_t *queue) {
    if (queue == NULL) {
        return 0;
    }

    pthread_mutex_lock(&queue->mutex);
    int size = queue->count;
    pthread_mutex_unlock(&queue->mutex);

    return size;
}

/*
 * Signal shutdown and wake up all waiting threads
 * Returns 0 on success, -1 on error
 */
int task_queue_shutdown(task_queue_t *queue) {
    if (queue == NULL) {
        return -1;
    }

    pthread_mutex_lock(&queue->mutex);

    queue->shutdown = true;

    /* Wake up all waiting threads */
    pthread_cond_broadcast(&queue->cond);

    pthread_mutex_unlock(&queue->mutex);

    LOG_INFO(NULL, "Task queue shutdown signaled");
    return 0;
}

/*
 * Destroy task queue and free resources
 * Should be called after all threads have stopped
 */
void task_queue_destroy(task_queue_t *queue) {
    if (queue == NULL) {
        return;
    }

    /* Free any remaining tasks */
    task_t *current = queue->head;
    while (current != NULL) {
        task_t *next = current->next;
        LOG_WARN(NULL, "Dropping unprocessed task (client_fd=%d)", current->client_fd);
        free(current);
        current = next;
    }

    /* Destroy synchronization primitives */
    pthread_cond_destroy(&queue->cond);
    pthread_mutex_destroy(&queue->mutex);

    /* Reset queue */
    queue->head = NULL;
    queue->tail = NULL;
    queue->count = 0;

    LOG_INFO(NULL, "Task queue destroyed");
}
