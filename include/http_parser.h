/*
 * NanoServe - HTTP Request Parser
 * Parses HTTP/1.1 requests
 */

#ifndef HTTP_PARSER_H
#define HTTP_PARSER_H

#include <stddef.h>

/* HTTP methods */
typedef enum {
    HTTP_GET,
    HTTP_POST,
    HTTP_HEAD,
    HTTP_PUT,
    HTTP_DELETE,
    HTTP_UNKNOWN
} http_method_t;

/* HTTP header */
typedef struct {
    char *name;
    char *value;
} http_header_t;

/* HTTP request structure */
typedef struct {
    http_method_t method;       /* Request method */
    char *uri;                  /* Request URI */
    char *version;              /* HTTP version (1.0, 1.1) */
    http_header_t *headers;     /* Array of headers */
    size_t header_count;        /* Number of headers */
    char *body;                 /* Request body (for POST, etc.) */
    size_t body_length;         /* Body length */
} http_request_t;

/*
 * Initialize HTTP request structure
 * Returns 0 on success, -1 on error
 */
int http_request_init(http_request_t *request);

/*
 * Parse HTTP request from raw data
 * Returns 0 on success, -1 on error
 */
int http_parse_request(http_request_t *request, const char *raw_request, size_t length);

/*
 * Parse request line (method, URI, version)
 * Returns 0 on success, -1 on error
 */
int parse_request_line(http_request_t *request, const char *line);

/*
 * Parse HTTP headers
 * Returns 0 on success, -1 on error
 */
int parse_headers(http_request_t *request, const char *header_section);

/*
 * Get header value by name (case-insensitive)
 * Returns header value or NULL if not found
 */
const char *http_get_header(const http_request_t *request, const char *name);

/*
 * Free HTTP request resources
 */
void http_request_free(http_request_t *request);

#endif /* HTTP_PARSER_H */
