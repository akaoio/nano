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

// Functions
int model_inspect_version(const char* model_path, model_version_info_t* version_info);
int model_check_compatibility(const char* model_path, compatibility_result_t* result);
int model_check_lora_compatibility(const char* base_model_path, const char* lora_adapter_path, compatibility_result_t* result);
const char* get_runtime_version_string(void);
