#include "transport_utils.h"
#include <string.h>
#include <stdbool.h>
#include <sys/types.h>

size_t process_received_buffer(char* buffer, size_t received) {
    if (!buffer || received == 0) return 0;
    
    buffer[received] = '\0';
    
    // Remove trailing newline
    size_t len = strlen(buffer);
    if (len > 0 && buffer[len-1] == '\n') {
        buffer[len-1] = '\0';
        len--;
    }
    
    return len;
}

bool is_receive_timeout_or_error(ssize_t result) {
    return result <= 0;
}

int standard_buffer_processing(char* buffer, size_t buffer_size, ssize_t received) {
    if (!buffer || buffer_size == 0) return -1;
    
    if (is_receive_timeout_or_error(received)) {
        return -1;
    }
    
    if ((size_t)received >= buffer_size) {
        return -1; // Buffer overflow
    }
    
    process_received_buffer(buffer, (size_t)received);
    return 0;
}