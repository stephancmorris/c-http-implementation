/*
 * NanoServe - Connection Dispatcher
 * Accepts connections and distributes them to worker threads via queue
 */

#ifndef DISPATCHER_H
#define DISPATCHER_H

#include "listener.h"
#include "queue.h"

/* Dispatcher configuration */
typedef struct {
    listener_t *listener;   /* TCP listener */
    queue_t *task_queue;    /* Task queue for worker threads */
    int running;            /* Dispatcher running flag */
} dispatcher_t;

/*
 * Initialize dispatcher with listener and queue
 * Returns 0 on success, -1 on error
 */
int dispatcher_init(dispatcher_t *dispatcher, listener_t *listener, queue_t *queue);

/*
 * Run dispatcher loop (accepts connections and enqueues them)
 * This is a blocking call that runs until shutdown
 */
void dispatcher_run(dispatcher_t *dispatcher);

/*
 * Stop dispatcher loop
 */
void dispatcher_stop(dispatcher_t *dispatcher);

/*
 * Cleanup dispatcher resources
 */
void dispatcher_destroy(dispatcher_t *dispatcher);

#endif /* DISPATCHER_H */
