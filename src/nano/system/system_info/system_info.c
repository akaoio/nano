#include "system_info.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/sysinfo.h>
#include <unistd.h>

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
    
    // RK3588 NPU detection - try to get actual NPU memory from system
    info->npu_cores = 3; // RK3588 has 3 NPU cores
    
    // Try to detect actual NPU memory, fall back to 8GB if detection fails
    // Check if we can read from NPU device info
    FILE* npu_info = fopen("/sys/class/devfreq/fdab0000.npu/available_frequencies", "r");
    if (npu_info) {
        fclose(npu_info);
        // NPU is available, but we need to determine memory size
        // For RK3588, standard configurations are 8GB or 16GB NPU memory
        // Try to infer from total system RAM
        if (info->total_ram_mb >= 30000) {
            // Systems with 32GB+ RAM often have larger NPU memory
            info->npu_memory_mb = 16384; // Try 16GB for high-end systems
        } else {
            info->npu_memory_mb = 8192; // Standard 8GB
        }
    } else {
        info->npu_memory_mb = 8192; // Fallback to 8GB
    }
    
    printf("🔍 System: %u CPU cores, %u NPU cores, %luMB NPU memory\n", 
           info->cpu_cores, info->npu_cores, info->npu_memory_mb);
    
    return 0;
}

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
