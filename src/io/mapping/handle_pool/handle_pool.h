#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "../../../libs/rkllm/rkllm.h"

#include "../../../common/core.h"

// C23 compatibility
#if __STDC_VERSION__ >= 202311L
#define NODISCARD [[nodiscard]]
#define MAYBE_UNUSED [[maybe_unused]]
#else
#define NODISCARD __attribute__((warn_unused_result))
#define MAYBE_UNUSED __attribute__((unused))
#endif
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

// Global handle pool instance
extern handle_pool_t g_handle_pool;

/**
 * @brief Initialize handle pool
 * @param pool Handle pool to initialize
 * @return 0 on success, -1 on error
 */
NODISCARD int handle_pool_init(handle_pool_t* pool);

/**
 * @brief Create new handle
 * @param pool Handle pool
 * @param model_path Path to model file
 * @return Handle ID on success, 0 on error
 */
NODISCARD uint32_t handle_pool_create(handle_pool_t* pool, const char* model_path);

/**
 * @brief Destroy handle
 * @param pool Handle pool
 * @param handle_id Handle ID to destroy
 * @return 0 on success, -1 on error
 */
NODISCARD int handle_pool_destroy(handle_pool_t* pool, uint32_t handle_id);

/**
 * @brief Set handle for existing slot
 * @param pool Handle pool
 * @param handle_id Handle ID
 * @param handle Handle to set
 * @return 0 on success, -1 on error
 */
NODISCARD int handle_pool_set_handle(handle_pool_t* pool, uint32_t handle_id, LLMHandle handle);

/**
 * @brief Get handle by ID
 * @param pool Handle pool
 * @param handle_id Handle ID
 * @return Handle pointer or NULL
 */
NODISCARD LLMHandle* handle_pool_get(handle_pool_t* pool, uint32_t handle_id);

/**
 * @brief Check if handle is valid
 * @param pool Handle pool
 * @param handle_id Handle ID
 * @return true if valid, false otherwise
 */
NODISCARD bool handle_pool_is_valid(handle_pool_t* pool, uint32_t handle_id);

/**
 * @brief Get memory usage for handle
 * @param pool Handle pool
 * @param handle_id Handle ID
 * @return Memory usage in bytes
 */
NODISCARD size_t handle_pool_get_memory_usage(handle_pool_t* pool, uint32_t handle_id);

/**
 * @brief Get total memory usage
 * @param pool Handle pool
 * @return Total memory usage in bytes
 */
NODISCARD size_t handle_pool_get_total_memory(handle_pool_t* pool);

/**
 * @brief Cleanup inactive handles
 * @param pool Handle pool
 * @return 0 on success, -1 on error
 */
NODISCARD int handle_pool_cleanup(handle_pool_t* pool);

// Global handle pool instance
extern handle_pool_t g_handle_pool;
