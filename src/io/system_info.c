#include "system_info.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/sysinfo.h>
#include <unistd.h>

// Detect system resources
int system_detect(system_info_t* info) {
    if (!info) return -1;
    
    // Get RAM info
    struct sysinfo si;
    if (sysinfo(&si) == 0) {
        info->total_ram_mb = si.totalram * si.mem_unit / (1024 * 1024);
        info->available_ram_mb = si.freeram * si.mem_unit / (1024 * 1024);
        printf("🔍 System RAM: %luMB total, %luMB available\n", info->total_ram_mb, info->available_ram_mb);
    } else {
        info->total_ram_mb = 32768; // Default: 32GB
        info->available_ram_mb = 16384; // Default: 16GB available
        printf("⚠️  Using default RAM values: %luMB total, %luMB available\n", info->total_ram_mb, info->available_ram_mb);
    }
    
    // Get CPU cores
    info->cpu_cores = sysconf(_SC_NPROCESSORS_ONLN);
    if (info->cpu_cores == 0) info->cpu_cores = 8; // Default for RK3588
    
    // RK3588 NPU detection - be more conservative with NPU memory
    info->npu_cores = 3; // RK3588 has 3 NPU cores
    info->npu_memory_mb = 8192; // Increase to 8GB for large models
    
    printf("🔍 System: %u CPU cores, %u NPU cores, %luMB NPU memory\n", 
           info->cpu_cores, info->npu_cores, info->npu_memory_mb);
    
    return 0;
}

// Analyze model requirements
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

// Check if system can load model
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

// Force garbage collection and memory cleanup
int system_force_gc(void) {
    printf("🧹 Running system memory cleanup...\n");
    
    // Force kernel to drop caches
    system("sync");
    system("echo 1 > /proc/sys/vm/drop_caches 2>/dev/null || true");
    system("echo 2 > /proc/sys/vm/drop_caches 2>/dev/null || true");
    system("echo 3 > /proc/sys/vm/drop_caches 2>/dev/null || true");
    
    // Wait a bit for cleanup
    usleep(500000); // 0.5 seconds
    
    printf("✅ Memory cleanup completed\n");
    return 0;
}

// Free unused memory
int system_free_memory(void) {
    printf("🧹 Freeing unused memory...\n");
    
    // Trigger memory compaction
    system("echo 1 > /proc/sys/vm/compact_memory 2>/dev/null || true");
    
    // Force garbage collection
    system("echo 1 > /proc/sys/vm/vfs_cache_pressure 2>/dev/null || true");
    
    usleep(200000); // 0.2 seconds
    return 0;
}

// Refresh memory info after cleanup
int system_refresh_memory_info(system_info_t* info) {
    if (!info) return -1;
    
    // Force a fresh memory read
    struct sysinfo si;
    if (sysinfo(&si) == 0) {
        info->available_ram_mb = si.freeram * si.mem_unit / (1024 * 1024);
        printf("🔄 Memory refreshed: %luMB available\n", info->available_ram_mb);
        return 0;
    }
    
    return -1;
}
