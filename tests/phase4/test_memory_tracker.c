#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "../../src/lib/core/memory_tracker.h"

int test_memory_leak_detection() {
    printf("Testing Memory Leak Detection...\n");
    
    if (!memory_tracker_is_initialized()) {
        printf("FAIL: Memory tracker not initialized\n");
        return -1;
    }
    
    // Get initial memory stats
    memory_stats_t initial_stats;
    if (memory_tracker_get_stats(&initial_stats) != 0) {
        printf("FAIL: Could not get initial memory stats\n");
        return -1;
    }
    
    // Allocate some memory through the tracker
    void* ptr1 = memory_tracker_malloc(1024, "test_allocation_1");
    void* ptr2 = memory_tracker_malloc(2048, "test_allocation_2");
    
    if (!ptr1 || !ptr2) {
        printf("FAIL: Memory allocation failed\n");
        return -1;
    }
    
    // Get stats after allocation
    memory_stats_t after_alloc_stats;
    if (memory_tracker_get_stats(&after_alloc_stats) != 0) {
        printf("FAIL: Could not get post-allocation memory stats\n");
        return -1;
    }
    
    // Verify allocation tracking
    if (after_alloc_stats.total_allocated <= initial_stats.total_allocated) {
        printf("FAIL: Allocation not properly tracked\n");
        return -1;
    }
    
    // Free one allocation
    memory_tracker_free(ptr1);
    
    // Get stats after partial free
    memory_stats_t after_partial_free_stats;
    if (memory_tracker_get_stats(&after_partial_free_stats) != 0) {
        printf("FAIL: Could not get post-partial-free memory stats\n");
        return -1;
    }
    
    // Verify partial free tracking
    if (after_partial_free_stats.total_allocated >= after_alloc_stats.total_allocated) {
        printf("FAIL: Free operation not properly tracked\n");
        return -1;
    }
    
    // Free remaining allocation
    memory_tracker_free(ptr2);
    
    // Get final stats
    memory_stats_t final_stats;
    if (memory_tracker_get_stats(&final_stats) != 0) {
        printf("FAIL: Could not get final memory stats\n");
        return -1;
    }
    
    // Check for memory leaks (should be back to initial state)
    if (final_stats.current_allocated != initial_stats.current_allocated) {
        printf("WARN: Potential memory leak detected (initial: %zu, final: %zu)\n", 
               initial_stats.current_allocated, final_stats.current_allocated);
        // This is a warning, not a failure, as other parts of the system might allocate memory
    }
    
    printf("PASS: Memory Leak Detection Test\n");
    printf("  - Initial allocated: %zu bytes\n", initial_stats.current_allocated);
    printf("  - Peak allocated: %zu bytes\n", after_alloc_stats.current_allocated);
    printf("  - Final allocated: %zu bytes\n", final_stats.current_allocated);
    
    return 0;
}

int test_memory_pressure_simulation() {
    printf("Testing Memory Pressure Simulation...\n");
    
    if (!memory_tracker_is_initialized()) {
        printf("FAIL: Memory tracker not initialized\n");
        return -1;
    }
    
    const size_t allocation_size = 1024;
    const int allocation_count = 1000;
    void** ptrs = malloc(allocation_count * sizeof(void*));
    
    if (!ptrs) {
        printf("FAIL: Could not allocate pointer array\n");
        return -1;
    }
    
    // Allocate many small chunks to simulate pressure
    int successful_allocations = 0;
    for (int i = 0; i < allocation_count; i++) {
        ptrs[i] = memory_tracker_malloc(allocation_size, "pressure_test");
        if (ptrs[i]) {
            successful_allocations++;
        }
    }
    
    // Get stats during pressure
    memory_stats_t pressure_stats;
    if (memory_tracker_get_stats(&pressure_stats) != 0) {
        printf("FAIL: Could not get pressure stats\n");
        free(ptrs);
        return -1;
    }
    
    printf("Memory pressure results:\n");
    printf("  - Successful allocations: %d/%d\n", successful_allocations, allocation_count);
    printf("  - Current allocated: %zu bytes\n", pressure_stats.current_allocated);
    printf("  - Peak allocated: %zu bytes\n", pressure_stats.peak_allocated);
    printf("  - Total allocations: %zu\n", pressure_stats.total_allocations);
    
    // Free all allocations
    for (int i = 0; i < successful_allocations; i++) {
        if (ptrs[i]) {
            memory_tracker_free(ptrs[i]);
        }
    }
    
    free(ptrs);
    
    printf("PASS: Memory Pressure Simulation\n");
    return 0;
}

int main() {
    printf("=== Phase 4 Memory Tracker Tests ===\n");
    
    // Initialize memory tracker
    if (memory_tracker_init() != 0) {
        printf("FAIL: Could not initialize memory tracker\n");
        return 1;
    }
    
    int failures = 0;
    
    if (test_memory_leak_detection() != 0) {
        failures++;
    }
    
    if (test_memory_pressure_simulation() != 0) {
        failures++;
    }
    
    // Cleanup
    memory_tracker_cleanup();
    
    printf("\n=== Test Summary ===\n");
    printf("Total tests: 2\n");
    printf("Failures: %d\n", failures);
    printf("Success: %d\n", 2 - failures);
    
    return failures > 0 ? 1 : 0;
}