#pragma once

#include <stdint.h>
#include <stdbool.h>

// Model version information
typedef struct {
    uint32_t major;
    uint32_t minor;
    uint32_t patch;
    char version_string[64];
    char toolkit_version[64];
    char target_platform[32];
    char model_dtype[16];
    uint32_t max_context_limit;
    uint32_t npu_core_num;
    bool is_lora_adapter;
    char base_model_hash[64];  // For LoRA compatibility check
} model_version_info_t;

// Model compatibility check result
typedef struct {
    bool is_compatible;
    char error_message[256];
    model_version_info_t model_info;
    model_version_info_t runtime_info;
} compatibility_result_t;

/**
 * @brief Extract model version from file
 * @param model_path Path to model file
 * @param version Version info to populate
 * @return 0 on success, -1 on error
 */
int model_check_version(const char* model_path, model_version_info_t* version);

/**
 * @brief Check model compatibility with runtime
 * @param model_path Path to model file
 * @param result Compatibility result
 * @return 0 on success, -1 on error
 */
int model_check_compatibility(const char* model_path, compatibility_result_t* result);

/**
 * @brief Check LoRA compatibility with base model
 * @param base_model_path Path to base model
 * @param lora_path Path to LoRA adapter
 * @param result Compatibility result
 * @return 0 on success, -1 on error
 */
int model_check_lora_compatibility(const char* base_model_path, const char* lora_path, compatibility_result_t* result);

/**
 * @brief Get runtime version string
 * @return Runtime version string
 */
const char* model_get_runtime_version(void);

/**
 * @brief Initialize model checker
 * @return 0 on success, -1 on error
 */
int model_checker_init(void);

/**
 * @brief Shutdown model checker
 */
void model_checker_shutdown(void);
