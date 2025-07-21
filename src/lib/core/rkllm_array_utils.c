#include "rkllm_array_utils.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// Global memory pool for arrays
rkllm_memory_pool_t* g_rkllm_array_pool = NULL;

// Convert JSON array to float array
int rkllm_convert_float_array(json_object* json_array, float** output, size_t* length) {
    if (!json_array || !output || !length) {
        return -1;
    }
    
    if (json_object_get_type(json_array) != json_type_array) {
        return -1;
    }
    
    size_t array_len = json_object_array_length(json_array);
    if (array_len == 0) {
        *output = NULL;
        *length = 0;
        return 0;
    }
    
    // Allocate memory for float array
    float* array = NULL;
    if (g_rkllm_array_pool) {
        array = (float*)rkllm_pool_alloc(g_rkllm_array_pool, sizeof(float) * array_len);
    } else {
        array = (float*)malloc(sizeof(float) * array_len);
    }
    
    if (!array) {
        return -1;
    }
    
    // Convert each element
    for (size_t i = 0; i < array_len; i++) {
        json_object* elem = json_object_array_get_idx(json_array, i);
        if (!elem) {
            free(array);
            return -1;
        }
        
        // Handle both int and double JSON types
        if (json_object_get_type(elem) == json_type_double) {
            array[i] = (float)json_object_get_double(elem);
        } else if (json_object_get_type(elem) == json_type_int) {
            array[i] = (float)json_object_get_int(elem);
        } else {
            free(array);
            return -1;
        }
    }
    
    *output = array;
    *length = array_len;
    return 0;
}

// Convert JSON array to int32_t array
int rkllm_convert_int32_array(json_object* json_array, int32_t** output, size_t* length) {
    if (!json_array || !output || !length) {
        return -1;
    }
    
    if (json_object_get_type(json_array) != json_type_array) {
        return -1;
    }
    
    size_t array_len = json_object_array_length(json_array);
    if (array_len == 0) {
        *output = NULL;
        *length = 0;
        return 0;
    }
    
    // Allocate memory for int32_t array
    int32_t* array = NULL;
    if (g_rkllm_array_pool) {
        array = (int32_t*)rkllm_pool_alloc(g_rkllm_array_pool, sizeof(int32_t) * array_len);
    } else {
        array = (int32_t*)malloc(sizeof(int32_t) * array_len);
    }
    
    if (!array) {
        return -1;
    }
    
    // Convert each element
    for (size_t i = 0; i < array_len; i++) {
        json_object* elem = json_object_array_get_idx(json_array, i);
        if (!elem || json_object_get_type(elem) != json_type_int) {
            free(array);
            return -1;
        }
        array[i] = (int32_t)json_object_get_int(elem);
    }
    
    *output = array;
    *length = array_len;
    return 0;
}

// Convert JSON array to int array
int rkllm_convert_int_array(json_object* json_array, int** output, size_t* length) {
    if (!json_array || !output || !length) {
        return -1;
    }
    
    if (json_object_get_type(json_array) != json_type_array) {
        return -1;
    }
    
    size_t array_len = json_object_array_length(json_array);
    if (array_len == 0) {
        *output = NULL;
        *length = 0;
        return 0;
    }
    
    // Allocate memory for int array
    int* array = NULL;
    if (g_rkllm_array_pool) {
        array = (int*)rkllm_pool_alloc(g_rkllm_array_pool, sizeof(int) * array_len);
    } else {
        array = (int*)malloc(sizeof(int) * array_len);
    }
    
    if (!array) {
        return -1;
    }
    
    // Convert each element
    for (size_t i = 0; i < array_len; i++) {
        json_object* elem = json_object_array_get_idx(json_array, i);
        if (!elem || json_object_get_type(elem) != json_type_int) {
            free(array);
            return -1;
        }
        array[i] = json_object_get_int(elem);
    }
    
    *output = array;
    *length = array_len;
    return 0;
}

// Convert 2D JSON array to float array (row-major order)
int rkllm_convert_float_array_2d(json_object* json_array, float** output, 
                                size_t* dim1, size_t* dim2) {
    if (!json_array || !output || !dim1 || !dim2) {
        return -1;
    }
    
    if (json_object_get_type(json_array) != json_type_array) {
        return -1;
    }
    
    size_t rows = json_object_array_length(json_array);
    if (rows == 0) {
        *output = NULL;
        *dim1 = 0;
        *dim2 = 0;
        return 0;
    }
    
    // Get first row to determine column count
    json_object* first_row = json_object_array_get_idx(json_array, 0);
    if (!first_row || json_object_get_type(first_row) != json_type_array) {
        return -1;
    }
    
    size_t cols = json_object_array_length(first_row);
    size_t total_elements = rows * cols;
    
    // Allocate flat array for 2D data
    float* array = NULL;
    if (g_rkllm_array_pool) {
        array = (float*)rkllm_pool_alloc(g_rkllm_array_pool, sizeof(float) * total_elements);
    } else {
        array = (float*)malloc(sizeof(float) * total_elements);
    }
    
    if (!array) {
        return -1;
    }
    
    // Convert each element
    for (size_t i = 0; i < rows; i++) {
        json_object* row = json_object_array_get_idx(json_array, i);
        if (!row || json_object_get_type(row) != json_type_array ||
            json_object_array_length(row) != cols) {
            free(array);
            return -1;
        }
        
        for (size_t j = 0; j < cols; j++) {
            json_object* elem = json_object_array_get_idx(row, j);
            if (!elem) {
                free(array);
                return -1;
            }
            
            size_t index = i * cols + j;  // Row-major order
            if (json_object_get_type(elem) == json_type_double) {
                array[index] = (float)json_object_get_double(elem);
            } else if (json_object_get_type(elem) == json_type_int) {
                array[index] = (float)json_object_get_int(elem);
            } else {
                free(array);
                return -1;
            }
        }
    }
    
    *output = array;
    *dim1 = rows;
    *dim2 = cols;
    return 0;
}

// Convert 4D JSON array to float array (for encoder caches)
int rkllm_convert_float_array_4d(json_object* json_array, float** output,
                                size_t* dim1, size_t* dim2, size_t* dim3, size_t* dim4) {
    if (!json_array || !output || !dim1 || !dim2 || !dim3 || !dim4) {
        return -1;
    }
    
    // For 4D arrays like [num_layers][num_tokens][num_kv_heads][head_dim]
    // We expect nested arrays 4 levels deep
    
    if (json_object_get_type(json_array) != json_type_array) {
        return -1;
    }
    
    size_t d1 = json_object_array_length(json_array);
    if (d1 == 0) {
        *output = NULL;
        *dim1 = *dim2 = *dim3 = *dim4 = 0;
        return 0;
    }
    
    // Navigate through nested arrays to get dimensions
    json_object* level1 = json_object_array_get_idx(json_array, 0);
    if (!level1 || json_object_get_type(level1) != json_type_array) return -1;
    size_t d2 = json_object_array_length(level1);
    
    json_object* level2 = json_object_array_get_idx(level1, 0);
    if (!level2 || json_object_get_type(level2) != json_type_array) return -1;
    size_t d3 = json_object_array_length(level2);
    
    json_object* level3 = json_object_array_get_idx(level2, 0);
    if (!level3 || json_object_get_type(level3) != json_type_array) return -1;
    size_t d4 = json_object_array_length(level3);
    
    size_t total_elements = d1 * d2 * d3 * d4;
    
    // Allocate flat array for 4D data
    float* array = NULL;
    if (g_rkllm_array_pool) {
        array = (float*)rkllm_pool_alloc(g_rkllm_array_pool, sizeof(float) * total_elements);
    } else {
        array = (float*)malloc(sizeof(float) * total_elements);
    }
    
    if (!array) {
        return -1;
    }
    
    // Convert nested arrays to flat array
    size_t index = 0;
    for (size_t i = 0; i < d1; i++) {
        json_object* l1 = json_object_array_get_idx(json_array, i);
        if (!l1 || json_object_get_type(l1) != json_type_array) {
            free(array);
            return -1;
        }
        
        for (size_t j = 0; j < d2; j++) {
            json_object* l2 = json_object_array_get_idx(l1, j);
            if (!l2 || json_object_get_type(l2) != json_type_array) {
                free(array);
                return -1;
            }
            
            for (size_t k = 0; k < d3; k++) {
                json_object* l3 = json_object_array_get_idx(l2, k);
                if (!l3 || json_object_get_type(l3) != json_type_array) {
                    free(array);
                    return -1;
                }
                
                for (size_t l = 0; l < d4; l++) {
                    json_object* elem = json_object_array_get_idx(l3, l);
                    if (!elem) {
                        free(array);
                        return -1;
                    }
                    
                    if (json_object_get_type(elem) == json_type_double) {
                        array[index++] = (float)json_object_get_double(elem);
                    } else if (json_object_get_type(elem) == json_type_int) {
                        array[index++] = (float)json_object_get_int(elem);
                    } else {
                        free(array);
                        return -1;
                    }
                }
            }
        }
    }
    
    *output = array;
    *dim1 = d1;
    *dim2 = d2;
    *dim3 = d3;
    *dim4 = d4;
    return 0;
}

// Create a dynamic array
rkllm_dynamic_array_t* rkllm_array_create(size_t element_size, size_t length) {
    rkllm_dynamic_array_t* array = (rkllm_dynamic_array_t*)malloc(sizeof(rkllm_dynamic_array_t));
    if (!array) {
        return NULL;
    }
    
    array->element_size = element_size;
    array->length = length;
    array->capacity = length;
    array->owned = true;
    array->ref_count = 1;
    
    if (pthread_mutex_init(&array->mutex, NULL) != 0) {
        free(array);
        return NULL;
    }
    
    // Allocate data
    if (g_rkllm_array_pool) {
        array->data = rkllm_pool_alloc(g_rkllm_array_pool, element_size * length);
    } else {
        array->data = malloc(element_size * length);
    }
    
    if (!array->data) {
        pthread_mutex_destroy(&array->mutex);
        free(array);
        return NULL;
    }
    
    return array;
}

// Destroy a dynamic array
void rkllm_array_destroy(rkllm_dynamic_array_t* array) {
    if (!array) {
        return;
    }
    
    pthread_mutex_lock(&array->mutex);
    array->ref_count--;
    
    if (array->ref_count > 0) {
        pthread_mutex_unlock(&array->mutex);
        return;
    }
    
    // Free data if we own it
    if (array->owned && array->data) {
        if (g_rkllm_array_pool) {
            rkllm_pool_free(g_rkllm_array_pool, array->data, array->element_size * array->capacity);
        } else {
            free(array->data);
        }
    }
    
    pthread_mutex_unlock(&array->mutex);
    pthread_mutex_destroy(&array->mutex);
    free(array);
}

// Reference counting
int rkllm_array_ref(rkllm_dynamic_array_t* array) {
    if (!array) {
        return -1;
    }
    
    pthread_mutex_lock(&array->mutex);
    array->ref_count++;
    pthread_mutex_unlock(&array->mutex);
    
    return 0;
}

int rkllm_array_unref(rkllm_dynamic_array_t* array) {
    if (!array) {
        return -1;
    }
    
    rkllm_array_destroy(array);
    return 0;
}

// Memory pool creation
rkllm_memory_pool_t* rkllm_pool_create(size_t pool_size) {
    rkllm_memory_pool_t* pool = (rkllm_memory_pool_t*)malloc(sizeof(rkllm_memory_pool_t));
    if (!pool) {
        return NULL;
    }
    
    pool->pool_size = pool_size;
    pool->used_size = 0;
    pool->allocation_count = 0;
    
    if (pthread_mutex_init(&pool->mutex, NULL) != 0) {
        free(pool);
        return NULL;
    }
    
    pool->pool_memory = malloc(pool_size);
    if (!pool->pool_memory) {
        pthread_mutex_destroy(&pool->mutex);
        free(pool);
        return NULL;
    }
    
    printf("🏊 RKLLM Array Pool: Created with %zu MB capacity\n", pool_size / (1024 * 1024));
    return pool;
}

// Memory pool destruction
void rkllm_pool_destroy(rkllm_memory_pool_t* pool) {
    if (!pool) {
        return;
    }
    
    pthread_mutex_lock(&pool->mutex);
    
    if (pool->allocation_count > 0) {
        printf("⚠️  RKLLM Array Pool: Destroying pool with %zu active allocations!\n", 
               pool->allocation_count);
    }
    
    free(pool->pool_memory);
    pthread_mutex_unlock(&pool->mutex);
    pthread_mutex_destroy(&pool->mutex);
    free(pool);
}

// Allocate from pool (simple linear allocator for now)
void* rkllm_pool_alloc(rkllm_memory_pool_t* pool, size_t size) {
    if (!pool || size == 0) {
        return NULL;
    }
    
    // Align to 8 bytes
    size = (size + 7) & ~7;
    
    pthread_mutex_lock(&pool->mutex);
    
    if (pool->used_size + size > pool->pool_size) {
        pthread_mutex_unlock(&pool->mutex);
        printf("⚠️  RKLLM Array Pool: Out of memory (requested: %zu, available: %zu)\n",
               size, pool->pool_size - pool->used_size);
        return NULL;
    }
    
    void* ptr = (char*)pool->pool_memory + pool->used_size;
    pool->used_size += size;
    pool->allocation_count++;
    
    pthread_mutex_unlock(&pool->mutex);
    return ptr;
}

// Free from pool (no-op for simple linear allocator)
void rkllm_pool_free(rkllm_memory_pool_t* pool, void* ptr, size_t size __attribute__((unused))) {
    if (!pool || !ptr) {
        return;
    }
    
    pthread_mutex_lock(&pool->mutex);
    pool->allocation_count--;
    pthread_mutex_unlock(&pool->mutex);
    
    // In a simple linear allocator, we don't actually free individual allocations
    // The pool is reset all at once
}

// Reset pool (free all allocations at once)
void rkllm_pool_reset(rkllm_memory_pool_t* pool) {
    if (!pool) {
        return;
    }
    
    pthread_mutex_lock(&pool->mutex);
    pool->used_size = 0;
    pool->allocation_count = 0;
    pthread_mutex_unlock(&pool->mutex);
    
    printf("🔄 RKLLM Array Pool: Reset (freed all allocations)\n");
}

// Calculate array size for multi-dimensional arrays
size_t rkllm_calculate_array_size(size_t element_size, size_t* dimensions, int num_dims) {
    size_t total_size = element_size;
    for (int i = 0; i < num_dims; i++) {
        total_size *= dimensions[i];
    }
    return total_size;
}

// Free array (checks if from pool or malloc)
void rkllm_free_array(void* array) {
    if (!array) {
        return;
    }
    
    // If we have a pool and the pointer is within pool bounds, it's from the pool
    if (g_rkllm_array_pool) {
        char* pool_start = (char*)g_rkllm_array_pool->pool_memory;
        char* pool_end = pool_start + g_rkllm_array_pool->pool_size;
        
        if ((char*)array >= pool_start && (char*)array < pool_end) {
            // From pool - no individual free needed
            return;
        }
    }
    
    // Otherwise it was malloc'd
    free(array);
}