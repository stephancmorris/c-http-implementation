/*
 * C-HTTP Payment Server - HTTP Response Implementation
 * Builds and serializes HTTP/1.1 responses
 */

#include "http_response.h"
#include "logger.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

/*
 * Get status message for HTTP status code
 * Returns string representation of status code
 */
const char *status_code_to_message(int status_code) {
    switch (status_code) {
        case HTTP_OK:
            return "OK";
        case HTTP_BAD_REQUEST:
            return "Bad Request";
        case HTTP_NOT_FOUND:
            return "Not Found";
        case HTTP_CONFLICT:
            return "Conflict";
        case HTTP_PAYLOAD_TOO_LARGE:
            return "Payload Too Large";
        case HTTP_UNPROCESSABLE:
            return "Unprocessable Entity";
        case HTTP_INTERNAL_ERROR:
            return "Internal Server Error";
        case HTTP_NOT_IMPLEMENTED:
            return "Not Implemented";
        default:
            return "Unknown";
    }
}

/*
 * Initialize HTTP response with status code
 * Returns 0 on success, -1 on error
 */
int http_response_init(http_response_t *response, int status_code) {
    if (response == NULL) {
        LOG_ERROR(NULL, "http_response_init: NULL response pointer");
        return -1;
    }

    /* Initialize status line */
    response->status_code = status_code;
    response->status_message = strdup(status_code_to_message(status_code));
    if (response->status_message == NULL) {
        LOG_ERROR(NULL, "Failed to allocate status message");
        return -1;
    }

    /* Initialize headers array */
    response->header_names = NULL;
    response->header_values = NULL;
    response->header_count = 0;
    response->header_capacity = 0;

    /* Initialize body */
    response->body = NULL;
    response->body_length = 0;

    LOG_DEBUG(NULL, "Initialized response with status %d %s",
              status_code, response->status_message);

    return 0;
}

/*
 * Add header to response
 * Returns 0 on success, -1 on error
 */
int http_response_add_header(http_response_t *response, const char *name, const char *value) {
    if (response == NULL || name == NULL || value == NULL) {
        LOG_ERROR(NULL, "http_response_add_header: NULL parameter");
        return -1;
    }

    /* Expand header arrays if needed */
    if (response->header_count >= response->header_capacity) {
        size_t new_capacity = response->header_capacity == 0 ? 8 : response->header_capacity * 2;

        char **new_names = (char **)realloc(response->header_names, new_capacity * sizeof(char *));
        if (new_names == NULL) {
            LOG_ERROR(NULL, "Failed to allocate memory for header names");
            return -1;
        }
        response->header_names = new_names;

        char **new_values = (char **)realloc(response->header_values, new_capacity * sizeof(char *));
        if (new_values == NULL) {
            LOG_ERROR(NULL, "Failed to allocate memory for header values");
            return -1;
        }
        response->header_values = new_values;

        response->header_capacity = new_capacity;
    }

    /* Add header */
    response->header_names[response->header_count] = strdup(name);
    if (response->header_names[response->header_count] == NULL) {
        LOG_ERROR(NULL, "Failed to allocate memory for header name");
        return -1;
    }

    response->header_values[response->header_count] = strdup(value);
    if (response->header_values[response->header_count] == NULL) {
        LOG_ERROR(NULL, "Failed to allocate memory for header value");
        free(response->header_names[response->header_count]);
        return -1;
    }

    response->header_count++;

    LOG_DEBUG(NULL, "Added header: %s: %s", name, value);

    return 0;
}

/*
 * Set response body
 * Returns 0 on success, -1 on error
 */
int http_response_set_body(http_response_t *response, const char *body, size_t length) {
    if (response == NULL) {
        LOG_ERROR(NULL, "http_response_set_body: NULL response pointer");
        return -1;
    }

    /* Free existing body if present */
    if (response->body != NULL) {
        free(response->body);
        response->body = NULL;
        response->body_length = 0;
    }

    /* Handle NULL or empty body */
    if (body == NULL || length == 0) {
        LOG_DEBUG(NULL, "Set empty response body");
        return 0;
    }

    /* Allocate and copy body */
    response->body = (char *)malloc(length);
    if (response->body == NULL) {
        LOG_ERROR(NULL, "Failed to allocate %zu bytes for response body", length);
        return -1;
    }

    memcpy(response->body, body, length);
    response->body_length = length;

    LOG_DEBUG(NULL, "Set response body: %zu bytes", length);

    return 0;
}

/*
 * Serialize response to string (ready to send)
 * Automatically adds Server, Date, and Content-Length headers
 * Returns serialized response string (caller must free), NULL on error
 */
char *http_response_serialize(http_response_t *response, size_t *out_length) {
    if (response == NULL || out_length == NULL) {
        LOG_ERROR(NULL, "http_response_serialize: NULL parameter");
        return NULL;
    }

    /* Calculate response size estimate */
    size_t estimated_size = 1024;  /* Status line + headers estimate */
    estimated_size += response->body_length;

    /* Allocate buffer for serialized response */
    char *buffer = (char *)malloc(estimated_size);
    if (buffer == NULL) {
        LOG_ERROR(NULL, "Failed to allocate serialization buffer");
        return NULL;
    }

    size_t offset = 0;

    /* Write status line: HTTP/1.1 200 OK\r\n */
    int written = snprintf(buffer + offset, estimated_size - offset,
                          "HTTP/1.1 %d %s\r\n",
                          response->status_code,
                          response->status_message);
    if (written < 0 || (size_t)written >= estimated_size - offset) {
        LOG_ERROR(NULL, "Buffer overflow writing status line");
        free(buffer);
        return NULL;
    }
    offset += written;

    /* Add Server header */
    written = snprintf(buffer + offset, estimated_size - offset,
                      "Server: C-HTTP-Payment-Server/1.0\r\n");
    if (written < 0 || (size_t)written >= estimated_size - offset) {
        LOG_ERROR(NULL, "Buffer overflow writing Server header");
        free(buffer);
        return NULL;
    }
    offset += written;

    /* Add Date header */
    time_t now = time(NULL);
    struct tm *gmt = gmtime(&now);
    char date_buf[128];
    strftime(date_buf, sizeof(date_buf), "%a, %d %b %Y %H:%M:%S GMT", gmt);
    written = snprintf(buffer + offset, estimated_size - offset,
                      "Date: %s\r\n", date_buf);
    if (written < 0 || (size_t)written >= estimated_size - offset) {
        LOG_ERROR(NULL, "Buffer overflow writing Date header");
        free(buffer);
        return NULL;
    }
    offset += written;

    /* Add Content-Length header */
    written = snprintf(buffer + offset, estimated_size - offset,
                      "Content-Length: %zu\r\n", response->body_length);
    if (written < 0 || (size_t)written >= estimated_size - offset) {
        LOG_ERROR(NULL, "Buffer overflow writing Content-Length header");
        free(buffer);
        return NULL;
    }
    offset += written;

    /* Write custom headers */
    for (size_t i = 0; i < response->header_count; i++) {
        written = snprintf(buffer + offset, estimated_size - offset,
                          "%s: %s\r\n",
                          response->header_names[i],
                          response->header_values[i]);
        if (written < 0 || (size_t)written >= estimated_size - offset) {
            LOG_ERROR(NULL, "Buffer overflow writing header %zu", i);
            free(buffer);
            return NULL;
        }
        offset += written;
    }

    /* Write blank line after headers */
    written = snprintf(buffer + offset, estimated_size - offset, "\r\n");
    if (written < 0 || (size_t)written >= estimated_size - offset) {
        LOG_ERROR(NULL, "Buffer overflow writing header separator");
        free(buffer);
        return NULL;
    }
    offset += written;

    /* Write body if present */
    if (response->body != NULL && response->body_length > 0) {
        if (offset + response->body_length > estimated_size) {
            LOG_ERROR(NULL, "Buffer overflow writing body");
            free(buffer);
            return NULL;
        }
        memcpy(buffer + offset, response->body, response->body_length);
        offset += response->body_length;
    }

    *out_length = offset;

    LOG_INFO(NULL, "Serialized response: %zu bytes (status %d)",
             offset, response->status_code);

    return buffer;
}

/*
 * Create error response with JSON error body
 * Returns 0 on success, -1 on error
 */
int http_response_create_error(http_response_t *response, int status_code, const char *error_message) {
    if (response == NULL || error_message == NULL) {
        LOG_ERROR(NULL, "http_response_create_error: NULL parameter");
        return -1;
    }

    /* Initialize response with error status code */
    int result = http_response_init(response, status_code);
    if (result != 0) {
        return -1;
    }

    /* Add Content-Type: application/json header */
    result = http_response_add_header(response, "Content-Type", "application/json");
    if (result != 0) {
        http_response_free(response);
        return -1;
    }

    /* Create JSON error body */
    char error_body[1024];
    int written = snprintf(error_body, sizeof(error_body),
                          "{\"error\":\"%s\",\"status\":%d,\"message\":\"%s\"}",
                          error_message,
                          status_code,
                          status_code_to_message(status_code));

    if (written < 0 || (size_t)written >= sizeof(error_body)) {
        LOG_ERROR(NULL, "Failed to format error response body");
        http_response_free(response);
        return -1;
    }

    /* Set body */
    result = http_response_set_body(response, error_body, strlen(error_body));
    if (result != 0) {
        http_response_free(response);
        return -1;
    }

    LOG_INFO(NULL, "Created error response: %d %s - %s",
             status_code, status_code_to_message(status_code), error_message);

    return 0;
}

/*
 * Free HTTP response resources
 */
void http_response_free(http_response_t *response) {
    if (response == NULL) {
        return;
    }

    /* Free status message */
    if (response->status_message != NULL) {
        free(response->status_message);
        response->status_message = NULL;
    }

    /* Free headers */
    if (response->header_names != NULL) {
        for (size_t i = 0; i < response->header_count; i++) {
            free(response->header_names[i]);
        }
        free(response->header_names);
        response->header_names = NULL;
    }

    if (response->header_values != NULL) {
        for (size_t i = 0; i < response->header_count; i++) {
            free(response->header_values[i]);
        }
        free(response->header_values);
        response->header_values = NULL;
    }

    /* Free body */
    if (response->body != NULL) {
        free(response->body);
        response->body = NULL;
    }

    response->header_count = 0;
    response->header_capacity = 0;
    response->body_length = 0;

    LOG_DEBUG(NULL, "Freed response resources");
}
