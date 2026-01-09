/*
 * C-HTTP Payment Server - HTTP Response Builder
 * Builds HTTP/1.1 responses
 */

#ifndef HTTP_RESPONSE_H
#define HTTP_RESPONSE_H

#include <stddef.h>

/* HTTP status codes */
#define HTTP_OK                  200
#define HTTP_BAD_REQUEST         400
#define HTTP_NOT_FOUND           404
#define HTTP_CONFLICT            409
#define HTTP_PAYLOAD_TOO_LARGE   413
#define HTTP_UNPROCESSABLE       422
#define HTTP_INTERNAL_ERROR      500
#define HTTP_NOT_IMPLEMENTED     501

/* HTTP response structure */
typedef struct {
    int status_code;            /* HTTP status code */
    char *status_message;       /* Status message */
    char **header_names;        /* Header names */
    char **header_values;       /* Header values */
    size_t header_count;        /* Number of headers */
    size_t header_capacity;     /* Header array capacity */
    char *body;                 /* Response body */
    size_t body_length;         /* Body length */
} http_response_t;

/*
 * Initialize HTTP response with status code
 * Returns 0 on success, -1 on error
 */
int http_response_init(http_response_t *response, int status_code);

/*
 * Add header to response
 * Returns 0 on success, -1 on error
 */
int http_response_add_header(http_response_t *response, const char *name, const char *value);

/*
 * Set response body
 * Returns 0 on success, -1 on error
 */
int http_response_set_body(http_response_t *response, const char *body, size_t length);

/*
 * Serialize response to string (ready to send)
 * Returns serialized response string (caller must free), NULL on error
 */
char *http_response_serialize(http_response_t *response, size_t *out_length);

/*
 * Get status message for status code
 */
const char *status_code_to_message(int status_code);

/*
 * Free HTTP response resources
 */
void http_response_free(http_response_t *response);

#endif /* HTTP_RESPONSE_H */
