/*
 * NanoServe - High-Reliability Idempotent HTTP Server
 * Main entry point
 */

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include "logger.h"
#include "listener.h"

/* Global listener for signal handler */
static listener_t g_listener;
static volatile sig_atomic_t g_running = 1;

/*
 * Signal handler for graceful shutdown
 */
void signal_handler(int signum) {
    (void)signum;
    LOG_INFO(NULL, "Shutdown signal received...");
    g_running = 0;
}

int main(int argc, char *argv[]) {
    /* Suppress unused parameter warnings */
    (void)argc;
    (void)argv;

    /* Initialize logger with DEBUG level to see all messages */
    logger_set_level(LOG_DEBUG);

    LOG_INFO(NULL, "NanoServe v2.0 - Starting...");
    LOG_INFO(NULL, "High-Reliability Idempotent HTTP Server");

    /* Register signal handlers for graceful shutdown */
    signal(SIGINT, signal_handler);   /* Ctrl+C */
    signal(SIGTERM, signal_handler);  /* kill command */

    /* Initialize listener on port 8080 with backlog of 128 */
    if (listener_init(&g_listener, 8080, 128) < 0) {
        LOG_ERROR(NULL, "Failed to initialize listener");
        return EXIT_FAILURE;
    }

    /* Start listening for connections */
    if (listener_start(&g_listener) < 0) {
        LOG_ERROR(NULL, "Failed to start listener");
        return EXIT_FAILURE;
    }

    LOG_INFO(NULL, "Server initialization complete");
    LOG_INFO(NULL, "Press Ctrl+C to shutdown");

    /* Main server loop - accept connections */
    while (g_running) {
        LOG_DEBUG(NULL, "Waiting for incoming connection...");

        int client_fd = listener_accept(&g_listener);

        if (client_fd < 0) {
            /* Error or interrupted - check if we should continue */
            if (!g_running) {
                break;
            }
            continue;
        }

        /* Connection accepted - for now, just close it immediately */
        /* TODO: Hand off to thread pool for processing */
        LOG_DEBUG(NULL, "Closing client connection (fd=%d) - no handler yet", client_fd);
        close(client_fd);
    }

    /* Cleanup */
    LOG_INFO(NULL, "Shutting down server...");
    listener_destroy(&g_listener);
    LOG_INFO(NULL, "Server shutdown complete");

    return EXIT_SUCCESS;
}
