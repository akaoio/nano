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
 * @brief Validate model file exists and is accessible
 * @param model_path Path to model file
 * @return 0 on success, -1 on error
 */
int model_validate_file(const char* model_path);

/**
 * @brief Get model information without loading
 * @param model_path Path to model file
 * @param info Output model information
 * @return 0 on success, -1 on error
 */
int model_get_info(const char* model_path, model_info_t* info);

/**
 * @brief Check if system has enough resources for model
 * @param info Model information
 * @return 0 if compatible, -1 if not
 */
int model_check_system_compatibility(const model_info_t* info);

/**
 * @brief Check if LoRA is compatible with base model
 * @param lora_path Path to LoRA file
 * @param base_model_path Path to base model file
 * @return 0 if compatible, -1 if not
 */
int model_check_lora_compatibility(const char* lora_path, const char* base_model_path);

/**
 * @brief Estimate memory usage for model
 * @param model_path Path to model file
 * @return Memory usage in MB, 0 on error
 */
uint32_t model_estimate_memory(const char* model_path);

/**
 * @brief Check if NPU memory is available
 * @param required_mb Required memory in MB
 * @return 0 if available, -1 if not
 */
int model_check_npu_memory(uint32_t required_mb);

/**
 * @brief Pre-validate model before loading
 * @param model_path Path to model file
 * @param lora_path Path to LoRA file (optional)
 * @return 0 if ready to load, -1 on error
 */
int model_pre_validate(const char* model_path, const char* lora_path);
