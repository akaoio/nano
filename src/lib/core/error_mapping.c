#include "error_mapping.h"
#include "common/time_utils/time_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

static const error_mapping_t g_error_mappings[] = {
    {
        RKLLM_SUCCESS, 
        0, 
        "Success", 
        "Operation completed successfully"
    },
    {
        RKLLM_INVALID_PARAM, 
        JSON_RPC_INVALID_PARAMS, 
        "Invalid method parameter(s)", 
        "One or more parameters provided to the method are invalid or missing"
    },
    {
        RKLLM_MODEL_NOT_FOUND, 
        JSON_RPC_INTERNAL_ERROR, 
        "Model file not found", 
        "The specified RKLLM model file could not be located or accessed"
    },
    {
        RKLLM_MEMORY_ERROR, 
        JSON_RPC_INTERNAL_ERROR, 
        "Memory allocation failed", 
        "Insufficient memory available for the requested operation"
    },
    {
        RKLLM_INFERENCE_ERROR, 
        JSON_RPC_INTERNAL_ERROR, 
        "Inference execution failed", 
        "An error occurred during model inference processing"
    },
    {
        RKLLM_DEVICE_ERROR, 
        JSON_RPC_INTERNAL_ERROR, 
        "NPU device error", 
        "Neural Processing Unit (NPU) hardware or driver error"
    },
    {
        RKLLM_TIMEOUT_ERROR, 
        JSON_RPC_INTERNAL_ERROR, 
        "Operation timeout", 
        "The requested operation exceeded the allowed time limit"
    },
    {
        RKLLM_CONTEXT_ERROR, 
        JSON_RPC_INVALID_PARAMS, 
        "Context length exceeded", 
        "Input context exceeds the model's maximum context window size"
    },
    {
        RKLLM_TOKEN_ERROR, 
        JSON_RPC_INVALID_PARAMS, 
        "Invalid token input", 
        "Token input format or content is invalid for the model"
    },
    {
        RKLLM_CALLBACK_ERROR, 
        JSON_RPC_INTERNAL_ERROR, 
        "Callback execution failed", 
        "An error occurred while executing the streaming callback function"
    },
    {
        RKLLM_FILE_ERROR, 
        JSON_RPC_INTERNAL_ERROR, 
        "File access error", 
        "Unable to read, write, or access required file"
    },
    {
        RKLLM_NETWORK_ERROR, 
        JSON_RPC_INTERNAL_ERROR, 
        "Network communication error", 
        "Network connection or communication failure"
    },
    {
        RKLLM_PERMISSION_ERROR, 
        JSON_RPC_INTERNAL_ERROR, 
        "Permission denied", 
        "Insufficient permissions to access required resources"
    },
    {
        RKLLM_VERSION_ERROR, 
        JSON_RPC_INTERNAL_ERROR, 
        "Version incompatibility", 
        "Model or library version incompatibility detected"
    },
    {
        RKLLM_INIT_ERROR, 
        JSON_RPC_INTERNAL_ERROR, 
        "Initialization failed", 
        "RKLLM library initialization failed"
    },
    {
        RKLLM_RESOURCE_BUSY, 
        JSON_RPC_INTERNAL_ERROR, 
        "Resource busy", 
        "Requested resource is currently in use by another operation"
    },
    {
        -1, 
        JSON_RPC_INTERNAL_ERROR, 
        "Unknown RKLLM error", 
        "An unrecognized error occurred in the RKLLM library"
    } // Sentinel
};

static bool g_error_mapping_initialized = false;
static FILE* g_error_log = NULL;

int error_mapping_init(void) {
    if (g_error_mapping_initialized) return 0;
    
    // Open error log file
    g_error_log = fopen("rkllm_errors.log", "a");
    if (!g_error_log) {
        fprintf(stderr, "Warning: Could not open error log file\n");
        // Continue without file logging
    }
    
    g_error_mapping_initialized = true;
    
    if (g_error_log) {
        fprintf(g_error_log, "[%llu] Error mapping system initialized\n", 
                (unsigned long long)get_timestamp_ms());
        fflush(g_error_log);
    }
    
    return 0;
}

void error_mapping_shutdown(void) {
    if (!g_error_mapping_initialized) return;
    
    if (g_error_log) {
        fprintf(g_error_log, "[%llu] Error mapping system shutdown\n", 
                (unsigned long long)get_timestamp_ms());
        fclose(g_error_log);
        g_error_log = NULL;
    }
    
    g_error_mapping_initialized = false;
}

bool error_mapping_is_initialized(void) {
    return g_error_mapping_initialized;
}

int map_rkllm_error_to_json_rpc(int rkllm_code, int* json_rpc_code, 
                                const char** message, const char** description) {
    if (!json_rpc_code || !message || !description) return -1;
    
    // Search for exact match
    for (int i = 0; g_error_mappings[i].rkllm_code != -1; i++) {
        if (g_error_mappings[i].rkllm_code == rkllm_code) {
            *json_rpc_code = g_error_mappings[i].json_rpc_code;
            *message = g_error_mappings[i].message;
            *description = g_error_mappings[i].description;
            return 0;
        }
    }
    
    // Default mapping for unknown errors (use sentinel entry)
    const error_mapping_t* default_mapping = &g_error_mappings[
        sizeof(g_error_mappings) / sizeof(error_mapping_t) - 1
    ];
    
    *json_rpc_code = default_mapping->json_rpc_code;
    *message = default_mapping->message;
    *description = default_mapping->description;
    
    return -1; // Indicate default mapping was used
}

json_object* create_error_response_from_rkllm(int rkllm_code, const char* request_id, 
                                             json_object* additional_data) {
    int json_rpc_code;
    const char* message;
    const char* description;
    
    map_rkllm_error_to_json_rpc(rkllm_code, &json_rpc_code, &message, &description);
    
    json_object* response = json_object_new_object();
    json_object* error = json_object_new_object();
    json_object* data = json_object_new_object();
    
    // JSON-RPC 2.0 response structure
    json_object_object_add(response, "jsonrpc", json_object_new_string("2.0"));
    json_object_object_add(response, "id", 
                          request_id ? json_object_new_string(request_id) : NULL);
    
    // Error object
    json_object_object_add(error, "code", json_object_new_int(json_rpc_code));
    json_object_object_add(error, "message", json_object_new_string(message));
    
    // Error data with RKLLM-specific information
    json_object_object_add(data, "rkllm_code", json_object_new_int(rkllm_code));
    json_object_object_add(data, "description", json_object_new_string(description));
    json_object_object_add(data, "source", json_object_new_string("RKLLM"));
    json_object_object_add(data, "timestamp", 
                          json_object_new_int64((int64_t)get_timestamp_ms()));
    json_object_object_add(data, "recoverable", 
                          json_object_new_boolean(is_rkllm_error_recoverable(rkllm_code)));
    
    // Add additional data if provided
    if (additional_data) {
        json_object_object_foreach(additional_data, key, val) {
            json_object_get(val); // Increment reference count
            json_object_object_add(data, key, val);
        }
    }
    
    json_object_object_add(error, "data", data);
    json_object_object_add(response, "error", error);
    
    // Log the error
    if (g_error_log) {
        const char* response_str = json_object_to_json_string(response);
        fprintf(g_error_log, "[%llu] RKLLM Error Response: %s\n", 
                (unsigned long long)get_timestamp_ms(), response_str);
        fflush(g_error_log);
    }
    
    return response;
}

json_object* create_json_rpc_error_response(int json_rpc_code, const char* message, 
                                           const char* request_id, json_object* additional_data) {
    json_object* response = json_object_new_object();
    json_object* error = json_object_new_object();
    
    // JSON-RPC 2.0 response structure
    json_object_object_add(response, "jsonrpc", json_object_new_string("2.0"));
    json_object_object_add(response, "id", 
                          request_id ? json_object_new_string(request_id) : NULL);
    
    // Error object
    json_object_object_add(error, "code", json_object_new_int(json_rpc_code));
    json_object_object_add(error, "message", json_object_new_string(message ? message : "Internal error"));
    
    if (additional_data) {
        json_object_get(additional_data); // Increment reference count
        json_object_object_add(error, "data", additional_data);
    }
    
    json_object_object_add(response, "error", error);
    
    return response;
}

const char* get_rkllm_error_description(int rkllm_code) {
    int json_rpc_code;
    const char* message;
    const char* description;
    
    map_rkllm_error_to_json_rpc(rkllm_code, &json_rpc_code, &message, &description);
    return description;
}

bool is_rkllm_error_recoverable(int rkllm_code) {
    switch (rkllm_code) {
        case RKLLM_SUCCESS:
            return true;
            
        // Recoverable errors (retry possible)
        case RKLLM_TIMEOUT_ERROR:
        case RKLLM_NETWORK_ERROR:
        case RKLLM_RESOURCE_BUSY:
        case RKLLM_CALLBACK_ERROR:
            return true;
            
        // Non-recoverable errors (require intervention)
        case RKLLM_INVALID_PARAM:
        case RKLLM_MODEL_NOT_FOUND:
        case RKLLM_MEMORY_ERROR:
        case RKLLM_DEVICE_ERROR:
        case RKLLM_CONTEXT_ERROR:
        case RKLLM_TOKEN_ERROR:
        case RKLLM_FILE_ERROR:
        case RKLLM_PERMISSION_ERROR:
        case RKLLM_VERSION_ERROR:
        case RKLLM_INIT_ERROR:
        default:
            return false;
    }
}

void log_rkllm_error(int rkllm_code, const char* context, const char* function, 
                     const char* file, int line) {
    if (!g_error_mapping_initialized) return;
    
    const char* description = get_rkllm_error_description(rkllm_code);
    bool recoverable = is_rkllm_error_recoverable(rkllm_code);
    
    // Log to stderr for immediate visibility
    fprintf(stderr, "[ERROR] RKLLM Code %d in %s (%s:%d): %s%s%s\n", 
            rkllm_code, function, file, line, description,
            context ? " - Context: " : "",
            context ? context : "");
    
    // Log to file if available
    if (g_error_log) {
        fprintf(g_error_log, 
                "[%llu] RKLLM Error %d in %s (%s:%d): %s%s%s [Recoverable: %s]\n",
                (unsigned long long)get_timestamp_ms(),
                rkllm_code, function, file, line, description,
                context ? " - Context: " : "",
                context ? context : "",
                recoverable ? "Yes" : "No");
        fflush(g_error_log);
    }
}