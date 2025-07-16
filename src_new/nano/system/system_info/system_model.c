#include "system_info.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

int model_analyze(const char* model_path, const system_info_t* sys_info, model_info_t* model_info) {
    if (!model_path || !sys_info || !model_info) return -1;
    
    // Get model file size
    struct stat st;
    if (stat(model_path, &st) != 0) {
        printf("❌ Model file not found: %s\n", model_path);
        model_info->model_size_mb = 0;
        model_info->memory_required_mb = 0;
        model_info->npu_cores_needed = 0;
        model_info->can_load = 0;
        return -1;
    }
    
    model_info->model_size_mb = st.st_size / (1024 * 1024);
    
    // More intelligent memory estimation based on model size
    if (model_info->model_size_mb > 6000) {
        // Large models (>6GB) need 30% overhead + 1GB buffer
        model_info->memory_required_mb = model_info->model_size_mb * 1.3 + 1024;
        model_info->npu_cores_needed = 3;
    } else if (model_info->model_size_mb > 3000) {
        // Medium models (3-6GB) need 25% overhead + 512MB buffer
        model_info->memory_required_mb = model_info->model_size_mb * 1.25 + 512;
        model_info->npu_cores_needed = 2;
    } else {
        // Small models (<3GB) need 20% overhead + 256MB buffer
        model_info->memory_required_mb = model_info->model_size_mb * 1.2 + 256;
        model_info->npu_cores_needed = 1;
    }
    
    // Check if model can be loaded (need enough RAM but allow tight memory usage)
    model_info->can_load = (model_info->memory_required_mb <= sys_info->available_ram_mb);
    
    printf("📊 Model %s: %luMB size, %luMB memory needed, %u NPU cores, can_load=%s\n",
           model_path, model_info->model_size_mb, model_info->memory_required_mb,
           model_info->npu_cores_needed, model_info->can_load ? "YES" : "NO");
    
    return 0;
}

int system_can_load_model(const system_info_t* sys_info, const model_info_t* model_info) {
    if (!sys_info || !model_info) return 0;
    
    // More lenient checking - focus on available RAM with safety margin
    uint64_t required_with_buffer = model_info->memory_required_mb + 1024; // +1GB safety buffer
    
    printf("🔍 Resource check: need %luMB + 1GB buffer = %luMB, available %luMB\n",
           model_info->memory_required_mb, required_with_buffer, sys_info->available_ram_mb);
    
    // Check RAM availability with buffer
    if (required_with_buffer > sys_info->available_ram_mb) {
        printf("❌ Not enough RAM: need %luMB, have %luMB\n", 
               required_with_buffer, sys_info->available_ram_mb);
        return 0;
    }
    
    // Check NPU cores (always pass for RK3588)
    if (model_info->npu_cores_needed > sys_info->npu_cores) {
        printf("❌ Not enough NPU cores: need %u, have %u\n", 
               model_info->npu_cores_needed, sys_info->npu_cores);
        return 0;
    }
    
    printf("✅ Resource check passed\n");
    return 1;
}
