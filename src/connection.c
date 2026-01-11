/*
 * C-HTTP Payment Server - Connection Handler
 * Handles reading from and writing to client connections
 */

#include "connection.h"
#include "http_parser.h"
#include "http_response.h"
#include "logger.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>

/*
 * Read data from a client socket
 * Returns number of bytes read, 0 on connection close, -1 on error
 */
ssize_t connection_read(int client_fd, char *buffer, size_t buffer_size) {
    if (buffer == NULL || buffer_size == 0) {
        LOG_ERROR(NULL, "connection_read: invalid buffer");
        return -1;
    }

    ssize_t bytes_read = recv(client_fd, buffer, buffer_size - 1, 0);

    if (bytes_read < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            /* No data available (for non-blocking sockets) */
            LOG_DEBUG(NULL, "recv() would block (fd=%d)", client_fd);
            return -1;
        } else if (errno == EINTR) {
            /* Interrupted by signal */
            LOG_DEBUG(NULL, "recv() interrupted by signal (fd=%d)", client_fd);
            return -1;
        } else {
            /* Actual error */
            LOG_ERROR(NULL, "recv() failed (fd=%d): %s", client_fd, strerror(errno));
            return -1;
        }
    } else if (bytes_read == 0) {
        /* Connection closed by client */
        LOG_DEBUG(NULL, "Connection closed by client (fd=%d)", client_fd);
        return 0;
    }

    /* Null-terminate the buffer for string operations */
    buffer[bytes_read] = '\0';

    LOG_DEBUG(NULL, "Read %zd bytes from client (fd=%d)", bytes_read, client_fd);
    return bytes_read;
}

/*
 * Write data to a client socket
 * Returns number of bytes written, -1 on error
 */
ssize_t connection_write(int client_fd, const char *data, size_t data_len) {
    if (data == NULL || data_len == 0) {
        LOG_ERROR(NULL, "connection_write: invalid data");
        return -1;
    }

    ssize_t bytes_written = send(client_fd, data, data_len, 0);

    if (bytes_written < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            /* Would block (for non-blocking sockets) */
            LOG_DEBUG(NULL, "send() would block (fd=%d)", client_fd);
            return -1;
        } else if (errno == EINTR) {
            /* Interrupted by signal */
            LOG_DEBUG(NULL, "send() interrupted by signal (fd=%d)", client_fd);
            return -1;
        } else if (errno == EPIPE) {
            /* Broken pipe - client closed connection */
            LOG_WARN(NULL, "Broken pipe - client closed connection (fd=%d)", client_fd);
            return -1;
        } else {
            /* Actual error */
            LOG_ERROR(NULL, "send() failed (fd=%d): %s", client_fd, strerror(errno));
            return -1;
        }
    }

    LOG_DEBUG(NULL, "Wrote %zd bytes to client (fd=%d)", bytes_written, client_fd);
    return bytes_written;
}

/*
 * Handle a complete client connection
 * Reads HTTP request, parses it, and sends back appropriate HTTP response
 * Returns 0 on success, -1 on error
 */
int connection_handle(int client_fd) {
    char buffer[CONN_BUFFER_SIZE];
    http_request_t request;
    http_response_t response;
    int result = 0;
    size_t serialized_length = 0;
    char *serialized = NULL;
    size_t total_sent = 0;
    ssize_t bytes_sent;
    int response_initialized = 0;

    LOG_INFO(NULL, "Starting connection_handle (fd=%d)", client_fd);
    LOG_DEBUG(NULL, "Handling connection (fd=%d)", client_fd);

    /* Initialize structures to safe state */
    memset(&response, 0, sizeof(response));

    /* Initialize request structure */
    if (http_request_init(&request) != 0) {
        LOG_ERROR(NULL, "Failed to initialize HTTP request (fd=%d)", client_fd);
        return -1;
    }

    /* Step 1: Read the HTTP request headers */
    LOG_INFO(NULL, "About to read from socket (fd=%d)", client_fd);
    ssize_t bytes_read = connection_read(client_fd, buffer, CONN_BUFFER_SIZE);
    LOG_INFO(NULL, "Read %zd bytes from socket (fd=%d)", bytes_read, client_fd);

    if (bytes_read <= 0) {
        if (bytes_read == 0) {
            LOG_DEBUG(NULL, "Client closed connection before sending data (fd=%d)", client_fd);
        } else {
            LOG_ERROR(NULL, "Failed to read from client (fd=%d)", client_fd);
        }
        http_request_free(&request);
        return -1;
    }

    LOG_DEBUG(NULL, "Read %zd bytes from client (fd=%d)", bytes_read, client_fd);

    /* Step 2: Find the end of headers (blank line: \r\n\r\n) */
    char *headers_end = strstr(buffer, "\r\n\r\n");
    if (headers_end == NULL) {
        LOG_WARN(NULL, "Malformed HTTP request - no blank line after headers (fd=%d)", client_fd);
        http_response_create_error(&response, HTTP_BAD_REQUEST, "Malformed HTTP request");
        response_initialized = 1;
        goto send_response;
    }

    /* Null-terminate the headers section */
    *headers_end = '\0';

    /* Step 3: Parse request line */
    char *request_line = buffer;
    char *line_end = strstr(buffer, "\r\n");
    if (line_end == NULL) {
        LOG_WARN(NULL, "Malformed request line (fd=%d)", client_fd);
        http_response_create_error(&response, HTTP_BAD_REQUEST, "Malformed request line");
        response_initialized = 1;
        goto send_response;
    }

    /* Temporarily null-terminate request line */
    *line_end = '\0';

    if (parse_request_line(&request, request_line) != 0) {
        LOG_WARN(NULL, "Failed to parse request line (fd=%d)", client_fd);
        http_response_create_error(&response, HTTP_BAD_REQUEST, "Invalid request line");
        response_initialized = 1;
        goto send_response;
    }

    /* Move to headers (skip \r\n) */
    char *headers_start = line_end + 2;

    LOG_INFO(NULL, "Request: %s %s HTTP/%s (fd=%d)",
             http_method_to_string(request.method),
             request.uri,
             http_version_to_string(request.version),
             client_fd);

    /* Step 4: Parse headers */
    if (parse_headers(&request, headers_start) != 0) {
        LOG_WARN(NULL, "Failed to parse headers (fd=%d)", client_fd);
        http_response_create_error(&response, HTTP_BAD_REQUEST, "Invalid headers");
        response_initialized = 1;
        goto send_response;
    }

    /* Step 5: Parse request body if Content-Length is present */
    if (request.content_length > 0) {
        if (request.method != HTTP_METHOD_POST && request.method != HTTP_METHOD_PUT) {
            LOG_WARN(NULL, "Content-Length on non-POST/PUT request (fd=%d)", client_fd);
        }

        /* Check if body exceeds size limit */
        if (request.content_length > MAX_REQUEST_BODY_SIZE) {
            LOG_WARN(NULL, "Request body too large: %zu bytes (fd=%d)",
                     request.content_length, client_fd);
            http_response_create_error(&response, HTTP_PAYLOAD_TOO_LARGE,
                                     "Request body exceeds 1MB limit");
            response_initialized = 1;
            goto send_response;
        }

        if (parse_request_body(&request, client_fd) != 0) {
            LOG_ERROR(NULL, "Failed to read request body (fd=%d)", client_fd);
            http_response_create_error(&response, HTTP_BAD_REQUEST, "Failed to read request body");
            response_initialized = 1;
            goto send_response;
        }

        LOG_INFO(NULL, "Read request body: %zu bytes (fd=%d)", request.body_length, client_fd);
    }

    /* Step 6: Validate POST requests have idempotency key */
    if (request.method == HTTP_METHOD_POST && !request.has_idempotency_key) {
        LOG_WARN(NULL, "POST request missing X-Idempotency-Key header (fd=%d)", client_fd);
        http_response_create_error(&response, HTTP_UNPROCESSABLE,
                                 "POST requests require X-Idempotency-Key header");
        response_initialized = 1;
        goto send_response;
    }

    /* Step 7: Generate success response (hardcoded test response for now) */
    if (http_response_init(&response, HTTP_OK) != 0) {
        LOG_ERROR(NULL, "Failed to initialize response (fd=%d)", client_fd);
        result = -1;
        goto cleanup;
    }
    response_initialized = 1;

    http_response_add_header(&response, "Content-Type", "application/json");

    /* Build response body with request info */
    char response_body[1024];
    int body_len;

    if (request.method == HTTP_METHOD_POST && request.has_idempotency_key) {
        body_len = snprintf(response_body, sizeof(response_body),
                           "{\"status\":\"success\",\"message\":\"Payment processed\","
                           "\"idempotency_key\":\"%s\",\"body_size\":%zu}",
                           request.idempotency_key,
                           request.body_length);
    } else {
        body_len = snprintf(response_body, sizeof(response_body),
                           "{\"status\":\"success\",\"message\":\"Request received\","
                           "\"method\":\"%s\",\"uri\":\"%s\"}",
                           http_method_to_string(request.method),
                           request.uri);
    }

    if (body_len < 0 || (size_t)body_len >= sizeof(response_body)) {
        LOG_ERROR(NULL, "Failed to format response body (fd=%d)", client_fd);
        http_response_create_error(&response, HTTP_INTERNAL_ERROR, "Failed to format response");
        response_initialized = 1;
        goto send_response;
    }

    http_response_set_body(&response, response_body, body_len);

send_response:
    /* Step 8: Serialize and send response */
    serialized_length = 0;
    serialized = http_response_serialize(&response, &serialized_length);

    if (serialized == NULL) {
        LOG_ERROR(NULL, "Failed to serialize response (fd=%d)", client_fd);
        result = -1;
        goto cleanup;
    }

    /* Send response in loop to handle partial writes */
    total_sent = 0;
    while (total_sent < serialized_length) {
        bytes_sent = connection_write(client_fd,
                                      serialized + total_sent,
                                      serialized_length - total_sent);
        if (bytes_sent < 0) {
            LOG_ERROR(NULL, "Failed to send response (fd=%d)", client_fd);
            result = -1;
            break;
        }
        total_sent += bytes_sent;
    }

    if (result == 0) {
        LOG_INFO(NULL, "Sent HTTP %d response (%zu bytes) to client (fd=%d)",
                 response.status_code, total_sent, client_fd);
    }

    free(serialized);

cleanup:
    /* Step 9: Clean up resources */
    http_request_free(&request);
    if (response_initialized) {
        http_response_free(&response);
    }

    return result;
}
