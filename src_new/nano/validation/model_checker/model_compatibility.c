#include "model_checker.h"
#include <stdio.h>
#include <string.h>

// Check model compatibility with runtime
int model_check_compatibility(const char* model_path, compatibility_result_t* result) {
    if (!model_path || !result) return -1;
    
    // Initialize result structure
    result->is_compatible = false;
    strcpy(result->error_message, "");
    
    // Extract model version
    int ret = model_check_version(model_path, &result->model_info);
    if (ret != 0) {
        snprintf(result->error_message, sizeof(result->error_message), 
                 "Failed to extract version from model file");
        return -1;
    }
    
    // Set runtime version info
    result->runtime_info.major = 1;
    result->runtime_info.minor = 2;
    result->runtime_info.patch = 1;
    strcpy(result->runtime_info.version_string, "1.2.1");
    
    // Check compatibility
    if (result->model_info.major != result->runtime_info.major) {
        snprintf(result->error_message, sizeof(result->error_message), 
                 "Major version mismatch: model=%s, runtime=%s", 
                 result->model_info.version_string, result->runtime_info.version_string);
        return -1;
    }
    
    // Check for known problematic combinations
    if (result->model_info.major == 1 && result->model_info.minor == 1 && 
        result->runtime_info.major == 1 && result->runtime_info.minor == 2) {
        snprintf(result->error_message, sizeof(result->error_message), 
                 "Known incompatibility: model v%s not compatible with runtime v%s", 
                 result->model_info.version_string, result->runtime_info.version_string);
        return -1;
    }
    
    result->is_compatible = true;
    return 0;
}

// Check LoRA compatibility with base model
int model_check_lora_compatibility(const char* base_model_path, const char* lora_path, compatibility_result_t* result) {
    if (!base_model_path || !lora_path || !result) return -1;
    
    // Initialize result structure
    result->is_compatible = false;
    strcpy(result->error_message, "");
    
    model_version_info_t base_version, lora_version;
    
    if (model_check_version(base_model_path, &base_version) != 0) {
        snprintf(result->error_message, sizeof(result->error_message), 
                 "Failed to extract base model version");
        return -1;
    }
    
    if (model_check_version(lora_path, &lora_version) != 0) {
        snprintf(result->error_message, sizeof(result->error_message), 
                 "Failed to extract LoRA adapter version");
        return -1;
    }
    
    // Copy version info to result
    result->model_info = base_version;
    result->runtime_info = lora_version;
    
    // LoRA and base model must have exact version match
    if (base_version.major != lora_version.major ||
        base_version.minor != lora_version.minor ||
        base_version.patch != lora_version.patch) {
        
        snprintf(result->error_message, sizeof(result->error_message), 
                 "LoRA version mismatch: base=%s, adapter=%s", 
                 base_version.version_string, lora_version.version_string);
        return -1;
    }
    
    result->is_compatible = true;
    return 0;
}
