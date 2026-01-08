/*
 * C-HTTP Payment Server - Connection Handler
 * Handles reading from and writing to client connections
 */

#include "connection.h"
#include "logger.h"
#include <stdio.h>
#include <string.h>
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
 * Reads HTTP request and sends back a simple HTTP response
 * Returns 0 on success, -1 on error
 */
int connection_handle(int client_fd) {
    char buffer[CONN_BUFFER_SIZE];

    LOG_DEBUG(NULL, "Handling connection (fd=%d)", client_fd);

    /* Step 1: Read the HTTP request */
    ssize_t bytes_read = connection_read(client_fd, buffer, CONN_BUFFER_SIZE);

    if (bytes_read <= 0) {
        if (bytes_read == 0) {
            LOG_DEBUG(NULL, "Client closed connection before sending data (fd=%d)", client_fd);
        } else {
            LOG_ERROR(NULL, "Failed to read from client (fd=%d)", client_fd);
        }
        return -1;
    }

    /* Step 2: Log the request (first line only for now) */
    char *newline = strchr(buffer, '\n');
    if (newline != NULL) {
        *newline = '\0';  /* Temporarily truncate at first newline */
        LOG_INFO(NULL, "Request: %s (fd=%d)", buffer, client_fd);
        *newline = '\n';  /* Restore newline */
    } else {
        LOG_INFO(NULL, "Request: %s (fd=%d)", buffer, client_fd);
    }

    /* Step 3: Build a simple HTTP/1.1 200 OK response */
    const char *response_body =
        "<!DOCTYPE html>\n"
        "<html>\n"
        "<head><title>NanoServe v2.0</title></head>\n"
        "<body>\n"
        "<h1>NanoServe v2.0</h1>\n"
        "<p>High-Reliability Idempotent HTTP Server</p>\n"
        "<p>Connection handling is working!</p>\n"
        "</body>\n"
        "</html>\n";

    size_t body_len = strlen(response_body);

    /* Build HTTP response headers */
    char response[CONN_BUFFER_SIZE];
    int header_len = snprintf(response, CONN_BUFFER_SIZE,
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/html\r\n"
        "Content-Length: %zu\r\n"
        "Connection: close\r\n"
        "\r\n"
        "%s",
        body_len,
        response_body
    );

    if (header_len < 0 || (size_t)header_len >= CONN_BUFFER_SIZE) {
        LOG_ERROR(NULL, "Failed to build HTTP response (fd=%d)", client_fd);
        return -1;
    }

    /* Step 4: Send the response */
    ssize_t bytes_written = connection_write(client_fd, response, header_len);

    if (bytes_written < 0) {
        LOG_ERROR(NULL, "Failed to write response to client (fd=%d)", client_fd);
        return -1;
    }

    LOG_INFO(NULL, "Sent HTTP 200 OK response (%zd bytes) to client (fd=%d)",
             bytes_written, client_fd);

    return 0;
}
