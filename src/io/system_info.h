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

// System detection
int system_detect(system_info_t* info);
int model_analyze(const char* model_path, const system_info_t* sys_info, model_info_t* model_info);
int system_can_load_model(const system_info_t* sys_info, const model_info_t* model_info);

// Memory cleanup functions
int system_force_gc(void);
int system_free_memory(void);
int system_refresh_memory_info(system_info_t* info);
