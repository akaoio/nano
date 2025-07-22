#ifndef MCP_PROTOCOL_H
#define MCP_PROTOCOL_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/**
 * @file protocol.h
 * @brief MCP Protocol Public API
 * 
 * This header defines the public interface for MCP protocol handling,
 * including JSON-RPC 2.0 compliance, streaming support, and error handling.
 */

#ifdef __cplusplus
extern "C" {
#endif

#define MCP_PROTOCOL_VERSION "2025-01-07"
#define MCP_JSONRPC_VERSION "2.0"

/**
 * @brief MCP protocol states
 */
typedef enum {
    MCP_STATE_DISCONNECTED = 0,
    MCP_STATE_CONNECTED,
    MCP_STATE_INITIALIZING,
    MCP_STATE_INITIALIZED,
    MCP_STATE_ERROR
} mcp_state_t;

/**
 * @brief MCP error codes (JSON-RPC 2.0 compatible)
 */
typedef enum {
    MCP_ERROR_PARSE = -32700,
    MCP_ERROR_INVALID_REQUEST = -32600,
    MCP_ERROR_METHOD_NOT_FOUND = -32601,
    MCP_ERROR_INVALID_PARAMS = -32602,
    MCP_ERROR_INTERNAL = -32603,
    MCP_ERROR_NOT_INITIALIZED = -32002,
    MCP_ERROR_ALREADY_INITIALIZED = -32003,
    MCP_ERROR_INVALID_VERSION = -32004,
    // Streaming-specific errors
    MCP_ERROR_STREAM_NOT_FOUND = -32001,
    MCP_ERROR_STREAM_EXPIRED = -32005,
    MCP_ERROR_STREAM_INVALID_STATE = -32006
} mcp_error_code_t;

// Forward declaration
typedef struct mcp_context mcp_context_t;

/**
 * @brief Initialize MCP protocol context
 * @param ctx Protocol context to initialize
 * @return 0 on success, negative on error
 */
int mcp_protocol_init(mcp_context_t* ctx);

/**
 * @brief Shutdown MCP protocol context
 * @param ctx Protocol context
 */
void mcp_protocol_shutdown(mcp_context_t* ctx);

/**
 * @brief Format JSON-RPC 2.0 request
 * @param id Request ID
 * @param method Method name
 * @param params Parameters JSON string
 * @param buffer Output buffer
 * @param buffer_size Buffer size
 * @return 0 on success, negative on error
 */
int mcp_format_request(uint32_t id, const char* method, const char* params, 
                      char* buffer, size_t buffer_size);

/**
 * @brief Format JSON-RPC 2.0 response
 * @param id Request ID
 * @param result Result JSON string
 * @param buffer Output buffer
 * @param buffer_size Buffer size
 * @return 0 on success, negative on error
 */
int mcp_format_response(uint32_t id, const char* result, 
                       char* buffer, size_t buffer_size);

/**
 * @brief Format JSON-RPC 2.0 error response
 * @param id Request ID
 * @param code Error code
 * @param message Error message
 * @param data Additional error data (can be NULL)
 * @param buffer Output buffer
 * @param buffer_size Buffer size
 * @return 0 on success, negative on error
 */
int mcp_format_error(uint32_t id, mcp_error_code_t code, const char* message, 
                    const char* data, char* buffer, size_t buffer_size);

/**
 * @brief Format JSON-RPC 2.0 notification
 * @param method Method name
 * @param params Parameters JSON string
 * @param buffer Output buffer
 * @param buffer_size Buffer size
 * @return 0 on success, negative on error
 */
int mcp_format_notification(const char* method, const char* params, 
                           char* buffer, size_t buffer_size);

/**
 * @brief Get error message for error code
 * @param code Error code
 * @return Error message string (do not free)
 */
const char* mcp_error_message(mcp_error_code_t code);

// Streaming API

/**
 * @brief Format streaming chunk message
 * @param method Original method name
 * @param request_id JSON-RPC request ID (reused for streaming)
 * @param seq Sequence number
 * @param delta Content delta
 * @param end End of stream flag
 * @param error_msg Error message (if any)
 * @param buffer Output buffer
 * @param buffer_size Buffer size
 * @return 0 on success, negative on error
 */
int mcp_format_stream_chunk(const char* method, const char* request_id, 
                           uint32_t seq, const char* delta, bool end, 
                           const char* error_msg, char* buffer, size_t buffer_size);

/**
 * @brief Parse streaming request parameters
 * @param params Parameters JSON string
 * @param is_stream Output: Whether this is a streaming request
 * @param other_params Output: Other parameters (with stream flag removed)
 * @param params_size Size of other_params buffer
 * @return 0 on success, negative on error
 */
int mcp_parse_stream_request(const char* params, bool* is_stream, 
                            char* other_params, size_t params_size);

/**
 * @brief Handle stream polling request
 * @param request_id JSON-RPC request ID (reused for streaming)
 * @param from_seq Starting sequence number
 * @param response Output response buffer
 * @param response_size Response buffer size
 * @return 0 on success, negative on error
 */
int mcp_handle_stream_poll_request(const char* request_id, uint32_t from_seq, 
                                  char* response, size_t response_size);

#ifdef __cplusplus
}
#endif

#endif // MCP_PROTOCOL_H