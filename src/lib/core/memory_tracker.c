#include "memory_tracker.h"
#include "common/time_utils/time_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static memory_tracker_t g_memory_tracker = {0};

int memory_tracker_init(bool log_all_operations) {
    if (g_memory_tracker.initialized) return 0;
    
    if (pthread_mutex_init(&g_memory_tracker.tracker_mutex, NULL) != 0) {
        return -1;
    }
    
    g_memory_tracker.log_file = fopen("memory_tracker.log", "w");
    if (!g_memory_tracker.log_file) {
        fprintf(stderr, "Warning: Could not open memory tracker log file\n");
        // Continue without file logging
    }
    
    g_memory_tracker.log_all_operations = log_all_operations;
    g_memory_tracker.leak_threshold = 1024; // 1KB default threshold
    g_memory_tracker.initialized = true;
    
    if (g_memory_tracker.log_file) {
        fprintf(g_memory_tracker.log_file, 
                "[%llu] Memory tracker initialized (log_all: %s)\n",
                (unsigned long long)get_timestamp_ms(),
                log_all_operations ? "true" : "false");
        fflush(g_memory_tracker.log_file);
    }
    
    return 0;
}

void memory_tracker_shutdown(void) {
    if (!g_memory_tracker.initialized) return;
    
    pthread_mutex_lock(&g_memory_tracker.tracker_mutex);
    
    // Check for memory leaks
    memory_allocation_t* current = g_memory_tracker.allocations;
    size_t leaks = 0;
    size_t leaked_bytes = 0;
    
    if (g_memory_tracker.log_file) {
        fprintf(g_memory_tracker.log_file, 
                "\n[%llu] MEMORY TRACKER SHUTDOWN REPORT\n",
                (unsigned long long)get_timestamp_ms());
        fprintf(g_memory_tracker.log_file, "=====================================\n");
    }
    
    while (current) {
        memory_allocation_t* next = current->next;
        
        leaked_bytes += current->size;
        
        if (g_memory_tracker.log_file) {
            uint64_t age = get_timestamp_ms() - current->timestamp;
            fprintf(g_memory_tracker.log_file, 
                    "LEAK: %p (%zu bytes, age: %llu ms) allocated in %s at %s:%d\n",
                    current->ptr, current->size, (unsigned long long)age,
                    current->function, current->file, current->line);
        }
        
        // Free the leaked memory to prevent actual leaks
        free(current->ptr);
        free(current);
        current = next;
        leaks++;
    }
    
    if (g_memory_tracker.log_file) {
        fprintf(g_memory_tracker.log_file, "\nSUMMARY:\n");
        fprintf(g_memory_tracker.log_file, "Total leaks: %zu allocations (%zu bytes)\n", 
                leaks, leaked_bytes);
        fprintf(g_memory_tracker.log_file, "Peak memory usage: %zu bytes\n", 
                g_memory_tracker.peak_allocated);
        fprintf(g_memory_tracker.log_file, "Total allocations made: %zu\n", 
                g_memory_tracker.total_allocations_made);
        fprintf(g_memory_tracker.log_file, "Total frees made: %zu\n", 
                g_memory_tracker.total_frees_made);
        fprintf(g_memory_tracker.log_file, "Allocation/Free ratio: %.2f\n",
                g_memory_tracker.total_frees_made > 0 ? 
                (double)g_memory_tracker.total_allocations_made / g_memory_tracker.total_frees_made : 0.0);
        fclose(g_memory_tracker.log_file);
    }
    
    // Print leak summary to stderr
    if (leaks > 0) {
        fprintf(stderr, "\n⚠️  MEMORY LEAKS DETECTED: %zu allocations (%zu bytes)\n", 
                leaks, leaked_bytes);
        fprintf(stderr, "📊 See memory_tracker.log for details\n");
    } else {
        fprintf(stderr, "✅ No memory leaks detected\n");
    }
    
    pthread_mutex_unlock(&g_memory_tracker.tracker_mutex);
    pthread_mutex_destroy(&g_memory_tracker.tracker_mutex);
    
    g_memory_tracker.initialized = false;
}

void* memory_tracker_malloc(size_t size, const char* function, const char* file, int line) {
    void* ptr = malloc(size);
    if (!ptr) return NULL;
    
    if (!g_memory_tracker.initialized) {
        return ptr; // Fallback if tracker not initialized
    }
    
    pthread_mutex_lock(&g_memory_tracker.tracker_mutex);
    
    memory_allocation_t* allocation = malloc(sizeof(memory_allocation_t));
    if (allocation) {
        allocation->ptr = ptr;
        allocation->size = size;
        allocation->function = function;
        allocation->file = file;
        allocation->line = line;
        allocation->timestamp = get_timestamp_ms();
        allocation->is_array = false;
        allocation->next = g_memory_tracker.allocations;
        
        g_memory_tracker.allocations = allocation;
        g_memory_tracker.allocation_count++;
        g_memory_tracker.total_allocated += size;
        g_memory_tracker.total_allocations_made++;
        
        // Update peak usage
        if (g_memory_tracker.total_allocated > g_memory_tracker.peak_allocated) {
            g_memory_tracker.peak_allocated = g_memory_tracker.total_allocated;
        }
        
        if (g_memory_tracker.log_all_operations && g_memory_tracker.log_file) {
            fprintf(g_memory_tracker.log_file, 
                    "[%llu] ALLOC: %p (%zu bytes) in %s at %s:%d [Total: %zu bytes, Count: %zu]\n",
                    (unsigned long long)get_timestamp_ms(),
                    ptr, size, function, file, line,
                    g_memory_tracker.total_allocated, g_memory_tracker.allocation_count);
            fflush(g_memory_tracker.log_file);
        }
    }
    
    pthread_mutex_unlock(&g_memory_tracker.tracker_mutex);
    return ptr;
}

void* memory_tracker_calloc(size_t nmemb, size_t size, const char* function, const char* file, int line) {
    size_t total_size = nmemb * size;
    void* ptr = memory_tracker_malloc(total_size, function, file, line);
    if (ptr) {
        memset(ptr, 0, total_size);
    }
    return ptr;
}

void* memory_tracker_realloc(void* ptr, size_t size, const char* function, const char* file, int line) {
    if (!ptr) {
        return memory_tracker_malloc(size, function, file, line);
    }
    
    if (size == 0) {
        memory_tracker_free(ptr, function, file, line);
        return NULL;
    }
    
    if (!g_memory_tracker.initialized) {
        return realloc(ptr, size);
    }
    
    pthread_mutex_lock(&g_memory_tracker.tracker_mutex);
    
    // Find existing allocation
    memory_allocation_t* existing = g_memory_tracker.allocations;
    size_t old_size = 0;
    
    while (existing) {
        if (existing->ptr == ptr) {
            old_size = existing->size;
            break;
        }
        existing = existing->next;
    }
    
    pthread_mutex_unlock(&g_memory_tracker.tracker_mutex);
    
    void* new_ptr = realloc(ptr, size);
    if (!new_ptr) return NULL;
    
    // Update tracking
    if (existing) {
        pthread_mutex_lock(&g_memory_tracker.tracker_mutex);
        existing->ptr = new_ptr;
        existing->size = size;
        existing->function = function;
        existing->file = file;
        existing->line = line;
        existing->timestamp = get_timestamp_ms();
        
        g_memory_tracker.total_allocated = g_memory_tracker.total_allocated - old_size + size;
        
        if (g_memory_tracker.total_allocated > g_memory_tracker.peak_allocated) {
            g_memory_tracker.peak_allocated = g_memory_tracker.total_allocated;
        }
        
        if (g_memory_tracker.log_all_operations && g_memory_tracker.log_file) {
            fprintf(g_memory_tracker.log_file, 
                    "[%llu] REALLOC: %p->%p (%zu->%zu bytes) in %s at %s:%d\n",
                    (unsigned long long)get_timestamp_ms(),
                    ptr, new_ptr, old_size, size, function, file, line);
            fflush(g_memory_tracker.log_file);
        }
        
        pthread_mutex_unlock(&g_memory_tracker.tracker_mutex);
    } else {
        // Treat as new allocation if not found in tracking
        memory_tracker_free(new_ptr, function, file, line);
        new_ptr = memory_tracker_malloc(size, function, file, line);
    }
    
    return new_ptr;
}

void memory_tracker_free(void* ptr, const char* function, const char* file, int line) {
    if (!ptr) return;
    
    if (!g_memory_tracker.initialized) {
        free(ptr);
        return;
    }
    
    pthread_mutex_lock(&g_memory_tracker.tracker_mutex);
    
    // Find and remove allocation from tracking list
    memory_allocation_t** current = &g_memory_tracker.allocations;
    bool found = false;
    size_t freed_size = 0;
    
    while (*current) {
        if ((*current)->ptr == ptr) {
            memory_allocation_t* to_remove = *current;
            *current = (*current)->next;
            
            freed_size = to_remove->size;
            g_memory_tracker.allocation_count--;
            g_memory_tracker.total_allocated -= freed_size;
            g_memory_tracker.total_frees_made++;
            
            if (g_memory_tracker.log_all_operations && g_memory_tracker.log_file) {
                uint64_t lifetime = get_timestamp_ms() - to_remove->timestamp;
                fprintf(g_memory_tracker.log_file, 
                        "[%llu] FREE: %p (%zu bytes, lifetime: %llu ms) freed in %s at %s:%d [Total: %zu bytes, Count: %zu]\n",
                        (unsigned long long)get_timestamp_ms(),
                        ptr, freed_size, (unsigned long long)lifetime,
                        function, file, line,
                        g_memory_tracker.total_allocated, g_memory_tracker.allocation_count);
                fflush(g_memory_tracker.log_file);
            }
            
            free(to_remove);
            found = true;
            break;
        }
        current = &(*current)->next;
    }
    
    if (!found && g_memory_tracker.log_file) {
        fprintf(g_memory_tracker.log_file, 
                "[%llu] WARNING: Freeing untracked pointer %p in %s at %s:%d\n",
                (unsigned long long)get_timestamp_ms(),
                ptr, function, file, line);
        fflush(g_memory_tracker.log_file);
    }
    
    pthread_mutex_unlock(&g_memory_tracker.tracker_mutex);
    free(ptr);
}

char* memory_tracker_strdup(const char* str, const char* function, const char* file, int line) {
    if (!str) return NULL;
    
    size_t len = strlen(str) + 1;
    char* dup = memory_tracker_malloc(len, function, file, line);
    if (dup) {
        memcpy(dup, str, len);
    }
    return dup;
}

int memory_tracker_get_stats(memory_stats_t* stats) {
    if (!stats || !g_memory_tracker.initialized) return -1;
    
    pthread_mutex_lock(&g_memory_tracker.tracker_mutex);
    
    stats->allocation_count = g_memory_tracker.allocation_count;
    stats->total_allocated = g_memory_tracker.total_allocated;
    stats->peak_allocated = g_memory_tracker.peak_allocated;
    stats->total_allocations_made = g_memory_tracker.total_allocations_made;
    stats->total_frees_made = g_memory_tracker.total_frees_made;
    stats->potential_leaks = 0;
    
    // Count potential leaks (allocations above threshold or very old)
    memory_allocation_t* current = g_memory_tracker.allocations;
    uint64_t now = get_timestamp_ms();
    
    while (current) {
        uint64_t age = now - current->timestamp;
        if (current->size >= g_memory_tracker.leak_threshold || age > 300000) { // 5 minutes
            stats->potential_leaks++;
        }
        current = current->next;
    }
    
    pthread_mutex_unlock(&g_memory_tracker.tracker_mutex);
    return 0;
}

size_t memory_tracker_check_leaks(void) {
    if (!g_memory_tracker.initialized) return 0;
    
    memory_stats_t stats;
    memory_tracker_get_stats(&stats);
    return stats.potential_leaks;
}

void memory_tracker_print_report(FILE* output_file) {
    if (!output_file) output_file = stderr;
    if (!g_memory_tracker.initialized) {
        fprintf(output_file, "Memory tracker not initialized\n");
        return;
    }
    
    memory_stats_t stats;
    memory_tracker_get_stats(&stats);
    
    fprintf(output_file, "\n📊 MEMORY TRACKER REPORT\n");
    fprintf(output_file, "========================\n");
    fprintf(output_file, "Current allocations: %zu (%zu bytes)\n", 
            stats.allocation_count, stats.total_allocated);
    fprintf(output_file, "Peak memory usage: %zu bytes (%.2f MB)\n", 
            stats.peak_allocated, (double)stats.peak_allocated / (1024 * 1024));
    fprintf(output_file, "Total allocations made: %zu\n", stats.total_allocations_made);
    fprintf(output_file, "Total frees made: %zu\n", stats.total_frees_made);
    fprintf(output_file, "Potential leaks: %zu\n", stats.potential_leaks);
    
    if (stats.total_frees_made > 0) {
        double ratio = (double)stats.total_allocations_made / stats.total_frees_made;
        fprintf(output_file, "Allocation/Free ratio: %.2f %s\n", ratio,
                ratio > 1.1 ? "(⚠️  Possible leak)" : "(✅ Good)");
    }
    
    fprintf(output_file, "========================\n\n");
}

bool memory_tracker_is_initialized(void) {
    return g_memory_tracker.initialized;
}

void memory_tracker_set_leak_threshold(size_t threshold) {
    g_memory_tracker.leak_threshold = threshold;
}