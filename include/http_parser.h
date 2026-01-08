/*
 * C-HTTP Payment Server - HTTP Request Parser
 * Parses HTTP/1.1 requests with POST body and idempotency key support
 */

#ifndef HTTP_PARSER_H
#define HTTP_PARSER_H

#include <stddef.h>
#include <stdbool.h>

/* Maximum limits for request components */
#define MAX_HEADERS 64
#define MAX_URI_LENGTH 2048
#define MAX_HEADER_NAME_LENGTH 256
#define MAX_HEADER_VALUE_LENGTH 8192
#define MAX_IDEMPOTENCY_KEY_LENGTH 256
#define MAX_REQUEST_BODY_SIZE (1024 * 1024)  /* 1MB */

/*
 * HTTP Method Enumeration
 * Supported HTTP methods for request parsing
 */
typedef enum {
    HTTP_METHOD_UNKNOWN = 0,
    HTTP_METHOD_GET,
    HTTP_METHOD_POST,
    HTTP_METHOD_PUT,
    HTTP_METHOD_DELETE,
    HTTP_METHOD_HEAD,
    HTTP_METHOD_OPTIONS,
    HTTP_METHOD_PATCH
} http_method_t;

/*
 * HTTP Version Enumeration
 * Supported HTTP protocol versions
 */
typedef enum {
    HTTP_VERSION_UNKNOWN = 0,
    HTTP_VERSION_1_0,
    HTTP_VERSION_1_1
} http_version_t;

/*
 * HTTP Header Structure
 * Represents a single HTTP header name-value pair
 * Uses fixed-size arrays for stack allocation
 */
typedef struct {
    char name[MAX_HEADER_NAME_LENGTH];
    char value[MAX_HEADER_VALUE_LENGTH];
} http_header_t;

/*
 * HTTP Request Structure
 * Contains all parsed information from an HTTP request
 * Supports POST bodies and idempotency keys for distributed systems
 */
typedef struct {
    /* Request Line */
    http_method_t method;
    char uri[MAX_URI_LENGTH];
    http_version_t version;

    /* Headers (fixed array for stack allocation) */
    http_header_t headers[MAX_HEADERS];
    int header_count;

    /* Request Body (heap-allocated for POST/PUT) */
    char *body;
    size_t body_length;

    /* Content-Length (extracted from header) */
    size_t content_length;

    /* Idempotency Key (extracted from X-Idempotency-Key header) */
    char idempotency_key[MAX_IDEMPOTENCY_KEY_LENGTH];
    bool has_idempotency_key;
} http_request_t;

/*
 * Initialize an HTTP request structure
 * Zeros out all fields and sets defaults
 * Returns 0 on success, -1 on error
 */
int http_request_init(http_request_t *request);

/*
 * Free an HTTP request structure
 * Deallocates dynamically allocated memory (body, etc.)
 */
void http_request_free(http_request_t *request);

/*
 * Parse complete HTTP request from raw data
 * Returns 0 on success, -1 on error
 */
int http_parse_request(http_request_t *request, const char *raw_request, size_t length);

/*
 * Parse request line (method, URI, version)
 * Example: "POST /api/payment HTTP/1.1"
 * Returns 0 on success, -1 on error
 */
int parse_request_line(http_request_t *request, const char *line);

/*
 * Parse HTTP headers section
 * Returns 0 on success, -1 on error
 */
int parse_headers(http_request_t *request, const char *header_section);

/*
 * Get header value by name (case-insensitive)
 * Returns pointer to value string, or NULL if not found
 */
const char *http_get_header(const http_request_t *request, const char *name);

/*
 * Convert HTTP method enum to string
 * Returns string representation (e.g., "GET", "POST")
 */
const char *http_method_to_string(http_method_t method);

/*
 * Convert string to HTTP method enum
 * Returns HTTP_METHOD_UNKNOWN if not recognized
 */
http_method_t http_string_to_method(const char *method_str);

/*
 * Convert HTTP version enum to string
 * Returns string representation (e.g., "HTTP/1.1")
 */
const char *http_version_to_string(http_version_t version);

#endif /* HTTP_PARSER_H */
