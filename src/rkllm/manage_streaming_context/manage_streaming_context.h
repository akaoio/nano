#ifndef MANAGE_STREAMING_CONTEXT_H
#define MANAGE_STREAMING_CONTEXT_H

#include <json-c/json.h>

// Streaming context for active inference
typedef struct {
    int client_fd;
    int request_id;
    int is_active;
} StreamingContext;

/**
 * Sets the global streaming context for the current inference
 * @param client_fd File descriptor of the client
 * @param request_id JSON-RPC request ID
 */
void set_streaming_context(int client_fd, int request_id);

/**
 * Clears the global streaming context
 */
void clear_streaming_context(void);

/**
 * Gets the current streaming context
 * @return Pointer to current context or NULL if none active
 */
StreamingContext* get_streaming_context(void);

#endif