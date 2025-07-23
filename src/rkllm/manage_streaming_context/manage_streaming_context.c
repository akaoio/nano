#include "manage_streaming_context.h"
#include <string.h>

// Global streaming context - only ONE inference at a time
static StreamingContext global_streaming_context = {0, 0, 0};

void set_streaming_context(int client_fd, int request_id) {
    global_streaming_context.client_fd = client_fd;
    global_streaming_context.request_id = request_id;
    global_streaming_context.is_active = 1;
}

void clear_streaming_context(void) {
    memset(&global_streaming_context, 0, sizeof(StreamingContext));
}

StreamingContext* get_streaming_context(void) {
    if (global_streaming_context.is_active) {
        return &global_streaming_context;
    }
    return NULL;
}