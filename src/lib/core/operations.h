#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "../../external/rkllm/rkllm.h"
#include "mcp/transport.h"

// IO Operations Interface - Direct RKLLM integration with streaming support
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


// Streaming Support



/**
 * @brief Check if request is a streaming request
 * @param params_json Parameters JSON string
 * @return true if streaming request, false otherwise
 */
bool io_is_streaming_request(const char* params_json);

/**
 * @brief Add chunk to active stream using request ID
 * @param request_id JSON-RPC request ID  
 * @param delta Chunk content
 * @param end End of stream flag
 * @param error_msg Error message (if any)
 * @return 0 on success, -1 on error
 */
int io_add_stream_chunk(const char* request_id, const char* delta, bool end, const char* error_msg);

