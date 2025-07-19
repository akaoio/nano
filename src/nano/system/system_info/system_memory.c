#define _GNU_SOURCE
#include "system_info.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int system_force_gc(void) {
    printf("🧹 Running system memory cleanup...\n");
    
    // Force kernel to drop caches
    system("sync");
    system("echo 1 > /proc/sys/vm/drop_caches 2>/dev/null || true");
    system("echo 2 > /proc/sys/vm/drop_caches 2>/dev/null || true");
    system("echo 3 > /proc/sys/vm/drop_caches 2>/dev/null || true");
    
    // Wait a bit for cleanup
    usleep(500000); // 0.5 seconds
    
    return 0;
}

int system_free_memory(void) {
    
    // Trigger memory compaction
    system("echo 1 > /proc/sys/vm/compact_memory 2>/dev/null || true");
    
    // Force garbage collection
    system("echo 1 > /proc/sys/vm/vfs_cache_pressure 2>/dev/null || true");
    
    usleep(200000); // 0.2 seconds
    return 0;
}
