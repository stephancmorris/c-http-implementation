/*
 * NanoServe - TCP Socket Listener
 * Handles socket creation, binding, and listening for incoming connections
 */

#include "listener.h"
#include "logger.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <arpa/inet.h>

/*
 * Initialize listener with configuration
 * Returns 0 on success, -1 on error
 */
int listener_init(listener_t *listener, uint16_t port, int backlog) {
    if (listener == NULL) {
        LOG_ERROR(NULL, "listener_init: listener is NULL");
        return -1;
    }

    /* Initialize struct fields */
    listener->port = port;
    listener->backlog = backlog;
    listener->socket_fd = -1;
    listener->shutdown_pipe[0] = -1;
    listener->shutdown_pipe[1] = -1;

    /* Create shutdown pipe (self-pipe trick) */
    if (pipe(listener->shutdown_pipe) < 0) {
        LOG_ERROR(NULL, "Failed to create shutdown pipe: %s", strerror(errno));
        return -1;
    }

    /* Make pipe non-blocking */
    int flags = fcntl(listener->shutdown_pipe[0], F_GETFL, 0);
    fcntl(listener->shutdown_pipe[0], F_SETFL, flags | O_NONBLOCK);

    LOG_DEBUG(NULL, "Listener initialized: port=%d, backlog=%d, shutdown_pipe=[%d,%d]",
              port, backlog, listener->shutdown_pipe[0], listener->shutdown_pipe[1]);
    return 0;
}

/*
 * Start listening on the configured port
 * Returns 0 on success, -1 on error
 */
int listener_start(listener_t *listener) {
    if (listener == NULL) {
        LOG_ERROR(NULL, "listener_start: listener is NULL");
        return -1;
    }

    /* Step 1: Create socket */
    LOG_DEBUG(NULL, "Creating TCP socket...");
    listener->socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listener->socket_fd < 0) {
        LOG_ERROR(NULL, "Failed to create socket: %s", strerror(errno));
        return -1;
    }
    LOG_DEBUG(NULL, "Socket created: fd=%d", listener->socket_fd);

    /* Step 2: Set SO_REUSEADDR option */
    /* This allows immediate reuse of the port after server restart */
    int opt = 1;
    if (setsockopt(listener->socket_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        LOG_WARN(NULL, "Failed to set SO_REUSEADDR: %s", strerror(errno));
        /* Not fatal, continue anyway */
    } else {
        LOG_DEBUG(NULL, "SO_REUSEADDR enabled");
    }

    /* Step 3: Prepare address structure */
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;  /* Bind to 0.0.0.0 (all interfaces) */
    server_addr.sin_port = htons(listener->port);  /* Convert to network byte order */

    /* Step 4: Bind socket to address */
    LOG_DEBUG(NULL, "Binding to 0.0.0.0:%d...", listener->port);
    if (bind(listener->socket_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        LOG_ERROR(NULL, "Failed to bind to port %d: %s", listener->port, strerror(errno));
        close(listener->socket_fd);
        listener->socket_fd = -1;
        return -1;
    }
    LOG_INFO(NULL, "Socket bound to 0.0.0.0:%d", listener->port);

    /* Step 5: Start listening */
    LOG_DEBUG(NULL, "Starting to listen (backlog=%d)...", listener->backlog);
    if (listen(listener->socket_fd, listener->backlog) < 0) {
        LOG_ERROR(NULL, "Failed to listen on socket: %s", strerror(errno));
        close(listener->socket_fd);
        listener->socket_fd = -1;
        return -1;
    }

    LOG_INFO(NULL, "Listening on port %d (backlog=%d)", listener->port, listener->backlog);
    return 0;
}

/*
 * Accept incoming connection (uses select() for interruptible wait)
 * Returns client socket FD on success, -1 on error, -2 on shutdown signal
 */
int listener_accept(listener_t *listener) {
    if (listener == NULL || listener->socket_fd < 0) {
        LOG_ERROR(NULL, "listener_accept: invalid listener");
        return -1;
    }

    /* Use select() to wait for either:
     * 1. Incoming connection on socket_fd
     * 2. Shutdown signal on shutdown_pipe[0]
     */
    fd_set read_fds;
    FD_ZERO(&read_fds);
    FD_SET(listener->socket_fd, &read_fds);
    FD_SET(listener->shutdown_pipe[0], &read_fds);

    int max_fd = (listener->socket_fd > listener->shutdown_pipe[0])
                  ? listener->socket_fd
                  : listener->shutdown_pipe[0];

    /* Wait indefinitely for activity */
    int ready = select(max_fd + 1, &read_fds, NULL, NULL, NULL);

    if (ready < 0) {
        if (errno == EINTR) {
            /* Interrupted by signal */
            LOG_DEBUG(NULL, "select() interrupted by signal");
            return -1;
        }
        LOG_ERROR(NULL, "select() failed: %s", strerror(errno));
        return -1;
    }

    /* Check if shutdown was signaled */
    if (FD_ISSET(listener->shutdown_pipe[0], &read_fds)) {
        char buf[1];
        read(listener->shutdown_pipe[0], buf, 1);  /* Drain pipe */
        LOG_DEBUG(NULL, "Shutdown signal received via pipe");
        return -2;  /* Special return code for shutdown */
    }

    /* Accept incoming connection */
    if (FD_ISSET(listener->socket_fd, &read_fds)) {
        struct sockaddr_in client_addr;
        socklen_t client_addr_len = sizeof(client_addr);

        int client_fd = accept(listener->socket_fd,
                               (struct sockaddr *)&client_addr,
                               &client_addr_len);

        if (client_fd < 0) {
            LOG_ERROR(NULL, "accept() failed: %s", strerror(errno));
            return -1;
        }

        /* Get client IP and port for logging */
        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, INET_ADDRSTRLEN);
        uint16_t client_port = ntohs(client_addr.sin_port);

        LOG_INFO(NULL, "Accepted connection from %s:%d (fd=%d)",
                 client_ip, client_port, client_fd);

        return client_fd;
    }

    /* Should not reach here */
    return -1;
}

/*
 * Signal listener to shutdown (wakes up accept())
 * Returns 0 on success, -1 on error
 */
int listener_shutdown(listener_t *listener) {
    if (listener == NULL || listener->shutdown_pipe[1] < 0) {
        return -1;
    }

    /* Write to pipe to wake up select() in listener_accept() */
    char signal = 1;
    if (write(listener->shutdown_pipe[1], &signal, 1) < 0) {
        LOG_ERROR(NULL, "Failed to write to shutdown pipe: %s", strerror(errno));
        return -1;
    }

    LOG_DEBUG(NULL, "Shutdown signal sent to listener");
    return 0;
}

/*
 * Close listener socket and cleanup
 */
void listener_destroy(listener_t *listener) {
    if (listener == NULL) {
        return;
    }

    /* Close listening socket */
    if (listener->socket_fd >= 0) {
        LOG_DEBUG(NULL, "Closing listener socket (fd=%d)", listener->socket_fd);
        close(listener->socket_fd);
        listener->socket_fd = -1;
    }

    /* Close shutdown pipe */
    if (listener->shutdown_pipe[0] >= 0) {
        close(listener->shutdown_pipe[0]);
        listener->shutdown_pipe[0] = -1;
    }
    if (listener->shutdown_pipe[1] >= 0) {
        close(listener->shutdown_pipe[1]);
        listener->shutdown_pipe[1] = -1;
    }

    LOG_INFO(NULL, "Listener destroyed");
}
