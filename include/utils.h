/*
 * NanoServe - Utility Functions
 * Helper functions used across the codebase
 */

#ifndef UTILS_H
#define UTILS_H

#include <stddef.h>

/*
 * String utilities
 */

/* Trim whitespace from both ends of string (in-place) */
char *str_trim(char *str);

/* Case-insensitive string comparison */
int str_casecmp(const char *s1, const char *s2);

/* Safe string copy with size limit */
char *str_safe_copy(const char *src, size_t max_len);

/*
 * Path utilities
 */

/* Check if path contains directory traversal attempts */
int path_has_traversal(const char *path);

/* Join two paths safely */
char *path_join(const char *base, const char *path);

/*
 * Network utilities
 */

/* Get client IP address from socket */
int get_client_addr(int socket_fd, char *ip_str, size_t ip_len, uint16_t *port);

/*
 * Time utilities
 */

/* Get current timestamp in HTTP date format */
char *get_http_date(void);

/* Get current timestamp for logging */
char *get_timestamp(void);

#endif /* UTILS_H */
