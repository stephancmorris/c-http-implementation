/*
 * C-HTTP Payment Server - Thread-Safe Task Queue
 * FIFO queue for distributing client connections to worker threads
 */

#ifndef TASK_QUEUE_H
#define TASK_QUEUE_H

#include <pthread.h>
#include <stdbool.h>

/* Task representing a client connection to be processed */
typedef struct task {
    int client_fd;          /* Client socket file descriptor */
    struct task *next;      /* Pointer to next task in queue */
} task_t;

/* Thread-safe FIFO task queue */
typedef struct {
    task_t *head;           /* Head of queue (dequeue from here) */
    task_t *tail;           /* Tail of queue (enqueue here) */
    int count;              /* Number of tasks in queue */
    int max_size;           /* Maximum queue size (0 = unlimited) */
    bool shutdown;          /* Shutdown flag */

    pthread_mutex_t mutex;  /* Mutex for thread safety */
    pthread_cond_t cond;    /* Condition variable for signaling workers */
} task_queue_t;

/*
 * Initialize task queue
 * max_size: Maximum number of tasks (0 = unlimited)
 * Returns 0 on success, -1 on error
 */
int task_queue_init(task_queue_t *queue, int max_size);

/*
 * Enqueue a new task (client connection)
 * Blocks if queue is full (when max_size > 0)
 * Returns 0 on success, -1 on error
 */
int task_queue_enqueue(task_queue_t *queue, int client_fd);

/*
 * Dequeue a task (blocking)
 * Blocks until a task is available or shutdown is signaled
 * Returns client_fd on success, -1 on shutdown/error
 */
int task_queue_dequeue(task_queue_t *queue);

/*
 * Get current queue size (thread-safe)
 * Returns number of tasks in queue
 */
int task_queue_size(task_queue_t *queue);

/*
 * Signal shutdown and wake up all waiting threads
 * Returns 0 on success, -1 on error
 */
int task_queue_shutdown(task_queue_t *queue);

/*
 * Destroy task queue and free resources
 * Should be called after all threads have stopped
 */
void task_queue_destroy(task_queue_t *queue);

#endif /* TASK_QUEUE_H */
