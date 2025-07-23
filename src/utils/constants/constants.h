#ifndef CONSTANTS_H
#define CONSTANTS_H

/**
 * System-wide constants for buffer sizes and limits
 */

// Buffer size constants
#define CONNECTION_BUFFER_SIZE 8192      // Connection read buffer size
#define ERROR_MESSAGE_BUFFER_SIZE 512    // Error message buffer size  
#define SMALL_ERROR_BUFFER_SIZE 256      // Small error message buffer size
#define TIMESTAMP_BUFFER_SIZE 64         // Timestamp string buffer size

// String length limits
#define MAX_PATH_LENGTH 4096             // Maximum file path length
#define MAX_METHOD_NAME_LENGTH 128       // Maximum JSON-RPC method name length

// Timeout constants (in seconds)
#define INIT_TIMEOUT_SECONDS 30          // RKLLM init timeout
#define ASYNC_TIMEOUT_SECONDS 60         // Async operation timeout

#endif