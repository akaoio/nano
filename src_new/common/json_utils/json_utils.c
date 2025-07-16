#include "json_utils.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

const char* json_get_string(const char* json, const char* key, char* buffer, size_t buffer_size) {
    if (!json || !key || !buffer || buffer_size == 0) return NULL;
    
    char key_str[256];
    snprintf(key_str, sizeof(key_str), "\"%s\":", key);
    const char* start = strstr(json, key_str);
    if (!start) return NULL;
    
    start += strlen(key_str);
    while (*start == ' ' || *start == '\t' || *start == '\n' || *start == '\r') start++;
    
    const char* end;
    if (*start == '"') {
        // String value
        start++;
        end = strchr(start, '"');
        if (!end || (size_t)(end - start) >= buffer_size) return NULL;
    } else {
        // Number or other value
        end = start;
        while (*end && *end != ',' && *end != '}' && *end != ']' && 
               *end != ' ' && *end != '\t' && *end != '\n' && *end != '\r') {
            end++;
        }
        if ((size_t)(end - start) >= buffer_size) return NULL;
    }
    
    strncpy(buffer, start, end - start);
    buffer[end - start] = '\0';
    return buffer;
}

int json_get_int(const char* json, const char* key, int default_val) {
    char buffer[64];
    const char* value = json_get_string(json, key, buffer, sizeof(buffer));
    return value ? atoi(value) : default_val;
}

double json_get_double(const char* json, const char* key, double default_val) {
    char buffer[64];
    const char* value = json_get_string(json, key, buffer, sizeof(buffer));
    return value ? atof(value) : default_val;
}
