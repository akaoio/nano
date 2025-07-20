#pragma once

#include <stdint.h>
#include <stddef.h>

// System resource information
typedef struct {
    uint64_t total_ram_mb;
    uint64_t available_ram_mb;
    uint32_t cpu_cores;
    uint32_t npu_cores;
    uint64_t npu_memory_mb;
} system_info_t;

// Model resource requirements
typedef struct {
    uint64_t model_size_mb;
    uint64_t memory_required_mb;
    uint32_t npu_cores_needed;
    int can_load;
} model_info_t;

/**
 * @brief Detect system resources
 * @param info System info struct to fill
 * @return 0 on success, -1 on error
 */
int system_detect(system_info_t* info);

/**
 * @brief Analyze model requirements  
 * @param model_path Path to model file
 * @param sys_info System info
 * @param model_info Model info to fill
 * @return 0 on success, -1 on error
 */
int model_analyze(const char* model_path, const system_info_t* sys_info, model_info_t* model_info);

/**
 * @brief Check if system can load model
 * @param sys_info System info
 * @param model_info Model info
 * @return 1 if can load, 0 otherwise
 */
int system_can_load_model(const system_info_t* sys_info, const model_info_t* model_info);

/**
 * @brief Force garbage collection and memory cleanup
 * @return 0 on success, -1 on error
 */
int system_force_gc(void);

/**
 * @brief Free unused memory
 * @return 0 on success, -1 on error
 */
int system_free_memory(void);

/**
 * @brief Refresh memory info after cleanup
 * @param info System info to refresh
 * @return 0 on success, -1 on error
 */
int system_refresh_memory_info(system_info_t* info);
