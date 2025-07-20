#define _GNU_SOURCE
#include "operations.h"
#include "common/types.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * Simplified IO System - Direct synchronous request processing
 * 
 * All requests are processed directly and synchronously since RKLLM
 * inference is single-threaded. This eliminates unnecessary queue
 * and threading complexity.
 */

// Global callback for responses
static nano_callback_t g_response_callback = NULL;
static void* g_callback_userdata = NULL;

// System state
static bool g_system_initialized = false;

/**
 * @brief Initialize IO system with callback
 * @param callback Function to call when response is ready
 * @param userdata Data to pass to callback
 * @return IO_OK on success, IO_ERROR on failure
 */
int io_init(nano_callback_t callback, void* userdata) {
    if (g_system_initialized) {
        return IO_OK; // Already initialized
    }
    
    // Initialize operations subsystem
    if (io_operations_init() != 0) {
        return IO_ERROR;
    }
    
    g_response_callback = callback;
    g_callback_userdata = userdata;
    g_system_initialized = true;
    
    printf("✅ IO system initialized with direct processing\n");
    return IO_OK;
}

/**
 * @brief Process JSON request directly and synchronously
 * @param json_request JSON-RPC request string
 * @return IO_OK on success, error code on failure
 */
int io_push_request(const char* json_request) {
    if (!g_system_initialized || !json_request) {
        return IO_ERROR;
    }
    
    // Process request directly with the original JSON
    char response[4096] = {0};
    int result = io_process_request(json_request, response, sizeof(response));
    
    // Send response via callback if available
    if (g_response_callback && response[0]) {
        g_response_callback(response, g_callback_userdata);
    }
    
    return (result == 0) ? IO_OK : IO_ERROR;
}

/**
 * @brief Set streaming callback for real-time responses
 * @param callback Function to call for streaming chunks
 * @param userdata Data to pass to callback
 */
void io_set_streaming_callback(nano_callback_t callback, void* userdata) {
    g_response_callback = callback;
    g_callback_userdata = userdata;
}

/**
 * @brief Shutdown IO system
 */
void io_shutdown(void) {
    if (!g_system_initialized) {
        return;
    }
    
    // Shutdown operations subsystem
    io_operations_shutdown();
    
    g_response_callback = NULL;
    g_callback_userdata = NULL;
    g_system_initialized = false;
    
    printf("✅ IO system shutdown complete\n");
}

/**
 * @brief Parse JSON request with handle extraction
 * @param json_request JSON request string
 * @param request_id Output request ID
 * @param handle_id Output handle ID
 * @param method Output method name
 * @param params Output parameters
 * @return 0 on success, -1 on error
 */
int io_parse_json_request_with_handle(const char* json_request, uint32_t* request_id, 
                                     uint32_t* handle_id, char* method, char* params) {
    if (!json_request || !request_id || !handle_id || !method || !params) {
        return -1;
    }
    
    // Simple JSON parsing - in a full implementation this would use json-c
    // For now, we'll extract basic fields with string parsing
    
    // Extract request ID (simplified)
    const char* id_pos = strstr(json_request, "\"id\":");
    if (id_pos) {
        *request_id = (uint32_t)atoi(id_pos + 5);
    }
    
    // Extract handle ID from params (simplified)
    const char* handle_pos = strstr(json_request, "\"handle_id\":");
    if (handle_pos) {
        *handle_id = (uint32_t)atoi(handle_pos + 12);
    }
    
    // Extract method (simplified)
    const char* method_pos = strstr(json_request, "\"method\":\"");
    if (method_pos) {
        const char* method_start = method_pos + 10;
        const char* method_end = strchr(method_start, '"');
        if (method_end) {
            size_t method_len = method_end - method_start;
            if (method_len < 63) {
                strncpy(method, method_start, method_len);
                method[method_len] = '\0';
            }
        }
    }
    
    // Extract params (simplified - just copy the whole params object)
    const char* params_pos = strstr(json_request, "\"params\":");
    if (params_pos) {
        strncpy(params, params_pos + 9, 1023);
        params[1023] = '\0';
    }
    
    return 0;
}