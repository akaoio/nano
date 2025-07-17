#include "json_utils.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

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

float json_get_float(const char* json, const char* key, float default_val) {
    char buffer[64];
    const char* value = json_get_string(json, key, buffer, sizeof(buffer));
    return value ? (float)atof(value) : default_val;
}

bool json_get_bool(const char* json, const char* key, bool default_val) {
    char buffer[64];
    const char* value = json_get_string(json, key, buffer, sizeof(buffer));
    if (!value) return default_val;
    
    return (strcmp(value, "true") == 0 || strcmp(value, "1") == 0);
}

int json_extract_string_safe(const char* json, const char* key, char* buffer, size_t buffer_size) {
    if (!json || !key || !buffer || buffer_size == 0) return -1;
    
    char key_str[256];
    snprintf(key_str, sizeof(key_str), "\"%s\":", key);
    const char* start = strstr(json, key_str);
    if (!start) return -1;
    
    start += strlen(key_str);
    while (*start == ' ' || *start == '\t' || *start == '\n' || *start == '\r') start++;
    
    if (*start != '"') return -1;
    start++;
    
    const char* end = strchr(start, '"');
    if (!end) return -1;
    
    size_t length = end - start;
    if (length >= buffer_size) return -1;
    
    strncpy(buffer, start, length);
    buffer[length] = '\0';
    return 0;
}

int json_extract_strings(const char* json, const char* keys[], char* buffers[], size_t buffer_sizes[], int count) {
    if (!json || !keys || !buffers || !buffer_sizes || count <= 0) return 0;
    
    int extracted = 0;
    for (int i = 0; i < count; i++) {
        if (json_extract_string_safe(json, keys[i], buffers[i], buffer_sizes[i]) == 0) {
            extracted++;
        }
    }
    return extracted;
}

int json_extract_object(const char* json, const char* key, char* buffer, size_t buffer_size) {
    if (!json || !key || !buffer || buffer_size == 0) return -1;
    
    char key_str[256];
    snprintf(key_str, sizeof(key_str), "\"%s\":", key);
    const char* start = strstr(json, key_str);
    if (!start) return -1;
    
    start += strlen(key_str);
    while (*start == ' ' || *start == '\t' || *start == '\n' || *start == '\r') start++;
    
    if (*start != '{') return -1;
    
    // Find matching closing brace
    int brace_count = 1;
    const char* end = start + 1;
    while (*end && brace_count > 0) {
        if (*end == '{') brace_count++;
        else if (*end == '}') brace_count--;
        end++;
    }
    
    if (brace_count != 0) return -1;
    
    size_t length = end - start;
    if (length >= buffer_size) return -1;
    
    strncpy(buffer, start, length);
    buffer[length] = '\0';
    return 0;
}

uint32_t json_get_uint32(const char* json, const char* key, uint32_t default_val) {
    char buffer[64];
    const char* value = json_get_string(json, key, buffer, sizeof(buffer));
    return value ? (uint32_t)strtoul(value, NULL, 10) : default_val;
}
