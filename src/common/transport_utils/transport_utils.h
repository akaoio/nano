#pragma once

#include <stddef.h>
#include <sys/types.h>

/**
 * @brief Process received buffer by removing trailing newline and null-terminating
 * @param buffer Buffer to process
 * @param received Number of bytes received
 * @return Processed buffer length
 */
size_t process_received_buffer(char* buffer, size_t received);

/**
 * @brief Check if receive operation resulted in timeout or error
 * @param result Result from recv/read operation
 * @return true if timeout or error, false otherwise
 */
bool is_receive_timeout_or_error(ssize_t result);

/**
 * @brief Standard buffer processing for all transports
 * @param buffer Buffer to process
 * @param buffer_size Size of buffer
 * @param received Number of bytes received
 * @return 0 on success, -1 on error
 */
int standard_buffer_processing(char* buffer, size_t buffer_size, ssize_t received);