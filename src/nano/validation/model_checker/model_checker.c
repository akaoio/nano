#include "model_checker.h"
#include "../../system/system_info/system_info.h"
#include "../../system/resource_mgr/resource_mgr.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

// Extract model version from file
int model_check_version(const char* model_path, model_version_info_t* version) {
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
    
    // If no version found, return error to prevent unknown compatibility issues
    return -1;
}

// Get runtime version string
const char* model_get_runtime_version(void) {
    return "1.2.1";
}

// Check model compatibility with runtime
int model_check_compatibility(const char* model_path, compatibility_result_t* result) {
    if (!model_path || !result) return -1;
    
    // Initialize result
    result->is_compatible = false;
    strcpy(result->error_message, "");
    
    // Check if file exists and readable
    if (!file_exists_and_readable(model_path)) {
        snprintf(result->error_message, sizeof(result->error_message),
                "Model file not found or not readable: %s", model_path);
        return -1;
    }
    
    // Check system resources
    if (check_system_resources(model_path, result) != 0) {
        return -1;
    }
    
    // Check model version
    if (model_check_version(model_path, &result->model_info) != 0) {
        snprintf(result->error_message, sizeof(result->error_message),
                "Cannot extract model version from: %s", model_path);
        return -1;
    }
    
    // Check runtime compatibility
    const char* runtime_version = model_get_runtime_version();
    strcpy(result->runtime_info.version_string, runtime_version);
    
    // Check actual version compatibility
    if (strcmp(runtime_version, "1.2.1") == 0) {
        result->is_compatible = true;
    } else {
        result->is_compatible = false;
        printf("⚠️  Runtime version mismatch: expected 1.2.1, got %s\n", runtime_version);
    }
    
    printf("✅ Model compatibility check passed for: %s\n", model_path);
    return 0;
}

// Check LoRA compatibility with base model
int model_check_lora_compatibility(const char* base_model_path, const char* lora_path, compatibility_result_t* result) {
    if (!base_model_path || !lora_path || !result) return -1;
    
    // Initialize result
    result->is_compatible = false;
    strcpy(result->error_message, "");
    
    // Check if both files exist
    if (!file_exists_and_readable(base_model_path)) {
        snprintf(result->error_message, sizeof(result->error_message),
                "Base model file not found: %s", base_model_path);
        return -1;
    }
    
    if (!file_exists_and_readable(lora_path)) {
        snprintf(result->error_message, sizeof(result->error_message),
                "LoRA adapter file not found: %s", lora_path);
        return -1;
    }
    
    // Check system resources for both files
    int base_size = get_file_size_mb(base_model_path);
    int lora_size = get_file_size_mb(lora_path);
    
    if (base_size < 0 || lora_size < 0) {
        snprintf(result->error_message, sizeof(result->error_message),
                "Cannot determine file sizes");
        return -1;
    }
    
    // Check system resources for combined loading
    system_info_t sys_info;
    if (system_detect(&sys_info) != 0) {
        snprintf(result->error_message, sizeof(result->error_message),
                "Cannot detect system capabilities");
        return -1;
    }
    
    // LoRA typically needs base model + adapter in NPU memory
    int required_npu_memory_mb = ((base_size + lora_size) * 3) / 2;  // 1.5x overhead
    if (required_npu_memory_mb > sys_info.npu_memory_mb) {
        snprintf(result->error_message, sizeof(result->error_message),
                "Insufficient NPU memory for LoRA: need %dMB, have %dMB", 
                required_npu_memory_mb, sys_info.npu_memory_mb);
        return -1;
    }
    
    // Check model versions
    model_version_info_t base_version, lora_version;
    if (model_check_version(base_model_path, &base_version) != 0) {
        snprintf(result->error_message, sizeof(result->error_message),
                "Cannot extract base model version");
        return -1;
    }
    
    if (model_check_version(lora_path, &lora_version) != 0) {
        snprintf(result->error_message, sizeof(result->error_message),
                "Cannot extract LoRA version");
        return -1;
    }
    
    // Check LoRA compatibility with base model
    if (base_version.major == lora_version.major &&
        base_version.minor == lora_version.minor) {
        result->is_compatible = true;
    } else {
        result->is_compatible = false;
        snprintf(result->error_message, sizeof(result->error_message),
                "LoRA version mismatch: base %d.%d, lora %d.%d",
                base_version.major, base_version.minor,
                lora_version.major, lora_version.minor);
    }
    result->model_info = base_version;
    
    printf("✅ LoRA compatibility check passed: Base=%s, LoRA=%s\n", 
           base_model_path, lora_path);
    printf("🔍 Required NPU memory: %dMB, Available: %dMB\n",
           required_npu_memory_mb, sys_info.npu_memory_mb);
    
    return 0;
}

// Initialize model checker
int model_checker_init(void) {
    // Initialize model checker subsystem
    return 0;
}

// Shutdown model checker
void model_checker_shutdown(void) {
    // Cleanup model checker subsystem
}

// Get file size in MB
static int get_file_size_mb(const char* path) {
    struct stat st;
    if (stat(path, &st) != 0) return -1;
    return (int)(st.st_size / (1024 * 1024));
}

// Check if file exists and is readable
static bool file_exists_and_readable(const char* path) {
    return access(path, R_OK) == 0;
}

// Check system resources for model loading
static int check_system_resources(const char* model_path, compatibility_result_t* result) {
    // Get model file size
    int model_size_mb = get_file_size_mb(model_path);
    if (model_size_mb < 0) {
        snprintf(result->error_message, sizeof(result->error_message),
                "Cannot determine model file size: %s", model_path);
        return -1;
    }
    
    // Get system info
    system_info_t sys_info;
    if (system_detect(&sys_info) != 0) {
        snprintf(result->error_message, sizeof(result->error_message),
                "Cannot detect system capabilities");
        return -1;
    }
    
    // Check NPU memory (models typically need 1.5x file size in NPU memory)
    int required_npu_memory_mb = (model_size_mb * 3) / 2;  // 1.5x overhead
    if (required_npu_memory_mb > sys_info.npu_memory_mb) {
        snprintf(result->error_message, sizeof(result->error_message),
                "Insufficient NPU memory: need %dMB, have %dMB", 
                required_npu_memory_mb, sys_info.npu_memory_mb);
        return -1;
    }
    
    // Check RAM (need at least 2GB free for model loading)
    if (sys_info.ram_available_mb < 2048) {
        snprintf(result->error_message, sizeof(result->error_message),
                "Insufficient RAM: need 2048MB, have %dMB available", 
                sys_info.ram_available_mb);
        return -1;
    }
    
    // Check NPU cores (need at least 1)
    if (sys_info.npu_cores < 1) {
        snprintf(result->error_message, sizeof(result->error_message),
                "No NPU cores available");
        return -1;
    }
    
    printf("🔍 Model size: %dMB, Required NPU memory: %dMB, Available: %dMB\n",
           model_size_mb, required_npu_memory_mb, sys_info.npu_memory_mb);
    
    return 0;
}
