/*
 * NanoServe - Logging Subsystem
 * Thread-safe logging with multiple log levels
 */

#ifndef LOGGER_H
#define LOGGER_H

#include <pthread.h>

/* Log levels */
typedef enum {
    LOG_DEBUG,
    LOG_INFO,
    LOG_WARN,
    LOG_ERROR
} log_level_t;

/* Logger configuration */
typedef struct {
    log_level_t min_level;      /* Minimum log level to display */
    pthread_mutex_t mutex;      /* Mutex for thread-safe logging */
} logger_t;

/*
 * Initialize logger with minimum log level
 * Returns 0 on success, -1 on error
 */
int logger_init(logger_t *logger, log_level_t min_level);

/*
 * Log a message with specified level
 * Thread-safe
 */
void log_message(logger_t *logger, log_level_t level, const char *format, ...);

/*
 * Convenience macros for logging
 */
#define LOG_DEBUG(logger, ...) log_message(logger, LOG_DEBUG, __VA_ARGS__)
#define LOG_INFO(logger, ...)  log_message(logger, LOG_INFO, __VA_ARGS__)
#define LOG_WARN(logger, ...)  log_message(logger, LOG_WARN, __VA_ARGS__)
#define LOG_ERROR(logger, ...) log_message(logger, LOG_ERROR, __VA_ARGS__)

/*
 * Get log level name as string
 */
const char *log_level_to_string(log_level_t level);

/*
 * Cleanup logger resources
 */
void logger_destroy(logger_t *logger);

#endif /* LOGGER_H */
