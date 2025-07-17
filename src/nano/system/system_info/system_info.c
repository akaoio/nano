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
    
    // RK3588 NPU detection - be more conservative with NPU memory
    info->npu_cores = 3; // RK3588 has 3 NPU cores
    info->npu_memory_mb = 8192; // Increase to 8GB for large models
    
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
