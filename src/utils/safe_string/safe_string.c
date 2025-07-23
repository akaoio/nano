#include "safe_string.h"
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>

int safe_snprintf(char* buffer, size_t buffer_size, const char* format, ...) {
    if (!buffer || buffer_size == 0 || !format) {
        errno = EINVAL;
        return -1;
    }
    
    va_list args;
    va_start(args, format);
    
    int result = vsnprintf(buffer, buffer_size, format, args);
    
    va_end(args);
    
    // Check for truncation or error
    if (result < 0) {
        // Error occurred
        buffer[0] = '\0';  // Ensure null termination
        return -1;
    }
    
    if ((size_t)result >= buffer_size) {
        // Output was truncated
        buffer[buffer_size - 1] = '\0';  // Ensure null termination
        errno = ENOBUFS;
        return -1;
    }
    
    return result;
}

int safe_strcpy(char* dest, const char* src, size_t dest_size) {
    if (!dest || !src || dest_size == 0) {
        errno = EINVAL;
        return -1;
    }
    
    size_t src_len = strlen(src);
    
    if (src_len >= dest_size) {
        // Source string too long for destination
        dest[0] = '\0';
        errno = ENOBUFS;
        return -1;
    }
    
    strcpy(dest, src);
    return 0;
}

int safe_strcat(char* dest, const char* src, size_t dest_size) {
    if (!dest || !src || dest_size == 0) {
        errno = EINVAL;
        return -1;
    }
    
    size_t dest_len = strlen(dest);
    size_t src_len = strlen(src);
    
    if (dest_len + src_len >= dest_size) {
        // Combined string too long for destination
        errno = ENOBUFS;
        return -1;
    }
    
    strcat(dest, src);
    return 0;
}