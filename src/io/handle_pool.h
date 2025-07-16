#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "rkllm.h"

#define MAX_HANDLES 8
#define MAX_MODEL_PATH 256

typedef struct {
    uint32_t id;
    LLMHandle handle;
    bool active;
    char model_path[MAX_MODEL_PATH];
    size_t memory_usage;
    uint64_t last_used;
} handle_slot_t;

typedef struct {
    handle_slot_t slots[MAX_HANDLES];
    uint32_t next_id;
    size_t total_memory;
} handle_pool_t;

// Initialize handle pool
int handle_pool_init(handle_pool_t* pool);

// Create new handle
uint32_t handle_pool_create(handle_pool_t* pool, const char* model_path);

// Destroy handle
int handle_pool_destroy(handle_pool_t* pool, uint32_t handle_id);

// Get handle by ID
LLMHandle* handle_pool_get(handle_pool_t* pool, uint32_t handle_id);

// Check if handle is valid
bool handle_pool_is_valid(handle_pool_t* pool, uint32_t handle_id);

// Get memory usage for handle
size_t handle_pool_get_memory_usage(handle_pool_t* pool, uint32_t handle_id);

// Get total memory usage
size_t handle_pool_get_total_memory(handle_pool_t* pool);

// Cleanup inactive handles
int handle_pool_cleanup(handle_pool_t* pool);
