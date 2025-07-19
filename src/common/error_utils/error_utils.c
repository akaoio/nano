#include "error_utils.h"
#include <json-c/json.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char* create_error_result(int code, const char* message) {
    if (!message) message = "Unknown error";
    
    json_object *root = json_object_new_object();
    json_object *error = json_object_new_object();
    json_object *code_obj = json_object_new_int(code);
    json_object *message_obj = json_object_new_string(message);
    
    json_object_object_add(error, "code", code_obj);
    json_object_object_add(error, "message", message_obj);
    json_object_object_add(root, "error", error);
    
    const char *json_str = json_object_to_json_string(root);
    char *result = strdup(json_str);
    
    json_object_put(root);
    return result;
}

char* create_success_result(const char* message) {
    if (!message) message = "Success";
    
    json_object *root = json_object_new_object();
    json_object *result_obj = json_object_new_string(message);
    
    json_object_object_add(root, "result", result_obj);
    
    const char *json_str = json_object_to_json_string(root);
    char *result = strdup(json_str);
    
    json_object_put(root);
    return result;
}