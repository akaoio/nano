#include "model_checker.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

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
    
    // If no version found, assume it's compatible with current runtime
    // This is a fallback to avoid breaking existing models
    version->major = 1;
    version->minor = 2;
    version->patch = 1;
    strcpy(version->version_string, "1.2.1");
    
    return 0;
}

// Get runtime version string
const char* model_get_runtime_version(void) {
    return "1.2.1";
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
