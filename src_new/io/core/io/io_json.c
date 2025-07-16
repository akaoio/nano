#define _GNU_SOURCE
#include "io.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

int io_parse_json_request(const char* json_request, uint32_t* request_id, 
                         uint32_t* handle_id, char* method, char* params) {
    if (!json_request || !request_id || !handle_id || !method || !params) {
        return IO_ERROR;
    }
    
    // Initialize outputs
    *request_id = 0;
    *handle_id = 0;
    method[0] = '\0';
    params[0] = '\0';
    
    // Extract request ID
    const char* id_start = strstr(json_request, "\"id\":");
    if (id_start) {
        sscanf(id_start + 5, "%u", request_id);
    }
    
    // Extract method
    const char* method_start = strstr(json_request, "\"method\":\"");
    if (method_start) {
        sscanf(method_start + 10, "%31[^\"]", method);
    }
    
    // Extract params
    const char* params_start = strstr(json_request, "\"params\":");
    if (params_start) {
        const char* params_start_brace = strchr(params_start + 9, '{');
        if (params_start_brace) {
            const char* params_end = strchr(params_start_brace, '}');
            if (params_end) {
                size_t len = params_end - params_start_brace + 1;
                if (len < 4096) {
                    strncpy(params, params_start_brace, len);
                    params[len] = '\0';
                }
            }
        }
    }
    
    // Extract handle_id from params if not init method
    if (strcmp(method, "init") != 0) {
        const char* handle_start = strstr(params, "\"handle_id\":");
        if (handle_start) {
            sscanf(handle_start + 12, "%u", handle_id);
        }
    }
    
    return IO_OK;
}
