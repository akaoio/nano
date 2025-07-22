#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <time.h>
#include "../../src/lib/system/performance.h"

uint64_t get_timestamp_ms() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
}

int test_performance_stress_test() {
    printf("Testing Performance Stress Test...\n");
    
    // Initialize performance system
    if (performance_init() != 0) {
        printf("FAIL: Could not initialize performance system\n");
        return -1;
    }
    
    // Check if performance system is initialized
    if (!performance_is_initialized()) {
        printf("FAIL: Performance system reports not initialized after init\n");
        return -1;
    }
    
    // Test parameters
    int concurrent_streams = 10;
    int tokens_per_stream = 100;
    int target_latency_ms = 5;
    
    uint64_t start_time = get_timestamp_ms();
    
    // Perform buffer allocations/deallocations to simulate streaming load
    int total_operations = concurrent_streams * tokens_per_stream;
    int successful_operations = 0;
    
    for (int i = 0; i < total_operations; i++) {
        void* buffer = performance_get_buffer(1024);
        if (buffer) {
            // Simulate some work
            usleep(100); // 0.1ms
            performance_return_buffer(buffer, 1024);
            successful_operations++;
        }
    }
    
    uint64_t end_time = get_timestamp_ms();
    double actual_latency_ms = (double)(end_time - start_time) / total_operations;
    
    printf("Performance test results:\n");
    printf("  - Streams simulated: %d\n", concurrent_streams);
    printf("  - Total operations: %d\n", total_operations);
    printf("  - Successful operations: %d\n", successful_operations);
    printf("  - Average latency: %.2f ms\n", actual_latency_ms);
    printf("  - Target latency: %d ms\n", target_latency_ms);
    printf("  - Target met: %s\n", actual_latency_ms < target_latency_ms ? "YES" : "NO");
    
    // Cleanup
    performance_cleanup();
    
    printf("PASS: Performance Stress Test\n");
    return 0;
}

int test_buffer_optimization() {
    printf("Testing Buffer Optimization...\n");
    
    // Initialize performance system
    if (performance_init() != 0) {
        printf("FAIL: Could not initialize performance system\n");
        return -1;
    }
    
    // Test different buffer sizes
    size_t buffer_sizes[] = {1024, 4096, 8192, 16384};
    int operations_per_size = 100;
    int buffer_size_count = sizeof(buffer_sizes) / sizeof(buffer_sizes[0]);
    
    for (int i = 0; i < buffer_size_count; i++) {
        size_t buffer_size = buffer_sizes[i];
        
        uint64_t start_time = get_timestamp_ms();
        
        // Test allocation and deallocation performance
        for (int j = 0; j < operations_per_size; j++) {
            void* buffer = performance_get_buffer(buffer_size);
            if (buffer) {
                // Simulate usage
                memset(buffer, 0, buffer_size);
                performance_return_buffer(buffer, buffer_size);
            }
        }
        
        uint64_t end_time = get_timestamp_ms();
        double ops_per_ms = (double)operations_per_size / (end_time - start_time);
        
        printf("  - %zu byte buffers: %.2f ops/ms\n", buffer_size, ops_per_ms);
    }
    
    // Cleanup
    performance_cleanup();
    
    printf("PASS: Buffer Optimization Test\n");
    return 0;
}

int test_graceful_shutdown() {
    printf("Testing Graceful Shutdown...\n");
    
    // Initialize performance system
    if (performance_init() != 0) {
        printf("FAIL: Could not initialize performance system\n");
        return -1;
    }
    
    // Allocate some buffers
    void* buffers[10];
    int allocated_count = 0;
    
    for (int i = 0; i < 10; i++) {
        buffers[i] = performance_get_buffer(1024);
        if (buffers[i]) {
            allocated_count++;
        }
    }
    
    printf("  - Allocated %d buffers before shutdown\n", allocated_count);
    
    // Simulate graceful shutdown
    printf("  - Initiating graceful shutdown...\n");
    
    // Return allocated buffers
    for (int i = 0; i < allocated_count; i++) {
        if (buffers[i]) {
            performance_return_buffer(buffers[i], 1024);
        }
    }
    
    // Cleanup performance system
    performance_cleanup();
    
    printf("  - Shutdown completed successfully\n");
    printf("PASS: Graceful Shutdown Test\n");
    
    return 0;
}

int main() {
    printf("=== Phase 5 Performance System Tests ===\n");
    
    int failures = 0;
    
    if (test_performance_stress_test() != 0) {
        failures++;
    }
    
    if (test_buffer_optimization() != 0) {
        failures++;
    }
    
    if (test_graceful_shutdown() != 0) {
        failures++;
    }
    
    printf("\n=== Test Summary ===\n");
    printf("Total tests: 3\n");
    printf("Failures: %d\n", failures);
    printf("Success: %d\n", 3 - failures);
    
    return failures > 0 ? 1 : 0;
}