#ifndef CREATE_BUFFER_H
#define CREATE_BUFFER_H

#include <stddef.h>

/**
 * Buffer structure for I/O operations
 */
typedef struct {
    char* data;        // Buffer data
    size_t size;       // Buffer size
    size_t len;        // Current data length
    size_t pos;        // Current position
} Buffer;

/**
 * Creates I/O buffer
 * @param initial_size Initial buffer size
 * @return Buffer pointer or NULL on error
 */
Buffer* create_buffer(size_t initial_size);

#endif