#include "create_buffer.h"
#include <stdlib.h>
#include <string.h>

Buffer* create_buffer(size_t initial_size) {
    Buffer* buf = malloc(sizeof(Buffer));
    if (!buf) {
        return NULL;
    }
    
    buf->data = malloc(initial_size);
    if (!buf->data) {
        free(buf);
        return NULL;
    }
    
    buf->size = initial_size;
    buf->len = 0;
    buf->pos = 0;
    memset(buf->data, 0, initial_size);
    
    return buf;
}