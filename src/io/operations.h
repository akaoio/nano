#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "rkllm.h"

// Operation function signature
typedef int (*operation_func_t)(uint32_t handle_id, const char* params, char* result, size_t result_size);

// Operation mapping
typedef struct {
    const char* method;
    operation_func_t func;
} operation_t;

// Execute a method
int execute_method(const char* method, uint32_t handle_id, const char* params, char* result, size_t result_size);

// Available operations
int method_init(uint32_t handle_id, const char* params, char* result, size_t result_size);
int method_run(uint32_t handle_id, const char* params, char* result, size_t result_size);
int method_destroy(uint32_t handle_id, const char* params, char* result, size_t result_size);
int method_status(uint32_t handle_id, const char* params, char* result, size_t result_size);

// JSON parsing utilities
const char* json_get_string(const char* json, const char* key, char* buffer, size_t buffer_size);
int json_get_int(const char* json, const char* key, int default_val);
double json_get_double(const char* json, const char* key, double default_val);
