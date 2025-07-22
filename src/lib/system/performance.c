#define _DEFAULT_SOURCE
#include "performance.h"
#include "metrics.h"
#include "../common/time_utils/time_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

static performance_system_t g_performance = {0};

static int init_buffer_pool(buffer_pool_t* pool, size_t buffer_size, size_t pool_size, const char* name) {
    pool->buffer_size = buffer_size;
    pool->pool_size = pool_size;
    pool->pool_name = name;
    
    pool->entries = calloc(pool_size, sizeof(buffer_pool_entry_t));
    if (!pool->entries) {
        return -1;
    }
    
    // Pre-allocate buffers
    for (size_t i = 0; i < pool_size; i++) {
        pool->entries[i].buffer = malloc(buffer_size);
        if (!pool->entries[i].buffer) {
            // Cleanup on failure
            for (size_t j = 0; j < i; j++) {
                free(pool->entries[j].buffer);
            }
            free(pool->entries);
            return -1;
        }
        
        pool->entries[i].size = buffer_size;
        pool->entries[i].in_use = false;
        pool->entries[i].allocated_at = 0;
        
        if (i < pool_size - 1) {
            pool->entries[i].next = &pool->entries[i + 1];
        }
    }
    
    if (pthread_mutex_init(&pool->pool_mutex, NULL) != 0) {
        // Cleanup on failure
        for (size_t i = 0; i < pool_size; i++) {
            free(pool->entries[i].buffer);
        }
        free(pool->entries);
        return -1;
    }
    
    pool->available_count = pool_size;
    pool->allocated_count = 0;
    pool->total_requests = 0;
    pool->pool_hits = 0;
    pool->pool_misses = 0;
    pool->initialized = true;
    
    return 0;
}

int performance_init(void) {
    if (g_performance.initialized) {
        return 0; // Already initialized
    }
    
    // Initialize buffer pools: 1K, 4K, 8K, 16K
    size_t sizes[] = {1024, 4096, 8192, 16384};
    size_t pool_sizes[] = {100, 50, 25, 10};
    const char* names[] = {"1K", "4K", "8K", "16K"};
    
    for (int i = 0; i < 4; i++) {
        if (init_buffer_pool(&g_performance.pools[i], sizes[i], pool_sizes[i], names[i]) != 0) {
            // Cleanup on failure
            for (int j = 0; j < i; j++) {
                buffer_pool_t* pool = &g_performance.pools[j];
                for (size_t k = 0; k < pool->pool_size; k++) {
                    free(pool->entries[k].buffer);
                }
                free(pool->entries);
                pthread_mutex_destroy(&pool->pool_mutex);
            }
            return -1;
        }
    }
    
    g_performance.start_time = get_timestamp_ms();
    g_performance.initialized = true;
    g_performance.total_allocations = 0;
    g_performance.total_frees = 0;
    g_performance.pool_allocations = 0;
    g_performance.pool_frees = 0;
    
    return 0;
}

void performance_shutdown(void) {
    if (!g_performance.initialized) {
        return;
    }
    
    // Shutdown all pools
    for (int i = 0; i < 4; i++) {
        buffer_pool_t* pool = &g_performance.pools[i];
        
        if (pool->initialized) {
            pthread_mutex_lock(&pool->pool_mutex);
            
            // Free all buffers
            for (size_t j = 0; j < pool->pool_size; j++) {
                free(pool->entries[j].buffer);
            }
            free(pool->entries);
            
            pthread_mutex_unlock(&pool->pool_mutex);
            pthread_mutex_destroy(&pool->pool_mutex);
            pool->initialized = false;
        }
    }
    
    g_performance.initialized = false;
}

bool performance_is_initialized(void) {
    return g_performance.initialized;
}

size_t performance_get_optimal_buffer_size(size_t size) {
    // Return next power of 2 that's >= size
    size_t optimal = 1024;
    while (optimal < size && optimal < 16384) {
        optimal <<= 2; // 1K -> 4K -> 16K -> 64K (but we cap at 16K for pools)
    }
    return optimal <= 16384 ? optimal : size; // If too large, return original
}

void* performance_get_buffer(size_t size) {
    if (!g_performance.initialized) {
        return malloc(size); // Fallback to regular malloc
    }
    
    g_performance.total_allocations++;
    
    // Find appropriate pool
    int pool_index = -1;
    for (int i = 0; i < 4; i++) {
        if (size <= g_performance.pools[i].buffer_size) {
            pool_index = i;
            break;
        }
    }
    
    if (pool_index == -1) {
        // Size too large for pools, use regular malloc
        metrics_counter_inc("performance_malloc_large", NULL, 0);
        return malloc(size);
    }
    
    buffer_pool_t* pool = &g_performance.pools[pool_index];
    pthread_mutex_lock(&pool->pool_mutex);
    
    pool->total_requests++;
    
    // Find available buffer
    for (size_t i = 0; i < pool->pool_size; i++) {
        if (!pool->entries[i].in_use) {
            pool->entries[i].in_use = true;
            pool->entries[i].allocated_at = get_timestamp_ms();
            pool->available_count--;
            pool->allocated_count++;
            pool->pool_hits++;
            g_performance.pool_allocations++;
            
            pthread_mutex_unlock(&pool->pool_mutex);
            
            // Update metrics
            metrics_counter_inc("performance_pool_hits", NULL, 0);
            const char* labels[][2] = {{"pool", pool->pool_name}};
            metrics_gauge_set("performance_pool_available", (double)pool->available_count, labels, 1);
            
            return pool->entries[i].buffer;
        }
    }
    
    // Pool exhausted
    pool->pool_misses++;
    pthread_mutex_unlock(&pool->pool_mutex);
    
    // Use regular malloc
    metrics_counter_inc("performance_pool_misses", NULL, 0);
    return malloc(size);
}

void performance_return_buffer(void* buffer, size_t size) {
    if (!g_performance.initialized || !buffer) {
        free(buffer);
        return;
    }
    
    g_performance.total_frees++;
    
    // Check if buffer belongs to any pool
    for (int i = 0; i < 4; i++) {
        buffer_pool_t* pool = &g_performance.pools[i];
        
        if (!pool->initialized) continue;
        
        pthread_mutex_lock(&pool->pool_mutex);
        
        for (size_t j = 0; j < pool->pool_size; j++) {
            if (pool->entries[j].buffer == buffer && pool->entries[j].in_use) {
                pool->entries[j].in_use = false;
                pool->entries[j].allocated_at = 0;
                pool->available_count++;
                pool->allocated_count--;
                g_performance.pool_frees++;
                
                pthread_mutex_unlock(&pool->pool_mutex);
                
                // Update metrics
                metrics_counter_inc("performance_pool_returns", NULL, 0);
                const char* labels[][2] = {{"pool", pool->pool_name}};
                metrics_gauge_set("performance_pool_available", (double)pool->available_count, labels, 1);
                
                return;
            }
        }
        
        pthread_mutex_unlock(&pool->pool_mutex);
    }
    
    // Not a pooled buffer, use regular free
    metrics_counter_inc("performance_free_regular", NULL, 0);
    free(buffer);
}

int performance_get_pool_stats(int pool_index, size_t* total_requests, 
                               size_t* pool_hits, size_t* pool_misses,
                               size_t* available_count, size_t* allocated_count) {
    if (!g_performance.initialized || pool_index < 0 || pool_index >= 4) {
        return -1;
    }
    
    buffer_pool_t* pool = &g_performance.pools[pool_index];
    if (!pool->initialized) {
        return -1;
    }
    
    pthread_mutex_lock(&pool->pool_mutex);
    
    if (total_requests) *total_requests = pool->total_requests;
    if (pool_hits) *pool_hits = pool->pool_hits;
    if (pool_misses) *pool_misses = pool->pool_misses;
    if (available_count) *available_count = pool->available_count;
    if (allocated_count) *allocated_count = pool->allocated_count;
    
    pthread_mutex_unlock(&pool->pool_mutex);
    
    return 0;
}

int performance_get_system_stats(uint64_t* total_allocations, uint64_t* total_frees,
                                 uint64_t* pool_allocations, uint64_t* pool_frees,
                                 uint64_t* uptime_ms) {
    if (!g_performance.initialized) {
        return -1;
    }
    
    if (total_allocations) *total_allocations = g_performance.total_allocations;
    if (total_frees) *total_frees = g_performance.total_frees;
    if (pool_allocations) *pool_allocations = g_performance.pool_allocations;
    if (pool_frees) *pool_frees = g_performance.pool_frees;
    if (uptime_ms) *uptime_ms = get_timestamp_ms() - g_performance.start_time;
    
    return 0;
}

int performance_test_buffer_pool(int operations, size_t buffer_size, double* ops_per_sec) {
    if (!g_performance.initialized || !ops_per_sec) {
        return -1;
    }
    
    uint64_t start_time = get_timestamp_ms();
    
    // Perform operations
    void** buffers = malloc(operations * sizeof(void*));
    if (!buffers) {
        return -1;
    }
    
    // Allocation phase
    for (int i = 0; i < operations; i++) {
        buffers[i] = performance_get_buffer(buffer_size);
        if (!buffers[i]) {
            // Cleanup on failure
            for (int j = 0; j < i; j++) {
                performance_return_buffer(buffers[j], buffer_size);
            }
            free(buffers);
            return -1;
        }
        
        // Write some data to ensure buffer is usable
        if (buffer_size >= sizeof(int)) {
            *((int*)buffers[i]) = i;
        }
    }
    
    // Deallocation phase
    for (int i = 0; i < operations; i++) {
        performance_return_buffer(buffers[i], buffer_size);
    }
    
    free(buffers);
    
    uint64_t end_time = get_timestamp_ms();
    double duration_sec = (end_time - start_time) / 1000.0;
    *ops_per_sec = (operations * 2) / duration_sec; // 2 operations per iteration (alloc + free)
    
    return 0;
}

int performance_test_memory_pressure(int max_memory_mb, bool* handled_gracefully) {
    if (!g_performance.initialized || !handled_gracefully) {
        return -1;
    }
    
    *handled_gracefully = true;
    size_t max_bytes = max_memory_mb * 1024 * 1024;
    size_t allocated = 0;
    void** allocations = NULL;
    int allocation_count = 0;
    
    // Try to allocate up to the limit
    while (allocated < max_bytes) {
        size_t alloc_size = 4096; // 4K chunks
        if (allocated + alloc_size > max_bytes) {
            alloc_size = max_bytes - allocated;
        }
        
        void* ptr = performance_get_buffer(alloc_size);
        if (!ptr) {
            // Memory allocation failed - this is expected under pressure
            break;
        }
        
        // Reallocate tracking array
        allocations = realloc(allocations, (allocation_count + 1) * sizeof(void*));
        if (!allocations) {
            performance_return_buffer(ptr, alloc_size);
            *handled_gracefully = false;
            return -1;
        }
        
        allocations[allocation_count++] = ptr;
        allocated += alloc_size;
        
        // Write to memory to ensure it's actually allocated
        memset(ptr, 0xAB, alloc_size);
    }
    
    // Clean up all allocations
    for (int i = 0; i < allocation_count; i++) {
        performance_return_buffer(allocations[i], 4096);
    }
    free(allocations);
    
    // System handled memory pressure if we got here without crashing
    return 0;
}

void performance_clear_stats(void) {
    if (!g_performance.initialized) {
        return;
    }
    
    g_performance.total_allocations = 0;
    g_performance.total_frees = 0;
    g_performance.pool_allocations = 0;
    g_performance.pool_frees = 0;
    
    for (int i = 0; i < 4; i++) {
        buffer_pool_t* pool = &g_performance.pools[i];
        if (pool->initialized) {
            pthread_mutex_lock(&pool->pool_mutex);
            pool->total_requests = 0;
            pool->pool_hits = 0;
            pool->pool_misses = 0;
            pthread_mutex_unlock(&pool->pool_mutex);
        }
    }
}

int performance_gc_pools(void) {
    if (!g_performance.initialized) {
        return -1;
    }
    
    int freed_entries = 0;
    uint64_t current_time = get_timestamp_ms();
    uint64_t gc_threshold = 5 * 60 * 1000; // 5 minutes
    
    // Currently, we don't actually free pool entries since they're pre-allocated
    // In a more sophisticated implementation, we could dynamically grow/shrink pools
    // For now, just return 0 to indicate no entries were freed
    
    return freed_entries;
}