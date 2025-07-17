#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

// Model validation and compatibility checks
// Pre-load validation for models and system resources

typedef struct {
    char path[512];
    size_t file_size;
    uint32_t model_type;
    uint32_t required_memory_mb;
    uint32_t required_npu_cores;
    bool is_lora;
    char base_model_path[512];  // For LoRA models
} model_info_t;




/**
 * @brief Check if LoRA is compatible with base model
 * @param lora_path Path to LoRA file
 * @param base_model_path Path to base model file
 * @return 0 if compatible, -1 if not
 */
int model_check_lora_compatibility(const char* lora_path, const char* base_model_path);



