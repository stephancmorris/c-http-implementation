/*
 * NanoServe - Thread-Safe Task Queue
 * FIFO queue with mutex protection and condition variables
 */

#ifndef QUEUE_H
#define QUEUE_H

#include <pthread.h>
#include <stddef.h>

/* Queue node structure */
typedef struct queue_node {
    int client_fd;              /* Client socket file descriptor */
    struct queue_node *next;    /* Next node in queue */
} queue_node_t;

/* Thread-safe queue structure */
typedef struct {
    queue_node_t *head;         /* Front of queue */
    queue_node_t *tail;         /* Back of queue */
    size_t size;                /* Current queue size */
    size_t max_size;            /* Maximum queue size (0 = unbounded) */
    pthread_mutex_t mutex;      /* Queue mutex */
    pthread_cond_t not_empty;   /* Condition: queue not empty */
    pthread_cond_t not_full;    /* Condition: queue not full */
    int shutdown;               /* Shutdown flag */
} queue_t;

/*
 * Initialize queue with maximum size
 * max_size: Maximum queue size (0 for unbounded)
 * Returns 0 on success, -1 on error
 */
int queue_init(queue_t *queue, size_t max_size);

/*
 * Push client FD to queue (blocking if full)
 * Returns 0 on success, -1 on error
 */
int queue_push(queue_t *queue, int client_fd);

/*
 * Pop client FD from queue (blocking if empty)
 * Returns client FD on success, -1 on error or shutdown
 */
int queue_pop(queue_t *queue);

/*
 * Get current queue size
 */
size_t queue_size(queue_t *queue);

/*
 * Check if queue is full
 */
int queue_is_full(queue_t *queue);

/*
 * Signal shutdown and wake all waiting threads
 */
void queue_shutdown(queue_t *queue);

/*
 * Destroy queue and free resources
 */
void queue_destroy(queue_t *queue);

#endif /* QUEUE_H */
