#ifndef CREATE_CONNECTION_H
#define CREATE_CONNECTION_H

#include <stddef.h>

/**
 * Connection structure for managing client connections
 */
typedef struct {
    int fd;                    // File descriptor
    char buffer[8192];         // Read buffer
    size_t buffer_len;         // Current buffer length
    int is_active;             // Connection active flag
} Connection;

/**
 * Creates new connection structure
 * @param fd Client file descriptor
 * @return Connection pointer or NULL on error
 */
Connection* create_connection(int fd);

#endif