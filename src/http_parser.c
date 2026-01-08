/*
 * NanoServe - HTTP Parser Implementation
 * Parses HTTP/1.1 requests with POST body and idempotency key support
 */

#include "http_parser.h"
#include "logger.h"
#include <string.h>
#include <stdlib.h>
#include <strings.h>  /* For strcasecmp */

/*
 * Initialize HTTP request structure
 * Zeros out all fields and sets default values
 */
int http_request_init(http_request_t *request) {
    if (request == NULL) {
        LOG_ERROR(NULL, "http_request_init: NULL request pointer");
        return -1;
    }

    /* Zero out the entire structure */
    memset(request, 0, sizeof(http_request_t));

    /* Set default values */
    request->method = HTTP_METHOD_UNKNOWN;
    request->version = HTTP_VERSION_UNKNOWN;
    request->header_count = 0;
    request->body = NULL;
    request->body_length = 0;
    request->content_length = 0;
    request->has_idempotency_key = false;

    LOG_DEBUG(NULL, "HTTP request structure initialized");
    return 0;
}

/*
 * Free HTTP request resources
 * Deallocates dynamically allocated memory
 */
void http_request_free(http_request_t *request) {
    if (request == NULL) {
        return;
    }

    /* Free request body if allocated */
    if (request->body != NULL) {
        free(request->body);
        request->body = NULL;
    }

    /* Zero out the structure */
    request->body_length = 0;
    request->content_length = 0;
    request->header_count = 0;
    request->has_idempotency_key = false;

    LOG_DEBUG(NULL, "HTTP request resources freed");
}

/*
 * Convert HTTP method enum to string
 */
const char *http_method_to_string(http_method_t method) {
    switch (method) {
        case HTTP_METHOD_GET:     return "GET";
        case HTTP_METHOD_POST:    return "POST";
        case HTTP_METHOD_PUT:     return "PUT";
        case HTTP_METHOD_DELETE:  return "DELETE";
        case HTTP_METHOD_HEAD:    return "HEAD";
        case HTTP_METHOD_OPTIONS: return "OPTIONS";
        case HTTP_METHOD_PATCH:   return "PATCH";
        case HTTP_METHOD_UNKNOWN:
        default:                  return "UNKNOWN";
    }
}

/*
 * Convert string to HTTP method enum
 */
http_method_t http_string_to_method(const char *method_str) {
    if (method_str == NULL) {
        return HTTP_METHOD_UNKNOWN;
    }

    if (strcmp(method_str, "GET") == 0) {
        return HTTP_METHOD_GET;
    } else if (strcmp(method_str, "POST") == 0) {
        return HTTP_METHOD_POST;
    } else if (strcmp(method_str, "PUT") == 0) {
        return HTTP_METHOD_PUT;
    } else if (strcmp(method_str, "DELETE") == 0) {
        return HTTP_METHOD_DELETE;
    } else if (strcmp(method_str, "HEAD") == 0) {
        return HTTP_METHOD_HEAD;
    } else if (strcmp(method_str, "OPTIONS") == 0) {
        return HTTP_METHOD_OPTIONS;
    } else if (strcmp(method_str, "PATCH") == 0) {
        return HTTP_METHOD_PATCH;
    }

    return HTTP_METHOD_UNKNOWN;
}

/*
 * Convert HTTP version enum to string
 */
const char *http_version_to_string(http_version_t version) {
    switch (version) {
        case HTTP_VERSION_1_0:    return "HTTP/1.0";
        case HTTP_VERSION_1_1:    return "HTTP/1.1";
        case HTTP_VERSION_UNKNOWN:
        default:                  return "UNKNOWN";
    }
}

/*
 * Get header value by name (case-insensitive)
 * Returns pointer to value string, or NULL if not found
 */
const char *http_get_header(const http_request_t *request, const char *name) {
    if (request == NULL || name == NULL) {
        return NULL;
    }

    /* Search through headers for matching name (case-insensitive) */
    for (int i = 0; i < request->header_count; i++) {
        if (strcasecmp(request->headers[i].name, name) == 0) {
            return request->headers[i].value;
        }
    }

    return NULL;  /* Header not found */
}
