#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "../libs/rkllm/rkllm.h"

// IO Operations Interface - Direct RKLLM integration (lightweight approach)
// Direct function calls to RKLLM without proxy abstraction

/**
 * @brief Parse JSON-RPC request and extract operation details (main version)
 * @param json_request JSON-RPC request string
 * @param request_id Output: Request ID from JSON-RPC
 * @param method Output: Method name (256 bytes)
 * @param params Output: Parameters JSON string (4096 bytes)
 * @return 0 on success, -1 on error
 */
int io_parse_json_request_main(const char* json_request, uint32_t* request_id, 
                               char* method, char* params);

/**
 * @brief Parse JSON-RPC request with handle ID (io version)
 * @param json_request JSON-RPC request string
 * @param request_id Output: Request ID from JSON-RPC
 * @param handle_id Output: Handle ID
 * @param method Output: Method name (32 bytes)
 * @param params Output: Parameters JSON string (4096 bytes)
 * @return 0 on success, -1 on error
 */
int io_parse_json_request_with_handle(const char* json_request, uint32_t* request_id, 
                                      uint32_t* handle_id, char* method, char* params);

/**
 * @brief Create JSON response
 * @param request_id Request ID
 * @param success Success flag
 * @param data Response data
 * @return Allocated JSON response string (caller must free)
 */
char* io_create_json_response(uint32_t request_id, bool success, const char* data);

/**
 * @brief Process operation request directly with RKLLM
 * @param method Operation method name
 * @param params_json Parameters JSON string
 * @param result_json Output: Result JSON string (caller must free)
 * @return 0 on success, -1 on error
 */
int io_process_operation(const char* method, const char* params_json, char** result_json);

/**
 * @brief Handle init operation
 * @param params_json Parameters JSON string
 * @param result_json Output: Result JSON string (caller must free)
 * @return 0 on success, -1 on error
 */
int io_handle_init(const char* params_json, char** result_json);

/**
 * @brief Handle run operation
 * @param params_json Parameters JSON string
 * @param result_json Output: Result JSON string (caller must free)
 * @return 0 on success, -1 on error
 */
int io_handle_run(const char* params_json, char** result_json);

/**
 * @brief Handle destroy operation
 * @param params_json Parameters JSON string (unused)
 * @param result_json Output: Result JSON string (caller must free)
 * @return 0 on success, -1 on error
 */
int io_handle_destroy(const char* params_json, char** result_json);

/**
 * @brief Get current RKLLM handle
 * @return Current handle or NULL if not initialized
 */
LLMHandle io_get_rkllm_handle(void);

/**
 * @brief Check if model is initialized
 * @return true if initialized, false otherwise
 */
bool io_is_initialized(void);

/**
 * @brief Initialize IO operations
 * @return 0 on success, -1 on error
 */
int io_operations_init(void);

/**
 * @brief Shutdown IO operations
 */
void io_operations_shutdown(void);

/**
 * @brief Process JSON request and return response
 * @param json_request JSON request string
 * @param response Output response buffer
 * @param response_size Size of response buffer
 * @return 0 on success, -1 on error
 */
int io_process_request(const char* json_request, char* response, size_t response_size);