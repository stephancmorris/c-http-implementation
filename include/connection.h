/*
 * NanoServe - Connection Handler
 * Handles reading from and writing to client connections
 */

#ifndef CONNECTION_H
#define CONNECTION_H

#include <stddef.h>
#include <sys/types.h>

/* Maximum buffer size for reading requests */
#define CONN_BUFFER_SIZE 8192

/*
 * Read data from a client socket
 * Returns number of bytes read, 0 on connection close, -1 on error
 */
ssize_t connection_read(int client_fd, char *buffer, size_t buffer_size);

/*
 * Write data to a client socket
 * Returns number of bytes written, -1 on error
 */
ssize_t connection_write(int client_fd, const char *data, size_t data_len);

/*
 * Handle a complete client connection
 * Reads HTTP request and sends back a simple HTTP response
 * Returns 0 on success, -1 on error
 */
int connection_handle(int client_fd);

#endif /* CONNECTION_H */
