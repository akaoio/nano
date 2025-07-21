#ifndef STREAMING_INTEGRATION_H
#define STREAMING_INTEGRATION_H

#include "rkllm_streaming_context.h"
#include "../transport/streaming_buffer_manager.h"
#include "../transport/manager.h"
#include <stdint.h>
#include <stdbool.h>

// Integration layer for RKLLM streaming with transport layer
// Provides high-level API for streaming operations

// Streaming operation result codes
typedef enum {
    STREAMING_INTEGRATION_OK = 0,
    STREAMING_INTEGRATION_ERROR = -1,
    STREAMING_INTEGRATION_INVALID_PARAMS = -2,
    STREAMING_INTEGRATION_NOT_INITIALIZED = -3,
    STREAMING_INTEGRATION_TRANSPORT_ERROR = -4,
    STREAMING_INTEGRATION_STREAM_ERROR = -5
} streaming_integration_result_t;

// Global streaming integration manager
typedef struct {
    bool initialized;
    enhanced_transport_manager_t* enhanced_managers[8]; // Support for multiple transports
    int active_managers_count;
    
    // Default configurations
    transport_stream_config_t default_websocket_config;
    transport_stream_config_t default_http_config;
    transport_stream_config_t default_tcp_config;
    
    // Statistics
    uint32_t total_streams_created;
    uint32_t active_streams_count;
    
} streaming_integration_manager_t;

// Function declarations

/**
 * @brief Initialize streaming integration system
 * @return STREAMING_INTEGRATION_OK on success
 */
streaming_integration_result_t streaming_integration_init(void);

/**
 * @brief Shutdown streaming integration system
 */
void streaming_integration_shutdown(void);

/**
 * @brief Create enhanced transport manager for streaming
 * @param base_manager Base transport manager
 * @param transport_name Transport name for optimal configuration
 * @return Enhanced transport manager or NULL on error
 */
enhanced_transport_manager_t* streaming_integration_create_enhanced_transport(
    transport_manager_t* base_manager, const char* transport_name);

/**
 * @brief Start streaming operation with transport integration
 * @param request_id JSON-RPC request ID
 * @param handle RKLLM handle
 * @param transport_manager Enhanced transport manager
 * @return Session context or NULL on error
 */
rkllm_stream_context_t* streaming_integration_start_stream(
    uint32_t request_id, LLMHandle handle, enhanced_transport_manager_t* transport_manager);

/**
 * @brief Stop streaming operation
 * @param session_id Session identifier
 * @return STREAMING_INTEGRATION_OK on success
 */
streaming_integration_result_t streaming_integration_stop_stream(const char* session_id);

/**
 * @brief Get streaming operation status
 * @param session_id Session identifier
 * @param status_json Output JSON string with status
 * @return STREAMING_INTEGRATION_OK on success
 */
streaming_integration_result_t streaming_integration_get_status(
    const char* session_id, char** status_json);

/**
 * @brief List all active streaming sessions
 * @param sessions_json Output JSON array of active sessions
 * @return STREAMING_INTEGRATION_OK on success
 */
streaming_integration_result_t streaming_integration_list_active_streams(char** sessions_json);

/**
 * @brief Handle streaming JSON-RPC request
 * @param method Method name (e.g., "rkllm_run_streaming")
 * @param params_json Parameters JSON string
 * @param request_id JSON-RPC request ID
 * @param transport_manager Enhanced transport manager for response
 * @param result_json Output result JSON string
 * @return STREAMING_INTEGRATION_OK on success
 */
streaming_integration_result_t streaming_integration_handle_request(
    const char* method, const char* params_json, uint32_t request_id,
    enhanced_transport_manager_t* transport_manager, char** result_json);

#endif // STREAMING_INTEGRATION_H