#ifndef MEMORY_TRACKER_H
#define MEMORY_TRACKER_H

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include <pthread.h>
#include <stdio.h>

/**
 * @file memory_tracker.h
 * @brief Application Memory Allocation Tracking System
 * 
 * Tracks all application-level memory allocations to detect and prevent memory leaks.
 * NOTE: This does NOT track NPU memory, which is managed internally by RKLLM.
 */

typedef struct memory_allocation {
    void* ptr;                      // Allocated pointer
    size_t size;                    // Allocation size
    const char* function;           // Function where allocated
    const char* file;               // Source file
    int line;                       // Source line number
    uint64_t timestamp;             // Allocation timestamp
    bool is_array;                  // True if allocated with array new
    struct memory_allocation* next; // Linked list next pointer
} memory_allocation_t;

typedef struct {
    memory_allocation_t* allocations;   // Linked list of tracked allocations
    size_t allocation_count;            // Number of active allocations
    size_t total_allocated;             // Total bytes currently allocated
    size_t peak_allocated;              // Peak memory usage
    size_t total_allocations_made;      // Total allocations made (lifetime)
    size_t total_frees_made;            // Total frees made (lifetime)
    pthread_mutex_t tracker_mutex;     // Thread safety mutex
    bool initialized;                   // Initialization status
    FILE* log_file;                     // Log file for memory operations
    bool log_all_operations;           // Log every malloc/free
    size_t leak_threshold;              // Leak detection threshold
} memory_tracker_t;

/**
 * @brief Initialize the memory tracking system
 * @param log_all_operations If true, log every memory operation
 * @return 0 on success, -1 on failure
 */
int memory_tracker_init(bool log_all_operations);

/**
 * @brief Shutdown the memory tracking system and report leaks
 */
void memory_tracker_shutdown(void);

/**
 * @brief Allocate memory with tracking
 * @param size Size to allocate
 * @param function Calling function name
 * @param file Source file name
 * @param line Source line number
 * @return Allocated pointer or NULL on failure
 */
void* memory_tracker_malloc(size_t size, const char* function, const char* file, int line);

/**
 * @brief Allocate zeroed memory with tracking
 * @param nmemb Number of elements
 * @param size Size of each element
 * @param function Calling function name
 * @param file Source file name
 * @param line Source line number
 * @return Allocated pointer or NULL on failure
 */
void* memory_tracker_calloc(size_t nmemb, size_t size, const char* function, const char* file, int line);

/**
 * @brief Reallocate memory with tracking
 * @param ptr Existing pointer (can be NULL)
 * @param size New size
 * @param function Calling function name
 * @param file Source file name
 * @param line Source line number
 * @return Reallocated pointer or NULL on failure
 */
void* memory_tracker_realloc(void* ptr, size_t size, const char* function, const char* file, int line);

/**
 * @brief Free tracked memory
 * @param ptr Pointer to free
 * @param function Calling function name
 * @param file Source file name
 * @param line Source line number
 */
void memory_tracker_free(void* ptr, const char* function, const char* file, int line);

/**
 * @brief Get current memory usage statistics
 * @param stats Output structure for statistics
 * @return 0 on success, -1 on failure
 */
typedef struct {
    size_t allocation_count;
    size_t total_allocated;
    size_t peak_allocated;
    size_t total_allocations_made;
    size_t total_frees_made;
    size_t potential_leaks;
} memory_stats_t;

int memory_tracker_get_stats(memory_stats_t* stats);

/**
 * @brief Check for memory leaks
 * @return Number of potential leaks detected
 */
size_t memory_tracker_check_leaks(void);

/**
 * @brief Force garbage collection of old allocations
 * @param max_age_ms Maximum age in milliseconds for allocations to keep
 * @return Number of old allocations found (not freed)
 */
size_t memory_tracker_gc_old_allocations(uint64_t max_age_ms);

/**
 * @brief Print memory usage report to file or stderr
 * @param output_file File to write to (NULL for stderr)
 */
void memory_tracker_print_report(FILE* output_file);

/**
 * @brief Mark a memory region for leak detection
 * @param ptr Pointer to mark
 * @param expected_lifetime_ms Expected lifetime in milliseconds
 */
void memory_tracker_mark_expected_lifetime(void* ptr, uint64_t expected_lifetime_ms);

/**
 * @brief Check if memory tracker is initialized
 * @return true if initialized, false otherwise
 */
bool memory_tracker_is_initialized(void);

/**
 * @brief Set leak detection threshold
 * @param threshold Number of bytes above which to consider a potential leak
 */
void memory_tracker_set_leak_threshold(size_t threshold);

// Convenience macros for tracked allocation
#define TRACKED_MALLOC(size) memory_tracker_malloc(size, __FUNCTION__, __FILE__, __LINE__)
#define TRACKED_CALLOC(nmemb, size) memory_tracker_calloc(nmemb, size, __FUNCTION__, __FILE__, __LINE__)
#define TRACKED_REALLOC(ptr, size) memory_tracker_realloc(ptr, size, __FUNCTION__, __FILE__, __LINE__)
#define TRACKED_FREE(ptr) memory_tracker_free(ptr, __FUNCTION__, __FILE__, __LINE__)

// String allocation helpers
#define TRACKED_STRDUP(str) memory_tracker_strdup(str, __FUNCTION__, __FILE__, __LINE__)
char* memory_tracker_strdup(const char* str, const char* function, const char* file, int line);

// Array allocation helpers
#define TRACKED_MALLOC_ARRAY(type, count) \
    ((type*)memory_tracker_malloc(sizeof(type) * (count), __FUNCTION__, __FILE__, __LINE__))

#define TRACKED_CALLOC_ARRAY(type, count) \
    ((type*)memory_tracker_calloc(count, sizeof(type), __FUNCTION__, __FILE__, __LINE__))

#endif // MEMORY_TRACKER_H