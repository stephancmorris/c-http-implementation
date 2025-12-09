/*
 * NanoServe - Static File Server
 * Handles file serving and MIME type detection
 */

#ifndef FILESERVER_H
#define FILESERVER_H

#include "http_parser.h"
#include "http_response.h"

/* File server configuration */
typedef struct {
    char *document_root;        /* Document root directory */
} fileserver_config_t;

/*
 * Initialize file server with document root
 * Returns 0 on success, -1 on error
 */
int fileserver_init(fileserver_config_t *config, const char *document_root);

/*
 * Resolve URI to file path
 * Returns resolved path (caller must free), NULL on error or security violation
 */
char *resolve_file_path(const fileserver_config_t *config, const char *uri);

/*
 * Serve file for given request
 * Populates response with file content or error
 * Returns 0 on success, -1 on error
 */
int serve_file(const fileserver_config_t *config,
               const http_request_t *request,
               http_response_t *response);

/*
 * Get MIME type for file extension
 * Returns MIME type string
 */
const char *get_mime_type(const char *filename);

/*
 * Cleanup file server resources
 */
void fileserver_destroy(fileserver_config_t *config);

#endif /* FILESERVER_H */
