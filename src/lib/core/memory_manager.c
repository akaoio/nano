#define _POSIX_C_SOURCE 199309L
#include "memory_manager.h"
#include <json-c/json.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>

// Global memory manager instance
static memory_manager_t g_memory_manager = {0};

// Helper functions
static uint64_t get_timestamp_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)(ts.tv_sec * 1000000000ULL + ts.tv_nsec);
}

static uint32_t get_thread_id(void) {
    return (uint32_t)pthread_self();
}

static size_t align_size(size_t size, size_t alignment) {
    if (alignment == 0) alignment = sizeof(void*);
    return (size + alignment - 1) & ~(alignment - 1);
}

// Memory type name conversion
const char* memory_type_to_string(memory_allocation_type_t type) {
    switch (type) {
        case MEMORY_TYPE_RKLLM_ARRAYS: return "rkllm_arrays";
        case MEMORY_TYPE_STREAMING_BUFFERS: return "streaming_buffers";
        case MEMORY_TYPE_TRANSPORT_BUFFERS: return "transport_buffers";
        case MEMORY_TYPE_JSON_OBJECTS: return "json_objects";
        case MEMORY_TYPE_STRING_BUFFERS: return "string_buffers";
        case MEMORY_TYPE_SESSION_DATA: return "session_data";
        case MEMORY_TYPE_TEMP_STORAGE: return "temp_storage";
        case MEMORY_TYPE_SYSTEM_BUFFERS: return "system_buffers";
        default: return "unknown";
    }
}

memory_allocation_type_t memory_type_from_string(const char* type_str) {
    if (!type_str) return MEMORY_TYPE_UNKNOWN;
    
    if (strcmp(type_str, "rkllm_arrays") == 0) return MEMORY_TYPE_RKLLM_ARRAYS;
    if (strcmp(type_str, "streaming_buffers") == 0) return MEMORY_TYPE_STREAMING_BUFFERS;
    if (strcmp(type_str, "transport_buffers") == 0) return MEMORY_TYPE_TRANSPORT_BUFFERS;
    if (strcmp(type_str, "json_objects") == 0) return MEMORY_TYPE_JSON_OBJECTS;
    if (strcmp(type_str, "string_buffers") == 0) return MEMORY_TYPE_STRING_BUFFERS;
    if (strcmp(type_str, "session_data") == 0) return MEMORY_TYPE_SESSION_DATA;
    if (strcmp(type_str, "temp_storage") == 0) return MEMORY_TYPE_TEMP_STORAGE;
    if (strcmp(type_str, "system_buffers") == 0) return MEMORY_TYPE_SYSTEM_BUFFERS;
    
    return MEMORY_TYPE_UNKNOWN;
}

// Allocation tracking functions
static int track_allocation(void* ptr, size_t size, memory_allocation_type_t type,
                           const char* file, int line, const char* function) {
    if (!g_memory_manager.leak_detection_enabled || !ptr) return 0;
    
    pthread_mutex_lock(&g_memory_manager.allocation_mutex);
    
    // Find free slot in allocation table
    for (size_t i = 0; i < g_memory_manager.allocation_table_size; i++) {
        if (!g_memory_manager.allocation_table[i].active) {
            memory_allocation_entry_t* entry = &g_memory_manager.allocation_table[i];
            
            entry->ptr = ptr;
            entry->size = size;
            entry->type = type;
            entry->timestamp = get_timestamp_ns();
            entry->file = file;
            entry->line = line;
            entry->function = function;
            entry->thread_id = get_thread_id();
            entry->active = true;
            
            // Simple stack trace (just store the current function for now)
            entry->stack_trace[0] = (void*)function;
            entry->stack_depth = 1;
            
            g_memory_manager.active_allocations++;
            pthread_mutex_unlock(&g_memory_manager.allocation_mutex);
            return 0;
        }
    }
    
    pthread_mutex_unlock(&g_memory_manager.allocation_mutex);
    return -1; // Allocation table full
}

static int untrack_allocation(void* ptr) {
    if (!g_memory_manager.leak_detection_enabled || !ptr) return 0;
    
    pthread_mutex_lock(&g_memory_manager.allocation_mutex);
    
    // Find allocation entry
    for (size_t i = 0; i < g_memory_manager.allocation_table_size; i++) {
        if (g_memory_manager.allocation_table[i].active && 
            g_memory_manager.allocation_table[i].ptr == ptr) {
            
            g_memory_manager.allocation_table[i].active = false;
            g_memory_manager.active_allocations--;
            pthread_mutex_unlock(&g_memory_manager.allocation_mutex);
            return 0;
        }
    }
    
    pthread_mutex_unlock(&g_memory_manager.allocation_mutex);
    return -1; // Allocation not found
}

// Memory manager initialization
int memory_manager_init(bool enable_leak_detection, bool enable_statistics) {
    if (g_memory_manager.initialized) {
        return 0; // Already initialized
    }
    
    memset(&g_memory_manager, 0, sizeof(memory_manager_t));
    
    g_memory_manager.leak_detection_enabled = enable_leak_detection;
    g_memory_manager.statistics_enabled = enable_statistics;
    g_memory_manager.manager_start_time = get_timestamp_ns();
    
    // Initialize allocation tracking
    if (enable_leak_detection) {
        g_memory_manager.allocation_table_size = MEMORY_TRACK_MAX_ENTRIES;
        g_memory_manager.allocation_table = calloc(MEMORY_TRACK_MAX_ENTRIES, 
                                                  sizeof(memory_allocation_entry_t));
        if (!g_memory_manager.allocation_table) {
            return -1;
        }
        
        if (pthread_mutex_init(&g_memory_manager.allocation_mutex, NULL) != 0) {
            free(g_memory_manager.allocation_table);
            return -1;
        }
    }
    
    // Initialize pool manager
    if (pthread_mutex_init(&g_memory_manager.pool_manager_mutex, NULL) != 0) {
        if (enable_leak_detection) {
            pthread_mutex_destroy(&g_memory_manager.allocation_mutex);
            free(g_memory_manager.allocation_table);
        }
        return -1;
    }
    
    // Set default memory pressure threshold (256MB)
    g_memory_manager.memory_pressure_threshold = 256 * 1024 * 1024;
    
    g_memory_manager.initialized = true;
    
    printf("✅ Memory management system initialized (leak_detection=%s, statistics=%s)\n",
           enable_leak_detection ? "enabled" : "disabled",
           enable_statistics ? "enabled" : "disabled");
    
    return 0;
}

int memory_manager_shutdown(void) {
    if (!g_memory_manager.initialized) {
        return 0;
    }
    
    int leaked_allocations = 0;
    
    // Check for leaks before shutdown
    if (g_memory_manager.leak_detection_enabled) {
        char* leak_report = NULL;
        leaked_allocations = memory_manager_check_leaks(&leak_report);
        
        if (leaked_allocations > 0) {
            printf("⚠️  Memory leaks detected during shutdown: %d allocations\n", leaked_allocations);
            if (leak_report) {
                printf("Leak report:\n%s\n", leak_report);
                free(leak_report);
            }
        }
    }
    
    // Destroy all pools
    pthread_mutex_lock(&g_memory_manager.pool_manager_mutex);
    for (int i = 0; i < g_memory_manager.pool_count; i++) {
        if (g_memory_manager.pools[i].active) {
            memory_manager_destroy_pool(&g_memory_manager.pools[i]);
        }
    }
    pthread_mutex_unlock(&g_memory_manager.pool_manager_mutex);
    
    // Cleanup allocation tracking
    if (g_memory_manager.leak_detection_enabled) {
        pthread_mutex_destroy(&g_memory_manager.allocation_mutex);
        free(g_memory_manager.allocation_table);
    }
    
    pthread_mutex_destroy(&g_memory_manager.pool_manager_mutex);
    
    printf("🧹 Memory management system shutdown (leaked allocations: %d)\n", leaked_allocations);
    g_memory_manager.initialized = false;
    
    return leaked_allocations;
}

// Pool management
enhanced_memory_pool_t* memory_manager_create_pool(const char* name, size_t pool_size, 
                                                   memory_allocation_type_t type, size_t alignment) {
    if (!g_memory_manager.initialized) {
        memory_manager_init(true, true); // Auto-initialize with tracking
    }
    
    pthread_mutex_lock(&g_memory_manager.pool_manager_mutex);
    
    if (g_memory_manager.pool_count >= MEMORY_POOL_MAX_COUNT) {
        pthread_mutex_unlock(&g_memory_manager.pool_manager_mutex);
        return NULL;
    }
    
    enhanced_memory_pool_t* pool = &g_memory_manager.pools[g_memory_manager.pool_count];
    memset(pool, 0, sizeof(enhanced_memory_pool_t));
    
    // Initialize pool
    strncpy(pool->name, name ? name : "unnamed", sizeof(pool->name) - 1);
    pool->pool_size = pool_size;
    pool->alignment = alignment ? alignment : sizeof(void*);
    pool->enable_statistics = g_memory_manager.statistics_enabled;
    
    // Allocate pool memory
    pool->pool_memory = malloc(pool_size);
    if (!pool->pool_memory) {
        pthread_mutex_unlock(&g_memory_manager.pool_manager_mutex);
        return NULL;
    }
    
    if (pthread_mutex_init(&pool->pool_mutex, NULL) != 0) {
        free(pool->pool_memory);
        pthread_mutex_unlock(&g_memory_manager.pool_manager_mutex);
        return NULL;
    }
    
    pool->active = true;
    g_memory_manager.pool_count++;
    
    pthread_mutex_unlock(&g_memory_manager.pool_manager_mutex);
    
    // Track pool memory allocation
    if (g_memory_manager.leak_detection_enabled) {
        track_allocation(pool->pool_memory, pool_size, type, __FILE__, __LINE__, __func__);
    }
    
    printf("🏊 Created memory pool '%s': %zu bytes, alignment=%zu\n", 
           pool->name, pool_size, pool->alignment);
    
    return pool;
}

int memory_manager_destroy_pool(enhanced_memory_pool_t* pool) {
    if (!pool || !pool->active) return -1;
    
    pthread_mutex_lock(&pool->pool_mutex);
    
    // Free all blocks in the pool
    memory_block_t* block = pool->first_block;
    while (block) {
        memory_block_t* next = block->next;
        free(block);
        block = next;
    }
    
    // Untrack pool memory
    if (g_memory_manager.leak_detection_enabled) {
        untrack_allocation(pool->pool_memory);
    }
    
    free(pool->pool_memory);
    pool->pool_memory = NULL;
    pool->active = false;
    
    pthread_mutex_unlock(&pool->pool_mutex);
    pthread_mutex_destroy(&pool->pool_mutex);
    
    printf("🧹 Destroyed memory pool '%s'\n", pool->name);
    return 0;
}

// Pool allocation functions
void* memory_manager_pool_alloc(enhanced_memory_pool_t* pool, size_t size) {
    if (!pool || !pool->active || size == 0) return NULL;
    
    uint64_t start_time = pool->enable_statistics ? get_timestamp_ns() : 0;
    
    pthread_mutex_lock(&pool->pool_mutex);
    
    size_t aligned_size = align_size(size, pool->alignment);
    
    // Simple linear allocation from pool
    if (pool->used_size + aligned_size > pool->pool_size) {
        pool->failed_allocations++;
        pthread_mutex_unlock(&pool->pool_mutex);
        return NULL; // Pool exhausted
    }
    
    void* ptr = (char*)pool->pool_memory + pool->used_size;
    pool->used_size += aligned_size;
    pool->allocation_count++;
    pool->total_allocations++;
    
    if (pool->used_size > pool->peak_used_size) {
        pool->peak_used_size = pool->used_size;
    }
    
    // Create memory block entry
    memory_block_t* block = malloc(sizeof(memory_block_t));
    if (block) {
        block->ptr = ptr;
        block->size = aligned_size;
        block->type = MEMORY_TYPE_UNKNOWN; // Pool-specific type
        block->timestamp = get_timestamp_ns();
        block->in_use = true;
        block->next = pool->first_block;
        pool->first_block = block;
    }
    
    // Update statistics
    if (pool->enable_statistics && start_time > 0) {
        uint64_t elapsed = get_timestamp_ns() - start_time;
        pool->total_alloc_time_ns += elapsed;
        pool->avg_alloc_time_ns = pool->total_allocations > 0 ? 
            (uint32_t)(pool->total_alloc_time_ns / pool->total_allocations) : 0;
    }
    
    pthread_mutex_unlock(&pool->pool_mutex);
    
    return ptr;
}

void memory_manager_pool_free(enhanced_memory_pool_t* pool, void* ptr, size_t size) {
    if (!pool || !pool->active || !ptr) return;
    
    uint64_t start_time = pool->enable_statistics ? get_timestamp_ns() : 0;
    
    pthread_mutex_lock(&pool->pool_mutex);
    
    // Find and mark block as freed
    memory_block_t* block = pool->first_block;
    memory_block_t* prev = NULL;
    
    while (block) {
        if (block->ptr == ptr) {
            block->in_use = false;
            pool->allocation_count--;
            
            // Update statistics
            if (pool->enable_statistics && start_time > 0) {
                uint64_t elapsed = get_timestamp_ns() - start_time;
                pool->total_free_time_ns += elapsed;
                // Calculate average free time
                size_t total_frees = pool->total_allocations - pool->allocation_count;
                pool->avg_free_time_ns = total_frees > 0 ? 
                    (uint32_t)(pool->total_free_time_ns / total_frees) : 0;
            }
            
            break;
        }
        prev = block;
        block = block->next;
    }
    
    pthread_mutex_unlock(&pool->pool_mutex);
    
    // Note: We don't actually reclaim memory in this simple implementation
    // In a production system, we'd implement proper free block management
}

// Tracked memory allocation
void* memory_manager_alloc_tracked(size_t size, memory_allocation_type_t type,
                                  const char* file, int line, const char* function) {
    if (!g_memory_manager.initialized) {
        memory_manager_init(true, true);
    }
    
    void* ptr = malloc(size);
    if (!ptr) return NULL;
    
    // Update global statistics
    g_memory_manager.total_allocated_bytes += size;
    g_memory_manager.current_allocated_bytes += size;
    g_memory_manager.total_allocations++;
    
    if (g_memory_manager.current_allocated_bytes > g_memory_manager.peak_allocated_bytes) {
        g_memory_manager.peak_allocated_bytes = g_memory_manager.current_allocated_bytes;
    }
    
    // Check memory pressure
    if (g_memory_manager.memory_pressure_threshold > 0 &&
        g_memory_manager.current_allocated_bytes > g_memory_manager.memory_pressure_threshold) {
        
        if (!g_memory_manager.memory_pressure_detected) {
            g_memory_manager.memory_pressure_detected = true;
            printf("⚠️  Memory pressure detected: %zu bytes (threshold: %zu bytes)\n",
                   g_memory_manager.current_allocated_bytes, g_memory_manager.memory_pressure_threshold);
            
            if (g_memory_manager.pressure_callback) {
                g_memory_manager.pressure_callback(g_memory_manager.current_allocated_bytes,
                                                  g_memory_manager.memory_pressure_threshold);
            }
        }
    } else {
        g_memory_manager.memory_pressure_detected = false;
    }
    
    // Track allocation
    track_allocation(ptr, size, type, file, line, function);
    
    return ptr;
}

void memory_manager_free_tracked(void* ptr, const char* file, int line, const char* function) {
    if (!ptr) return;
    
    // Find allocation size for statistics
    size_t freed_size = 0;
    if (g_memory_manager.leak_detection_enabled) {
        pthread_mutex_lock(&g_memory_manager.allocation_mutex);
        for (size_t i = 0; i < g_memory_manager.allocation_table_size; i++) {
            if (g_memory_manager.allocation_table[i].active && 
                g_memory_manager.allocation_table[i].ptr == ptr) {
                freed_size = g_memory_manager.allocation_table[i].size;
                break;
            }
        }
        pthread_mutex_unlock(&g_memory_manager.allocation_mutex);
    }
    
    // Untrack allocation
    untrack_allocation(ptr);
    
    // Update statistics
    if (freed_size > 0) {
        g_memory_manager.current_allocated_bytes -= freed_size;
    }
    g_memory_manager.total_frees++;
    
    free(ptr);
}

// Leak detection and reporting
int memory_manager_check_leaks(char** leak_report_json) {
    if (!g_memory_manager.initialized || !g_memory_manager.leak_detection_enabled) {
        if (leak_report_json) {
            *leak_report_json = strdup("{\"leak_detection\": \"disabled\"}");
        }
        return 0;
    }
    
    pthread_mutex_lock(&g_memory_manager.allocation_mutex);
    
    json_object* report = json_object_new_object();
    json_object* leaks_array = json_object_new_array();
    json_object* summary = json_object_new_object();
    
    int leak_count = 0;
    size_t leaked_bytes = 0;
    size_t type_counts[MEMORY_TYPE_COUNT] = {0};
    size_t type_bytes[MEMORY_TYPE_COUNT] = {0};
    
    // Scan allocation table for active entries
    for (size_t i = 0; i < g_memory_manager.allocation_table_size; i++) {
        memory_allocation_entry_t* entry = &g_memory_manager.allocation_table[i];
        if (entry->active) {
            leak_count++;
            leaked_bytes += entry->size;
            
            if (entry->type < MEMORY_TYPE_COUNT) {
                type_counts[entry->type]++;
                type_bytes[entry->type] += entry->size;
            }
            
            // Add leak details to report
            json_object* leak_obj = json_object_new_object();
            json_object_object_add(leak_obj, "ptr", json_object_new_string_len((char*)&entry->ptr, 16));
            json_object_object_add(leak_obj, "size", json_object_new_int64(entry->size));
            json_object_object_add(leak_obj, "type", json_object_new_string(memory_type_to_string(entry->type)));
            json_object_object_add(leak_obj, "file", json_object_new_string(entry->file ? entry->file : "unknown"));
            json_object_object_add(leak_obj, "line", json_object_new_int(entry->line));
            json_object_object_add(leak_obj, "function", json_object_new_string(entry->function ? entry->function : "unknown"));
            json_object_object_add(leak_obj, "thread_id", json_object_new_int(entry->thread_id));
            json_object_object_add(leak_obj, "timestamp", json_object_new_int64(entry->timestamp));
            
            json_object_array_add(leaks_array, leak_obj);
        }
    }
    
    // Build summary
    json_object_object_add(summary, "leak_count", json_object_new_int(leak_count));
    json_object_object_add(summary, "leaked_bytes", json_object_new_int64(leaked_bytes));
    json_object_object_add(summary, "active_allocations", json_object_new_int(g_memory_manager.active_allocations));
    
    // Add type breakdown
    json_object* type_breakdown = json_object_new_object();
    for (int t = 0; t < MEMORY_TYPE_COUNT; t++) {
        if (type_counts[t] > 0) {
            json_object* type_info = json_object_new_object();
            json_object_object_add(type_info, "count", json_object_new_int(type_counts[t]));
            json_object_object_add(type_info, "bytes", json_object_new_int64(type_bytes[t]));
            json_object_object_add(type_breakdown, memory_type_to_string(t), type_info);
        }
    }
    json_object_object_add(summary, "by_type", type_breakdown);
    
    json_object_object_add(report, "summary", summary);
    json_object_object_add(report, "leaks", leaks_array);
    
    // Update global leak statistics
    g_memory_manager.leaked_allocations = leak_count;
    g_memory_manager.leaked_bytes = leaked_bytes;
    
    pthread_mutex_unlock(&g_memory_manager.allocation_mutex);
    
    if (leak_report_json) {
        const char* json_str = json_object_to_json_string_ext(report, JSON_C_TO_STRING_PRETTY);
        *leak_report_json = strdup(json_str);
    }
    
    json_object_put(report);
    return leak_count;
}

// Statistics reporting
int memory_manager_get_statistics(char** stats_json) {
    if (!g_memory_manager.initialized || !stats_json) return -1;
    
    json_object* stats = json_object_new_object();
    
    // Global statistics
    json_object_object_add(stats, "initialized", json_object_new_boolean(g_memory_manager.initialized));
    json_object_object_add(stats, "leak_detection_enabled", json_object_new_boolean(g_memory_manager.leak_detection_enabled));
    json_object_object_add(stats, "statistics_enabled", json_object_new_boolean(g_memory_manager.statistics_enabled));
    
    json_object_object_add(stats, "total_allocated_bytes", json_object_new_int64(g_memory_manager.total_allocated_bytes));
    json_object_object_add(stats, "current_allocated_bytes", json_object_new_int64(g_memory_manager.current_allocated_bytes));
    json_object_object_add(stats, "peak_allocated_bytes", json_object_new_int64(g_memory_manager.peak_allocated_bytes));
    json_object_object_add(stats, "total_allocations", json_object_new_int64(g_memory_manager.total_allocations));
    json_object_object_add(stats, "total_frees", json_object_new_int64(g_memory_manager.total_frees));
    json_object_object_add(stats, "leaked_bytes", json_object_new_int64(g_memory_manager.leaked_bytes));
    json_object_object_add(stats, "leaked_allocations", json_object_new_int64(g_memory_manager.leaked_allocations));
    json_object_object_add(stats, "active_allocations", json_object_new_int(g_memory_manager.active_allocations));
    
    // Memory pressure
    json_object_object_add(stats, "memory_pressure_threshold", json_object_new_int64(g_memory_manager.memory_pressure_threshold));
    json_object_object_add(stats, "memory_pressure_detected", json_object_new_boolean(g_memory_manager.memory_pressure_detected));
    
    // Pool statistics
    json_object* pools_array = json_object_new_array();
    pthread_mutex_lock(&g_memory_manager.pool_manager_mutex);
    
    for (int i = 0; i < g_memory_manager.pool_count; i++) {
        if (g_memory_manager.pools[i].active) {
            enhanced_memory_pool_t* pool = &g_memory_manager.pools[i];
            
            json_object* pool_obj = json_object_new_object();
            json_object_object_add(pool_obj, "name", json_object_new_string(pool->name));
            json_object_object_add(pool_obj, "pool_size", json_object_new_int64(pool->pool_size));
            json_object_object_add(pool_obj, "used_size", json_object_new_int64(pool->used_size));
            json_object_object_add(pool_obj, "peak_used_size", json_object_new_int64(pool->peak_used_size));
            json_object_object_add(pool_obj, "allocation_count", json_object_new_int64(pool->allocation_count));
            json_object_object_add(pool_obj, "total_allocations", json_object_new_int64(pool->total_allocations));
            json_object_object_add(pool_obj, "failed_allocations", json_object_new_int64(pool->failed_allocations));
            
            // Utilization percentage
            double utilization = pool->pool_size > 0 ? (double)pool->used_size / pool->pool_size * 100.0 : 0.0;
            json_object_object_add(pool_obj, "utilization_percent", json_object_new_double(utilization));
            
            json_object_array_add(pools_array, pool_obj);
        }
    }
    
    pthread_mutex_unlock(&g_memory_manager.pool_manager_mutex);
    json_object_object_add(stats, "pools", pools_array);
    
    // Runtime information
    uint64_t uptime_ns = get_timestamp_ns() - g_memory_manager.manager_start_time;
    json_object_object_add(stats, "uptime_seconds", json_object_new_double(uptime_ns / 1000000000.0));
    
    const char* json_str = json_object_to_json_string_ext(stats, JSON_C_TO_STRING_PRETTY);
    *stats_json = strdup(json_str);
    json_object_put(stats);
    
    return 0;
}

// Garbage collection
size_t memory_manager_garbage_collect(void) {
    size_t bytes_freed = 0;
    
    if (!g_memory_manager.initialized) return 0;
    
    pthread_mutex_lock(&g_memory_manager.pool_manager_mutex);
    
    // Simple garbage collection: reset pools that are mostly unused
    for (int i = 0; i < g_memory_manager.pool_count; i++) {
        enhanced_memory_pool_t* pool = &g_memory_manager.pools[i];
        
        if (pool->active) {
            pthread_mutex_lock(&pool->pool_mutex);
            
            // If pool usage is very low, reset it
            double utilization = pool->pool_size > 0 ? (double)pool->used_size / pool->pool_size : 0.0;
            if (utilization < 0.1 && pool->allocation_count == 0) {
                bytes_freed += pool->used_size;
                pool->used_size = 0;
                
                // Free all block tracking entries
                memory_block_t* block = pool->first_block;
                while (block) {
                    memory_block_t* next = block->next;
                    free(block);
                    block = next;
                }
                pool->first_block = NULL;
                
                printf("🗑️  Garbage collected pool '%s': freed %zu bytes\n", pool->name, pool->used_size);
            }
            
            pthread_mutex_unlock(&pool->pool_mutex);
        }
    }
    
    pthread_mutex_unlock(&g_memory_manager.pool_manager_mutex);
    
    if (bytes_freed > 0) {
        printf("🗑️  Garbage collection completed: freed %zu bytes total\n", bytes_freed);
    }
    
    return bytes_freed;
}

// Memory pressure monitoring
void memory_manager_set_pressure_monitoring(size_t threshold_bytes, 
                                           void (*callback)(size_t current, size_t threshold)) {
    if (!g_memory_manager.initialized) {
        memory_manager_init(true, true);
    }
    
    g_memory_manager.memory_pressure_threshold = threshold_bytes;
    g_memory_manager.pressure_callback = callback;
    
    printf("📊 Memory pressure monitoring set: threshold=%zu bytes\n", threshold_bytes);
}