/*
 * NanoServe - High-Reliability Idempotent HTTP Server
 * Main entry point
 */

#include <stdio.h>
#include <stdlib.h>
#include "logger.h"

int main(int argc, char *argv[]) {
    /* Suppress unused parameter warnings */
    (void)argc;
    (void)argv;

    /* Initialize logger (use global logger with INFO level) */
    logger_set_level(LOG_INFO);

    LOG_INFO(NULL, "NanoServe v2.0 - Starting...");
    LOG_INFO(NULL, "High-Reliability Idempotent HTTP Server");
    LOG_DEBUG(NULL, "This is a debug message (won't show at INFO level)");

    // TODO: Initialize components
    // - Thread Pool
    // - Listener
    // - Dispatcher

    LOG_INFO(NULL, "Server initialization complete");
    LOG_INFO(NULL, "Press Ctrl+C to shutdown");

    /* Test different log levels */
    LOG_WARN(NULL, "This is a warning message");
    LOG_ERROR(NULL, "This is an error message");

    /* Test with DEBUG level */
    LOG_INFO(NULL, "Switching to DEBUG log level...");
    logger_set_level(LOG_DEBUG);
    LOG_DEBUG(NULL, "Now you can see debug messages!");

    return EXIT_SUCCESS;
}
