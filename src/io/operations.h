#pragma once

// IO Operations Interface
// Bridge between IO core and RKLLM proxy

/**
 * @brief Initialize IO operations system
 * @return 0 on success, -1 on error
 */
int io_operations_init(void);

/**
 * @brief Process IO operation request
 * @param json_request JSON-RPC request
 * @param json_response Output buffer for JSON-RPC response
 * @param max_response_len Maximum response buffer length
 * @return 0 on success, -1 on error
 */
int io_process_request(const char* json_request, char* json_response, size_t max_response_len);

/**
 * @brief Shutdown IO operations system
 */
void io_operations_shutdown(void);
