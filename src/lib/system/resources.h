#pragma once

#include "info.h"
#include <stdint.h>
#include <stdbool.h>

#define MAX_MODELS 3

typedef struct {
    uint32_t handle_id;
    bool active;
    model_info_t model_info;
    uint64_t last_used;
} model_resource_t;

typedef struct {
    system_info_t system_info;
    model_resource_t models[MAX_MODELS];
    uint32_t model_count;
    uint64_t total_memory_used;
} resource_mgr_t;

/**
 * @brief Initialize resource manager
 * @param mgr Resource manager to initialize
 * @return 0 on success, -1 on error
 */
int resource_mgr_init(resource_mgr_t* mgr);

/**
 * @brief Check if model can be loaded
 * @param mgr Resource manager
 * @param model_path Path to model file
 * @return 1 if can load, 0 otherwise
 */
int resource_mgr_can_load_model(resource_mgr_t* mgr, const char* model_path);

/**
 * @brief Reserve resources for model
 * @param mgr Resource manager
 * @param handle_id Handle ID
 * @param model_path Path to model file
 * @return 0 on success, -1 on error
 */
int resource_mgr_reserve_model(resource_mgr_t* mgr, uint32_t handle_id, const char* model_path);

/**
 * @brief Release resources for model
 * @param mgr Resource manager
 * @param handle_id Handle ID
 * @return 0 on success, -1 on error
 */
int resource_mgr_release_model(resource_mgr_t* mgr, uint32_t handle_id);

/**
 * @brief Get current memory usage
 * @param mgr Resource manager
 * @return Memory usage in MB
 */
uint64_t resource_mgr_get_memory_usage(resource_mgr_t* mgr);

/**
 * @brief Cleanup unused resources
 * @param mgr Resource manager
 * @return 0 on success, -1 on error
 */
int resource_mgr_cleanup(resource_mgr_t* mgr);
