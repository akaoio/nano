#include "format_response.h"
#include <stdlib.h>
#include <string.h>

char* format_response(json_object* id, json_object* result) {
    json_object* response = json_object_new_object();
    if (!response) {
        return NULL;
    }
    
    // Add jsonrpc version
    json_object* version = json_object_new_string("2.0");
    json_object_object_add(response, "jsonrpc", version);
    
    // Add id (can be null)
    if (id) {
        json_object_object_add(response, "id", json_object_get(id));
    } else {
        json_object_object_add(response, "id", NULL);
    }
    
    // Add result
    if (result) {
        json_object_object_add(response, "result", json_object_get(result));
    }
    
    // Convert to string
    const char* json_str = json_object_to_json_string(response);
    char* result_str = NULL;
    if (json_str) {
        result_str = strdup(json_str);
        if (!result_str) {
            json_object_put(response);
            return NULL; // Memory allocation failed
        }
    }
    
    json_object_put(response);
    return result_str;
}