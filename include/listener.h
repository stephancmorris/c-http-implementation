/*
 * NanoServe - TCP Socket Listener
 * Handles socket creation, binding, and listening for incoming connections
 */

#ifndef LISTENER_H
#define LISTENER_H

#include <stdint.h>

/* Listener configuration */
typedef struct {
    uint16_t port;          /* Port to bind to (default: 8080) */
    int backlog;            /* Connection backlog (default: 128) */
    int socket_fd;          /* Listener socket file descriptor */
} listener_t;

/*
 * Initialize listener with configuration
 * Returns 0 on success, -1 on error
 */
int listener_init(listener_t *listener, uint16_t port, int backlog);

/*
 * Start listening on the configured port
 * Returns 0 on success, -1 on error
 */
int listener_start(listener_t *listener);

/*
 * Accept incoming connection
 * Returns client socket FD on success, -1 on error
 */
int listener_accept(listener_t *listener);

/*
 * Close listener socket and cleanup
 */
void listener_destroy(listener_t *listener);

#endif /* LISTENER_H */
