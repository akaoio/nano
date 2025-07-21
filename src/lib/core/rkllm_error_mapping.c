#include "rkllm_error_mapping.h"
#include <json-c/json.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

// Comprehensive error mapping table
static const rkllm_error_mapping_t g_rkllm_error_map[] = {
    // Success
    {0, 0, "Success", "Operation completed successfully"},
    
    // Parameter errors
    {-1, JSON_RPC_INVALID_PARAMS, "Invalid parameters", "One or more parameters are invalid or missing"},
    {-4, JSON_RPC_INVALID_PARAMS, "Invalid handle", "The provided RKLLM handle is invalid or NULL"},
    {-11, JSON_RPC_RKLLM_CONFIG_ERROR, "Invalid configuration", "Model configuration parameters are invalid"},
    
    // Memory errors
    {-2, JSON_RPC_RKLLM_MEMORY_ERROR, "Memory allocation failed", "Insufficient memory to complete operation"},
    
    // Model errors
    {-3, JSON_RPC_RKLLM_INIT_FAILED, "Model loading failed", "Failed to load RKLLM model from file"},
    {-7, JSON_RPC_RKLLM_INVALID_MODEL, "Invalid model format", "Model file format is not supported or corrupted"},
    
    // State errors
    {-5, JSON_RPC_RKLLM_NOT_INITIALIZED, "Not initialized", "RKLLM is not initialized, call rkllm_init first"},
    {-6, JSON_RPC_RKLLM_INIT_FAILED, "Already initialized", "RKLLM is already initialized"},
    {-15, JSON_RPC_RKLLM_BUSY, "System busy", "RKLLM is busy processing another request"},
    {-16, JSON_RPC_RKLLM_BUSY, "Queue full", "Operation queue is full, try again later"},
    
    // Operation errors
    {-8, JSON_RPC_RKLLM_INFERENCE_ERROR, "Inference failed", "Model inference operation failed"},
    {-9, JSON_RPC_RKLLM_ABORTED, "Operation aborted", "Operation was aborted by user request"},
    {-10, JSON_RPC_RKLLM_TIMEOUT, "Operation timeout", "Operation timed out before completion"},
    
    // File errors
    {-12, JSON_RPC_RKLLM_FILE_ERROR, "File not found", "Specified file does not exist"},
    {-13, JSON_RPC_RKLLM_FILE_ERROR, "File read error", "Failed to read from file"},
    
    // Feature errors
    {-14, JSON_RPC_RKLLM_UNSUPPORTED, "Feature not supported", "Requested feature is not supported"},
    
    // Generic errors
    {-99, JSON_RPC_INTERNAL_ERROR, "Internal error", "Internal RKLLM error occurred"},
    {-100, JSON_RPC_INTERNAL_ERROR, "Unknown error", "An unknown error occurred"},
    
    // Sentinel
    {0, 0, NULL, NULL}
};

// Map RKLLM error code to JSON-RPC error
int rkllm_map_error_to_json_rpc(int rkllm_error, const char** message, const char** data) {
    // Success case
    if (rkllm_error == 0) {
        if (message) *message = "Success";
        if (data) *data = NULL;
        return 0;
    }
    
    // Search mapping table
    for (int i = 0; g_rkllm_error_map[i].error_message != NULL; i++) {
        if (g_rkllm_error_map[i].rkllm_error_code == rkllm_error) {
            if (message) *message = g_rkllm_error_map[i].error_message;
            if (data) *data = g_rkllm_error_map[i].error_data;
            return g_rkllm_error_map[i].json_rpc_error_code;
        }
    }
    
    // Default for unmapped errors
    if (message) *message = "Unknown RKLLM error";
    if (data) *data = "Error code not in mapping table";
    return JSON_RPC_INTERNAL_ERROR;
}

// Create JSON-RPC error response
char* rkllm_create_error_response(uint32_t request_id, int rkllm_error, const char* method) {
    const char* error_message = NULL;
    const char* error_data = NULL;
    int json_rpc_code = rkllm_map_error_to_json_rpc(rkllm_error, &error_message, &error_data);
    
    // Create JSON response
    json_object* response = json_object_new_object();
    json_object_object_add(response, "jsonrpc", json_object_new_string("2.0"));
    json_object_object_add(response, "id", json_object_new_int(request_id));
    
    // Create error object
    json_object* error_obj = json_object_new_object();
    json_object_object_add(error_obj, "code", json_object_new_int(json_rpc_code));
    json_object_object_add(error_obj, "message", json_object_new_string(error_message));
    
    // Add data if available
    if (error_data || method) {
        json_object* data_obj = json_object_new_object();
        if (error_data) {
            json_object_object_add(data_obj, "details", json_object_new_string(error_data));
        }
        if (method) {
            json_object_object_add(data_obj, "method", json_object_new_string(method));
        }
        json_object_object_add(data_obj, "rkllm_error_code", json_object_new_int(rkllm_error));
        json_object_object_add(error_obj, "data", data_obj);
    }
    
    json_object_object_add(response, "error", error_obj);
    
    // Convert to string
    const char* json_str = json_object_to_json_string_ext(response, JSON_C_TO_STRING_PLAIN);
    char* result = strdup(json_str);
    json_object_put(response);
    
    return result;
}

// Get human-readable error message
const char* rkllm_get_error_message(int rkllm_error) {
    if (rkllm_error == 0) {
        return "Success";
    }
    
    for (int i = 0; g_rkllm_error_map[i].error_message != NULL; i++) {
        if (g_rkllm_error_map[i].rkllm_error_code == rkllm_error) {
            return g_rkllm_error_map[i].error_message;
        }
    }
    
    return "Unknown error";
}

// Helper function to log RKLLM errors with context
void rkllm_log_error(int rkllm_error, const char* function_name, const char* context) {
    const char* error_msg = rkllm_get_error_message(rkllm_error);
    
    if (rkllm_error == 0) {
        return; // Don't log success
    }
    
    if (context) {
        printf("❌ RKLLM Error in %s: %s (code: %d) - %s\n", 
               function_name ? function_name : "unknown", 
               error_msg, rkllm_error, context);
    } else {
        printf("❌ RKLLM Error in %s: %s (code: %d)\n", 
               function_name ? function_name : "unknown", 
               error_msg, rkllm_error);
    }
}