#ifndef RKLLM_ARRAY_UTILS_H
#define RKLLM_ARRAY_UTILS_H

#include <stddef.h>
#include <stdbool.h>
#include <json-c/json.h>
#include <pthread.h>

// Dynamic array structure for managing RKLLM arrays
typedef struct {
    void* data;              // Actual array data
    size_t element_size;     // Size of each element (sizeof(float), sizeof(int32_t), etc.)
    size_t length;           // Number of elements
    size_t capacity;         // Allocated capacity (may be larger than length)
    bool owned;              // Whether we own the memory (true = we allocate, false = external)
    int ref_count;           // Reference counting for automatic cleanup
    pthread_mutex_t mutex;   // Thread safety for ref counting
} rkllm_dynamic_array_t;

// Memory pool for efficient array allocation
typedef struct {
    void* pool_memory;       // Pre-allocated memory pool
    size_t pool_size;        // Total pool size
    size_t used_size;        // Currently used size
    size_t allocation_count; // Number of active allocations
    pthread_mutex_t mutex;   // Thread safety
} rkllm_memory_pool_t;

// Array conversion functions for JSON to native types
int rkllm_convert_float_array(json_object* json_array, float** output, size_t* length);
int rkllm_convert_int32_array(json_object* json_array, int32_t** output, size_t* length);
int rkllm_convert_int_array(json_object* json_array, int** output, size_t* length);

// Multi-dimensional array support
int rkllm_convert_float_array_2d(json_object* json_array, float** output, 
                                 size_t* dim1, size_t* dim2);
int rkllm_convert_float_array_4d(json_object* json_array, float** output,
                                 size_t* dim1, size_t* dim2, size_t* dim3, size_t* dim4);

// Dynamic array management
rkllm_dynamic_array_t* rkllm_array_create(size_t element_size, size_t length);
void rkllm_array_destroy(rkllm_dynamic_array_t* array);
int rkllm_array_ref(rkllm_dynamic_array_t* array);
int rkllm_array_unref(rkllm_dynamic_array_t* array);

// Memory pool management
rkllm_memory_pool_t* rkllm_pool_create(size_t pool_size);
void rkllm_pool_destroy(rkllm_memory_pool_t* pool);
void* rkllm_pool_alloc(rkllm_memory_pool_t* pool, size_t size);
void rkllm_pool_free(rkllm_memory_pool_t* pool, void* ptr, size_t size);
void rkllm_pool_reset(rkllm_memory_pool_t* pool);

// Utility functions
size_t rkllm_calculate_array_size(size_t element_size, size_t* dimensions, int num_dims);
void rkllm_free_array(void* array);

// Global memory pool for arrays (initialized in rkllm_proxy_init)
extern rkllm_memory_pool_t* g_rkllm_array_pool;

#endif // RKLLM_ARRAY_UTILS_H