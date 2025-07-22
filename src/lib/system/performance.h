#ifndef SYSTEM_PERFORMANCE_H
#define SYSTEM_PERFORMANCE_H

#include <stdint.h>
#include <stdbool.h>
#include <pthread.h>

/**
 * @file performance.h
 * @brief Performance Optimization System
 * 
 * Provides buffer pooling and performance optimizations to reduce
 * memory allocation overhead and improve throughput.
 */

typedef struct buffer_pool_entry {
    void* buffer;
    size_t size;
    bool in_use;
    uint64_t allocated_at;
    struct buffer_pool_entry* next;
} buffer_pool_entry_t;

typedef struct {
    buffer_pool_entry_t* entries;
    size_t pool_size;
    size_t buffer_size;
    size_t available_count;
    size_t allocated_count;
    size_t total_requests;
    size_t pool_hits;
    size_t pool_misses;
    pthread_mutex_t pool_mutex;
    bool initialized;
    const char* pool_name;
} buffer_pool_t;

typedef struct {
    buffer_pool_t pools[4]; // 1K, 4K, 8K, 16K buffers
    bool initialized;
    uint64_t start_time;
    uint64_t total_allocations;
    uint64_t total_frees;
    uint64_t pool_allocations;
    uint64_t pool_frees;
} performance_system_t;

/**
 * @brief Initialize the performance system
 * @return 0 on success, -1 on failure
 */
int performance_init(void);

/**
 * @brief Shutdown the performance system
 */
void performance_shutdown(void);

/**
 * @brief Check if performance system is initialized
 * @return true if initialized, false otherwise
 */
bool performance_is_initialized(void);

/**
 * @brief Get buffer from pool or allocate new one
 * @param size Requested buffer size
 * @return Pointer to buffer, NULL on failure
 */
void* performance_get_buffer(size_t size);

/**
 * @brief Return buffer to pool or free it
 * @param buffer Buffer pointer
 * @param size Original buffer size (for validation)
 */
void performance_return_buffer(void* buffer, size_t size);

/**
 * @brief Get optimal buffer size for given size
 * @param size Requested size
 * @return Optimal buffer size (power of 2)
 */
size_t performance_get_optimal_buffer_size(size_t size);

/**
 * @brief Get buffer pool statistics
 * @param pool_index Index of pool (0-3)
 * @param total_requests Output: total buffer requests
 * @param pool_hits Output: requests served from pool
 * @param pool_misses Output: requests requiring new allocation
 * @param available_count Output: currently available buffers
 * @param allocated_count Output: currently allocated buffers
 * @return 0 on success, -1 on failure
 */
int performance_get_pool_stats(int pool_index, size_t* total_requests, 
                               size_t* pool_hits, size_t* pool_misses,
                               size_t* available_count, size_t* allocated_count);

/**
 * @brief Get overall performance statistics
 * @param total_allocations Output: total allocations performed
 * @param total_frees Output: total frees performed
 * @param pool_allocations Output: allocations from pools
 * @param pool_frees Output: returns to pools
 * @param uptime_ms Output: system uptime in milliseconds
 * @return 0 on success, -1 on failure
 */
int performance_get_system_stats(uint64_t* total_allocations, uint64_t* total_frees,
                                 uint64_t* pool_allocations, uint64_t* pool_frees,
                                 uint64_t* uptime_ms);

/**
 * @brief Test buffer pool performance
 * @param operations Number of operations to perform
 * @param buffer_size Buffer size to test
 * @param ops_per_sec Output: operations per second achieved
 * @return 0 on success, -1 on failure
 */
int performance_test_buffer_pool(int operations, size_t buffer_size, double* ops_per_sec);

/**
 * @brief Simulate memory pressure and test graceful handling
 * @param max_memory_mb Maximum memory to use in MB
 * @param handled_gracefully Output: true if handled without errors
 * @return 0 on success, -1 on failure
 */
int performance_test_memory_pressure(int max_memory_mb, bool* handled_gracefully);

/**
 * @brief Clear pool statistics
 */
void performance_clear_stats(void);

/**
 * @brief Force garbage collection of unused pool entries
 * @return Number of entries freed
 */
int performance_gc_pools(void);

#endif // SYSTEM_PERFORMANCE_H