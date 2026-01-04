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
#include "connection.h"
#include "task_queue.h"
#include "thread_pool.h"

/* Global components for signal handler */
static listener_t g_listener;
static task_queue_t g_queue;
static thread_pool_t g_pool;
static volatile sig_atomic_t g_running = 1;

/*
 * Signal handler for graceful shutdown
 */
void signal_handler(int signum) {
    (void)signum;
    LOG_INFO(NULL, "Shutdown signal received...");
    g_running = 0;

    /* Wake up listener_accept() immediately */
    listener_shutdown(&g_listener);
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

    /* Initialize task queue (unlimited size) */
    if (task_queue_init(&g_queue, 0) < 0) {
        LOG_ERROR(NULL, "Failed to initialize task queue");
        return EXIT_FAILURE;
    }

    /* Initialize thread pool with 10 worker threads */
    if (thread_pool_init(&g_pool, 10, &g_queue) < 0) {
        LOG_ERROR(NULL, "Failed to initialize thread pool");
        task_queue_destroy(&g_queue);
        return EXIT_FAILURE;
    }

    /* Start worker threads */
    if (thread_pool_start(&g_pool) < 0) {
        LOG_ERROR(NULL, "Failed to start thread pool");
        thread_pool_destroy(&g_pool);
        task_queue_destroy(&g_queue);
        return EXIT_FAILURE;
    }

    /* Initialize listener on port 8080 with backlog of 128 */
    if (listener_init(&g_listener, 8080, 128) < 0) {
        LOG_ERROR(NULL, "Failed to initialize listener");
        thread_pool_shutdown(&g_pool);
        thread_pool_destroy(&g_pool);
        task_queue_destroy(&g_queue);
        return EXIT_FAILURE;
    }

    /* Start listening for connections */
    if (listener_start(&g_listener) < 0) {
        LOG_ERROR(NULL, "Failed to start listener");
        listener_destroy(&g_listener);
        thread_pool_shutdown(&g_pool);
        thread_pool_destroy(&g_pool);
        task_queue_destroy(&g_queue);
        return EXIT_FAILURE;
    }

    LOG_INFO(NULL, "Server initialization complete");
    LOG_INFO(NULL, "Press Ctrl+C to shutdown");

    /* Main server loop - accept connections */
    while (g_running) {
        LOG_DEBUG(NULL, "Waiting for incoming connection...");

        int client_fd = listener_accept(&g_listener);

        if (client_fd == -2) {
            /* Shutdown signal received via pipe */
            LOG_DEBUG(NULL, "Shutdown requested, exiting accept loop");
            break;
        }

        if (client_fd < 0) {
            /* Error or interrupted - check if we should continue */
            if (!g_running) {
                break;
            }
            continue;
        }

        /* Connection accepted - enqueue for thread pool processing */
        if (task_queue_enqueue(&g_queue, client_fd) < 0) {
            LOG_ERROR(NULL, "Failed to enqueue client_fd=%d, closing connection", client_fd);
            close(client_fd);
        }
    }

    /* Cleanup */
    LOG_INFO(NULL, "Shutting down server...");

    /* Shutdown thread pool and wait for workers to finish */
    thread_pool_shutdown(&g_pool);
    thread_pool_destroy(&g_pool);

    /* Destroy task queue */
    task_queue_destroy(&g_queue);

    /* Destroy listener */
    listener_destroy(&g_listener);

    LOG_INFO(NULL, "Server shutdown complete");

    return EXIT_SUCCESS;
}
