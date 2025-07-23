#ifndef SAFE_STRING_H
#define SAFE_STRING_H

#include <stddef.h>

/**
 * Safe string formatting with overflow protection
 * @param buffer Destination buffer
 * @param buffer_size Size of destination buffer
 * @param format Printf-style format string
 * @param ... Variable arguments for format string
 * @return Number of characters written (excluding null terminator), or -1 on error
 */
int safe_snprintf(char* buffer, size_t buffer_size, const char* format, ...);

/**
 * Safe string copy with overflow protection
 * @param dest Destination buffer
 * @param src Source string
 * @param dest_size Size of destination buffer
 * @return 0 on success, -1 on error
 */
int safe_strcpy(char* dest, const char* src, size_t dest_size);

/**
 * Safe string concatenation with overflow protection
 * @param dest Destination buffer
 * @param src Source string to append
 * @param dest_size Size of destination buffer
 * @return 0 on success, -1 on error
 */
int safe_strcat(char* dest, const char* src, size_t dest_size);

#endif