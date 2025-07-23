#include "destroy_buffer.h"
#include <stdlib.h>

void destroy_buffer(Buffer* buf) {
    if (!buf) {
        return;
    }
    
    if (buf->data) {
        free(buf->data);
    }
    
    free(buf);
}