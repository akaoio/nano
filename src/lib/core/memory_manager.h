#ifndef MEMORY_MANAGER_H
#define MEMORY_MANAGER_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <pthread.h>
#include <time.h>

// Memory allocation tracking for leak detection
#define MEMORY_TRACK_STACK_DEPTH 8
#define MEMORY_TRACK_MAX_ENTRIES 16384
#define MEMORY_POOL_MAX_COUNT 16

// Memory allocation types for categorization
typedef enum {
    MEMORY_TYPE_UNKNOWN = 0,
    MEMORY_TYPE_RKLLM_ARRAYS,
    MEMORY_TYPE_STREAMING_BUFFERS,
    MEMORY_TYPE_TRANSPORT_BUFFERS,
    MEMORY_TYPE_JSON_OBJECTS,
    MEMORY_TYPE_STRING_BUFFERS,
    MEMORY_TYPE_SESSION_DATA,
    MEMORY_TYPE_TEMP_STORAGE,
    MEMORY_TYPE_SYSTEM_BUFFERS,
    MEMORY_TYPE_COUNT
} memory_allocation_type_t;

// Memory allocation entry for tracking
typedef struct {
    void* ptr;                              // Allocated pointer
    size_t size;                            // Allocation size
    memory_allocation_type_t type;          // Allocation type
    uint64_t timestamp;                     // Allocation timestamp
    const char* file;                       // Source file
    int line;                               // Source line
    const char* function;                   // Function name
    uint32_t thread_id;                     // Thread ID
    bool active;                            // Is allocation active
    
    // Stack trace for debugging (simplified)
    void* stack_trace[MEMORY_TRACK_STACK_DEPTH];
    int stack_depth;
} memory_allocation_entry_t;

// Memory pool with enhanced tracking and management
typedef struct {
    char name[32];                          // Pool name
    void* pool_memory;                      // Pre-allocated memory pool
    size_t pool_size;                       // Total pool size
    size_t used_size;                       // Currently used size
    size_t peak_used_size;                  // Peak usage
    size_t allocation_count;                // Number of active allocations
    size_t total_allocations;               // Total allocations ever made
    size_t failed_allocations;              // Failed allocation attempts
    
    // Memory blocks tracking
    struct memory_block* first_block;       // Linked list of allocated blocks
    
    // Performance metrics
    uint64_t total_alloc_time_ns;           // Total time spent on allocations
    uint64_t total_free_time_ns;            // Total time spent on frees
    uint32_t avg_alloc_time_ns;             // Average allocation time
    uint32_t avg_free_time_ns;              // Average free time
    
    // Thread safety
    pthread_mutex_t pool_mutex;
    
    // Configuration
    bool allow_fragmentation;               // Allow fragmented allocations
    bool enable_statistics;                 // Enable detailed statistics
    size_t alignment;                       // Memory alignment requirement
    
    bool active;                            // Pool is active
} enhanced_memory_pool_t;

// Memory block within a pool
typedef struct memory_block {
    void* ptr;                              // Block pointer
    size_t size;                            // Block size
    memory_allocation_type_t type;          // Allocation type
    uint64_t timestamp;                     // Allocation timestamp
    struct memory_block* next;              // Next block
    bool in_use;                            // Block is in use
} memory_block_t;

// Global memory manager
typedef struct {
    bool initialized;
    bool leak_detection_enabled;
    bool statistics_enabled;
    
    // Allocation tracking
    memory_allocation_entry_t* allocation_table;
    size_t allocation_table_size;
    size_t active_allocations;
    pthread_mutex_t allocation_mutex;
    
    // Memory pools
    enhanced_memory_pool_t pools[MEMORY_POOL_MAX_COUNT];
    int pool_count;
    pthread_mutex_t pool_manager_mutex;
    
    // Global statistics
    size_t total_allocated_bytes;
    size_t current_allocated_bytes;
    size_t peak_allocated_bytes;
    size_t total_allocations;
    size_t total_frees;
    size_t leaked_bytes;
    size_t leaked_allocations;
    
    // Performance metrics
    uint64_t manager_start_time;
    uint32_t allocation_rate;               // Allocations per second
    uint32_t free_rate;                     // Frees per second
    
    // Memory pressure monitoring
    size_t memory_pressure_threshold;       // Bytes threshold for pressure
    bool memory_pressure_detected;
    void (*pressure_callback)(size_t current_usage, size_t threshold);
    
} memory_manager_t;

// Function declarations

/**
 * @brief Initialize the enhanced memory management system
 * @param enable_leak_detection Enable leak detection
 * @param enable_statistics Enable detailed statistics
 * @return 0 on success, -1 on error
 */
int memory_manager_init(bool enable_leak_detection, bool enable_statistics);

/**
 * @brief Shutdown memory management system
 * @return 0 on success, leaked allocation count on memory leaks
 */
int memory_manager_shutdown(void);

/**
 * @brief Create enhanced memory pool
 * @param name Pool name
 * @param pool_size Pool size in bytes
 * @param type Memory type for this pool
 * @param alignment Memory alignment (0 for default)
 * @return Pool pointer or NULL on error
 */
enhanced_memory_pool_t* memory_manager_create_pool(const char* name, size_t pool_size, 
                                                   memory_allocation_type_t type, size_t alignment);

/**
 * @brief Destroy memory pool
 * @param pool Pool pointer
 * @return 0 on success, -1 on error
 */
int memory_manager_destroy_pool(enhanced_memory_pool_t* pool);

/**
 * @brief Allocate memory with tracking (use MEMORY_ALLOC macro instead)
 * @param size Allocation size
 * @param type Allocation type
 * @param file Source file
 * @param line Source line
 * @param function Function name
 * @return Allocated pointer or NULL on error
 */
void* memory_manager_alloc_tracked(size_t size, memory_allocation_type_t type,
                                  const char* file, int line, const char* function);

/**
 * @brief Free tracked memory (use MEMORY_FREE macro instead)
 * @param ptr Pointer to free
 * @param file Source file
 * @param line Source line
 * @param function Function name
 */
void memory_manager_free_tracked(void* ptr, const char* file, int line, const char* function);

/**
 * @brief Allocate from specific pool
 * @param pool Pool pointer
 * @param size Allocation size
 * @return Allocated pointer or NULL on error
 */
void* memory_manager_pool_alloc(enhanced_memory_pool_t* pool, size_t size);

/**
 * @brief Free to specific pool
 * @param pool Pool pointer
 * @param ptr Pointer to free
 * @param size Original allocation size
 */
void memory_manager_pool_free(enhanced_memory_pool_t* pool, void* ptr, size_t size);

/**
 * @brief Check for memory leaks
 * @param leak_report_json Output JSON string with leak report
 * @return Number of leaked allocations
 */
int memory_manager_check_leaks(char** leak_report_json);

/**
 * @brief Get memory statistics
 * @param stats_json Output JSON string with statistics
 * @return 0 on success, -1 on error
 */
int memory_manager_get_statistics(char** stats_json);

/**
 * @brief Get pool statistics
 * @param pool Pool pointer
 * @param stats_json Output JSON string with pool statistics
 * @return 0 on success, -1 on error
 */
int memory_manager_get_pool_statistics(enhanced_memory_pool_t* pool, char** stats_json);

/**
 * @brief Trigger garbage collection on pools
 * @return Number of bytes freed
 */
size_t memory_manager_garbage_collect(void);

/**
 * @brief Set memory pressure threshold and callback
 * @param threshold_bytes Threshold in bytes
 * @param callback Callback function when threshold exceeded
 */
void memory_manager_set_pressure_monitoring(size_t threshold_bytes, 
                                           void (*callback)(size_t current, size_t threshold));

/**
 * @brief Validate memory integrity
 * @param detailed_report Generate detailed report
 * @return 0 if valid, number of corruption issues found
 */
int memory_manager_validate_integrity(bool detailed_report);

// Convenience macros for tracked memory allocation
#define MEMORY_ALLOC(size, type) \
    memory_manager_alloc_tracked(size, type, __FILE__, __LINE__, __func__)

#define MEMORY_FREE(ptr) \
    memory_manager_free_tracked(ptr, __FILE__, __LINE__, __func__)

#define MEMORY_ALLOC_ARRAY(count, type, mem_type) \
    (type*)memory_manager_alloc_tracked((count) * sizeof(type), mem_type, __FILE__, __LINE__, __func__)

#define MEMORY_REALLOC(ptr, old_size, new_size, type) \
    memory_manager_realloc_tracked(ptr, old_size, new_size, type, __FILE__, __LINE__, __func__)

// Memory type name helpers
const char* memory_type_to_string(memory_allocation_type_t type);
memory_allocation_type_t memory_type_from_string(const char* type_str);

#endif // MEMORY_MANAGER_H