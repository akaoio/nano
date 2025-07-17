#include "error_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char* create_error_result(int code, const char* message) {
    if (!message) message = "Unknown error";
    
    size_t len = snprintf(NULL, 0, "{\"error\":{\"code\":%d,\"message\":\"%s\"}}", code, message);
    char* result = malloc(len + 1);
    if (!result) return NULL;
    
    snprintf(result, len + 1, "{\"error\":{\"code\":%d,\"message\":\"%s\"}}", code, message);
    return result;
}

char* create_success_result(const char* message) {
    if (!message) message = "Success";
    
    size_t len = snprintf(NULL, 0, "{\"result\":\"%s\"}", message);
    char* result = malloc(len + 1);
    if (!result) return NULL;
    
    snprintf(result, len + 1, "{\"result\":\"%s\"}", message);
    return result;
}