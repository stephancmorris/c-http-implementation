/*
 * NanoServe - Logging Subsystem
 * Thread-safe logging with multiple log levels
 */

#include "logger.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <time.h>
#include <string.h>

/* Global logger instance */
static logger_t g_logger = {
    .min_level = LOG_INFO,
    .mutex = PTHREAD_MUTEX_INITIALIZER
};

/*
 * Initialize logger with minimum log level
 */
int logger_init(logger_t *logger, log_level_t min_level) {
    if (logger == NULL) {
        return -1;
    }

    logger->min_level = min_level;

    /* Initialize mutex */
    if (pthread_mutex_init(&logger->mutex, NULL) != 0) {
        fprintf(stderr, "Failed to initialize logger mutex\n");
        return -1;
    }

    return 0;
}

/*
 * Get log level name as string
 */
const char *log_level_to_string(log_level_t level) {
    switch (level) {
        case LOG_DEBUG: return "DEBUG";
        case LOG_INFO:  return "INFO";
        case LOG_WARN:  return "WARN";
        case LOG_ERROR: return "ERROR";
        default:        return "UNKNOWN";
    }
}

/*
 * Get current timestamp in format: YYYY-MM-DD HH:MM:SS
 */
static void get_timestamp(char *buffer, size_t buffer_size) {
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);

    strftime(buffer, buffer_size, "%Y-%m-%d %H:%M:%S", tm_info);
}

/*
 * Log a message with specified level (thread-safe)
 */
void log_message(logger_t *logger, log_level_t level, const char *format, ...) {
    /* Use global logger if none provided */
    if (logger == NULL) {
        logger = &g_logger;
    }

    /* Check if this message should be logged */
    if (level < logger->min_level) {
        return;
    }

    /* Lock mutex for thread-safe logging */
    pthread_mutex_lock(&logger->mutex);

    /* Get timestamp */
    char timestamp[32];
    get_timestamp(timestamp, sizeof(timestamp));

    /* Print timestamp and log level */
    fprintf(stderr, "[%s] [%s] ", timestamp, log_level_to_string(level));

    /* Print formatted message */
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);

    /* Add newline */
    fprintf(stderr, "\n");

    /* Flush to ensure immediate output */
    fflush(stderr);

    /* Unlock mutex */
    pthread_mutex_unlock(&logger->mutex);
}

/*
 * Cleanup logger resources
 */
void logger_destroy(logger_t *logger) {
    if (logger == NULL) {
        return;
    }

    pthread_mutex_destroy(&logger->mutex);
}

/*
 * Get global logger instance
 */
logger_t *logger_get_global(void) {
    return &g_logger;
}

/*
 * Set global logger minimum level
 */
void logger_set_level(log_level_t level) {
    g_logger.min_level = level;
}
