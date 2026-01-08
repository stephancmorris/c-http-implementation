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

/*
 * Parse HTTP request line
 * Example: "POST /api/payment HTTP/1.1"
 * Returns 0 on success, -1 on error
 */
int parse_request_line(http_request_t *request, const char *line) {
    if (request == NULL || line == NULL) {
        LOG_ERROR(NULL, "parse_request_line: NULL parameter");
        return -1;
    }

    /* Create a mutable copy of the line for parsing */
    char line_copy[MAX_URI_LENGTH + 256];  /* METHOD + URI + VERSION + spaces */
    size_t line_len = strlen(line);

    if (line_len >= sizeof(line_copy)) {
        LOG_ERROR(NULL, "Request line too long: %zu bytes", line_len);
        return -1;
    }

    strncpy(line_copy, line, sizeof(line_copy) - 1);
    line_copy[sizeof(line_copy) - 1] = '\0';

    /* Remove trailing \r\n if present */
    char *newline = strchr(line_copy, '\r');
    if (newline) *newline = '\0';
    newline = strchr(line_copy, '\n');
    if (newline) *newline = '\0';

    /* Parse method (first token) */
    char *saveptr = NULL;
    char *method_str = strtok_r(line_copy, " ", &saveptr);
    if (method_str == NULL) {
        LOG_ERROR(NULL, "Missing HTTP method in request line");
        return -1;
    }

    /* Convert method string to enum */
    request->method = http_string_to_method(method_str);
    if (request->method == HTTP_METHOD_UNKNOWN) {
        LOG_WARN(NULL, "Unknown HTTP method: %s", method_str);
        /* Continue parsing - we'll handle unknown methods */
    }

    /* Parse URI (second token) */
    char *uri_str = strtok_r(NULL, " ", &saveptr);
    if (uri_str == NULL) {
        LOG_ERROR(NULL, "Missing URI in request line");
        return -1;
    }

    /* Validate URI format (must start with /) */
    if (uri_str[0] != '/') {
        LOG_ERROR(NULL, "Invalid URI format (must start with /): %s", uri_str);
        return -1;
    }

    /* Copy URI to request structure */
    size_t uri_len = strlen(uri_str);
    if (uri_len >= MAX_URI_LENGTH) {
        LOG_ERROR(NULL, "URI too long: %zu bytes (max %d)", uri_len, MAX_URI_LENGTH);
        return -1;
    }
    strncpy(request->uri, uri_str, MAX_URI_LENGTH - 1);
    request->uri[MAX_URI_LENGTH - 1] = '\0';

    /* Parse HTTP version (third token) */
    char *version_str = strtok_r(NULL, " ", &saveptr);
    if (version_str == NULL) {
        LOG_ERROR(NULL, "Missing HTTP version in request line");
        return -1;
    }

    /* Parse HTTP version */
    if (strcmp(version_str, "HTTP/1.1") == 0) {
        request->version = HTTP_VERSION_1_1;
    } else if (strcmp(version_str, "HTTP/1.0") == 0) {
        request->version = HTTP_VERSION_1_0;
    } else {
        LOG_ERROR(NULL, "Unsupported HTTP version: %s", version_str);
        request->version = HTTP_VERSION_UNKNOWN;
        return -1;
    }

    /* Check for extra tokens (malformed request) */
    char *extra = strtok_r(NULL, " ", &saveptr);
    if (extra != NULL) {
        LOG_WARN(NULL, "Extra data in request line: %s", extra);
    }

    LOG_DEBUG(NULL, "Parsed request line: %s %s %s",
              http_method_to_string(request->method),
              request->uri,
              http_version_to_string(request->version));

    return 0;
}
