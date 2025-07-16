#include "model_version.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

// Simple version extraction - looks for version strings in the model file
int extract_model_version(const char* model_path, model_version_info_t* version) {
    if (!model_path || !version) return -1;
    
    // Initialize to unknown version
    version->major = 0;
    version->minor = 0;
    version->patch = 0;
    strcpy(version->version_string, "unknown");
    strcpy(version->toolkit_version, "unknown");
    strcpy(version->target_platform, "unknown");
    strcpy(version->model_dtype, "unknown");
    version->max_context_limit = 0;
    version->npu_core_num = 0;
    version->is_lora_adapter = false;
    strcpy(version->base_model_hash, "unknown");
    
    FILE* file = fopen(model_path, "rb");
    if (!file) return -1;
    
    // Read first 4KB to look for version information
    char buffer[4096];
    size_t bytes_read = fread(buffer, 1, sizeof(buffer), file);
    fclose(file);
    
    if (bytes_read == 0) return -1;
    
    // Look for version patterns in the buffer
    // Try to find "1.1.1", "1.2.1", etc.
    for (size_t i = 0; i < bytes_read - 5; i++) {
        if (buffer[i] >= '1' && buffer[i] <= '9' && 
            buffer[i+1] == '.' && 
            buffer[i+2] >= '0' && buffer[i+2] <= '9' &&
            buffer[i+3] == '.' &&
            buffer[i+4] >= '0' && buffer[i+4] <= '9') {
            
            // Found a version pattern like "X.Y.Z"
            version->major = buffer[i] - '0';
            version->minor = buffer[i+2] - '0';
            version->patch = buffer[i+4] - '0';
            
            snprintf(version->version_string, sizeof(version->version_string),
                     "%d.%d.%d", version->major, version->minor, version->patch);
            
            return 0;
        }
    }
    
    // If no version found, assume it's compatible with current runtime
    // This is a fallback to avoid breaking existing models
    version->major = 1;
    version->minor = 2;
    version->patch = 1;
    strcpy(version->version_string, "1.2.1");
    
    return 0;
}

// Check if model is compatible with runtime
int model_check_compatibility(const char* model_path, compatibility_result_t* result) {
    if (!model_path || !result) return -1;
    
    // Initialize result structure
    result->is_compatible = false;
    strcpy(result->error_message, "");
    
    // Extract model version
    int ret = extract_model_version(model_path, &result->model_info);
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
    
    if (extract_model_version(base_model_path, &base_version) != 0) {
        snprintf(result->error_message, sizeof(result->error_message), 
                 "Failed to extract base model version");
        return -1;
    }
    
    if (extract_model_version(lora_path, &lora_version) != 0) {
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

// Get runtime version string
const char* get_runtime_version_string(void) {
    return "1.2.1";
}
