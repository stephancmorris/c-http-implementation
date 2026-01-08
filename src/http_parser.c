/*
 * C-HTTP Payment Server - HTTP Parser Implementation
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

/*
 * Helper function: Trim leading and trailing whitespace from string
 * Modifies string in place
 */
static void trim_whitespace(char *str) {
    if (str == NULL || *str == '\0') {
        return;
    }

    /* Trim leading whitespace */
    char *start = str;
    while (*start == ' ' || *start == '\t' || *start == '\r' || *start == '\n') {
        start++;
    }

    /* Trim trailing whitespace */
    char *end = start + strlen(start) - 1;
    while (end > start && (*end == ' ' || *end == '\t' || *end == '\r' || *end == '\n')) {
        end--;
    }
    *(end + 1) = '\0';

    /* Move trimmed string to beginning if needed */
    if (start != str) {
        memmove(str, start, strlen(start) + 1);
    }
}

/*
 * Parse HTTP headers section
 * Example header section:
 *   "Host: example.com\r\n"
 *   "Content-Length: 123\r\n"
 *   "X-Idempotency-Key: abc123\r\n"
 *   "\r\n"
 * Returns 0 on success, -1 on error
 */
int parse_headers(http_request_t *request, const char *header_section) {
    if (request == NULL || header_section == NULL) {
        LOG_ERROR(NULL, "parse_headers: NULL parameter");
        return -1;
    }

    /* Initialize header-related fields */
    request->header_count = 0;
    request->content_length = 0;
    request->has_idempotency_key = false;
    request->idempotency_key[0] = '\0';

    /* Create a mutable copy of the header section */
    char *header_copy = strdup(header_section);
    if (header_copy == NULL) {
        LOG_ERROR(NULL, "Failed to allocate memory for header copy");
        return -1;
    }

    /* Parse headers line by line */
    char *saveptr = NULL;
    char *line = strtok_r(header_copy, "\r\n", &saveptr);

    while (line != NULL) {
        /* Skip empty lines (signals end of headers) */
        if (strlen(line) == 0) {
            break;
        }

        /* Find the colon separator */
        char *colon = strchr(line, ':');
        if (colon == NULL) {
            LOG_WARN(NULL, "Malformed header line (missing colon): %s", line);
            line = strtok_r(NULL, "\r\n", &saveptr);
            continue;
        }

        /* Split into name and value */
        *colon = '\0';  /* Terminate name string */
        char *name = line;
        char *value = colon + 1;

        /* Trim whitespace from both parts */
        trim_whitespace(name);
        trim_whitespace(value);

        /* Validate header name length */
        size_t name_len = strlen(name);
        if (name_len == 0) {
            LOG_WARN(NULL, "Empty header name");
            line = strtok_r(NULL, "\r\n", &saveptr);
            continue;
        }
        if (name_len >= MAX_HEADER_NAME_LENGTH) {
            LOG_ERROR(NULL, "Header name too long: %zu bytes (max %d)",
                      name_len, MAX_HEADER_NAME_LENGTH);
            free(header_copy);
            return -1;
        }

        /* Validate header value length */
        size_t value_len = strlen(value);
        if (value_len >= MAX_HEADER_VALUE_LENGTH) {
            LOG_WARN(NULL, "Header value too long: %zu bytes (max %d), truncating",
                     value_len, MAX_HEADER_VALUE_LENGTH);
            value[MAX_HEADER_VALUE_LENGTH - 1] = '\0';
            value_len = MAX_HEADER_VALUE_LENGTH - 1;
        }

        /* Check if we've exceeded maximum header count */
        if (request->header_count >= MAX_HEADERS) {
            LOG_ERROR(NULL, "Too many headers: maximum %d headers allowed", MAX_HEADERS);
            free(header_copy);
            return -1;
        }

        /* Store header in array */
        strncpy(request->headers[request->header_count].name, name,
                MAX_HEADER_NAME_LENGTH - 1);
        request->headers[request->header_count].name[MAX_HEADER_NAME_LENGTH - 1] = '\0';

        strncpy(request->headers[request->header_count].value, value,
                MAX_HEADER_VALUE_LENGTH - 1);
        request->headers[request->header_count].value[MAX_HEADER_VALUE_LENGTH - 1] = '\0';

        LOG_DEBUG(NULL, "Parsed header: %s: %s", name, value);

        /* Check for special headers (case-insensitive) */

        /* X-Idempotency-Key header */
        if (strcasecmp(name, "X-Idempotency-Key") == 0) {
            size_t key_len = strlen(value);
            if (key_len >= MAX_IDEMPOTENCY_KEY_LENGTH) {
                LOG_WARN(NULL, "Idempotency key too long: %zu bytes (max %d), truncating",
                         key_len, MAX_IDEMPOTENCY_KEY_LENGTH);
                value[MAX_IDEMPOTENCY_KEY_LENGTH - 1] = '\0';
            }
            strncpy(request->idempotency_key, value, MAX_IDEMPOTENCY_KEY_LENGTH - 1);
            request->idempotency_key[MAX_IDEMPOTENCY_KEY_LENGTH - 1] = '\0';
            request->has_idempotency_key = true;
            LOG_DEBUG(NULL, "Extracted Idempotency-Key: %s", request->idempotency_key);
        }

        /* Content-Length header */
        if (strcasecmp(name, "Content-Length") == 0) {
            char *endptr;
            unsigned long content_len = strtoul(value, &endptr, 10);

            /* Validate conversion */
            if (*endptr != '\0') {
                LOG_WARN(NULL, "Invalid Content-Length value: %s", value);
                request->content_length = 0;
            } else {
                request->content_length = (size_t)content_len;
                LOG_DEBUG(NULL, "Extracted Content-Length: %zu bytes", request->content_length);
            }
        }

        request->header_count++;

        /* Move to next line */
        line = strtok_r(NULL, "\r\n", &saveptr);
    }

    free(header_copy);

    LOG_INFO(NULL, "Parsed %d headers successfully", request->header_count);
    return 0;
}
