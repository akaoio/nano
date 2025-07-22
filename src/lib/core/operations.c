#define _DEFAULT_SOURCE

#include "operations.h"
#include "rkllm_proxy.h"
#include "../protocol/adapter.h"
#include "../protocol/mcp_protocol.h"
#include "../../external/rkllm/rkllm.h"
#include "../../common/string_utils/string_utils.h"
#include <json-c/json.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

// IO Operations - Direct RKLLM integration (lightweight approach)
// Handles JSON request parsing and direct RKLLM function calls

// Helper function to create JSON error response
static char* create_error_response(int code, const char* message) {
    json_object* response = json_object_new_object();
    json_object* error_obj = json_object_new_object();
    
    json_object_object_add(error_obj, "code", json_object_new_int(code));
    json_object_object_add(error_obj, "message", json_object_new_string(message));
    json_object_object_add(response, "error", error_obj);
    
    const char* json_str = json_object_to_json_string(response);
    char* result = strdup(json_str);
    json_object_put(response);
    
    return result;
}

// Main operation handler - routes to RKLLM functions
int operations_handle_request(const char* method, const char* params_json, char** result_json) {
    if (!method || !result_json) {
        return -1;
    }

    // Initialize result
    *result_json = NULL;

    // Route to RKLLM proxy for all RKLLM function calls
    if (strncmp(method, "rkllm_", 6) == 0) {
        return rkllm_proxy_call(method, params_json, result_json);
    }

    // Handle MCP protocol methods
    if (strcmp(method, "initialize") == 0) {
        // MCP initialization
        json_object* response = json_object_new_object();
        json_object_object_add(response, "protocolVersion", json_object_new_string("2024-11-05"));
        json_object_object_add(response, "serverInfo", json_object_new_object());
        
        const char* json_str = json_object_to_json_string(response);
        *result_json = strdup(json_str);
        json_object_put(response);
        return 0;
    }

    if (strcmp(method, "ping") == 0) {
        *result_json = strdup("{\"pong\": true}");
        return 0;
    }

    if (strcmp(method, "poll") == 0) {
        // HTTP polling for streaming data
        char poll_response[8192];
        int result = mcp_handle_stream_poll_request("default", poll_response, sizeof(poll_response));
        *result_json = strdup(poll_response);
        return result;
    }

    // Unknown method
    *result_json = create_error_response(-32601, "Method not found");
    return -1;
}

// Initialize operations system
int operations_init(void) {
    return 0; // No initialization required for lightweight approach
}

// Cleanup operations system  
void operations_cleanup(void) {
    // No cleanup required for lightweight approach
}

// Legacy IO operation wrapper
int io_process_operation(const char* method, const char* params_json, char** result_json) {
    return operations_handle_request(method, params_json, result_json);
}

// Legacy shutdown wrapper
void io_operations_shutdown(void) {
    operations_cleanup();
}

// Legacy init wrapper
int io_operations_init(void) {
    return operations_init();
}