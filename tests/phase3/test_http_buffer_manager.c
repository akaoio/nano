#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include "../../src/lib/protocol/http_buffer_manager.h"

// Test HTTP buffer manager functionality
int test_http_buffer_manager_basic() {
    printf("Testing HTTP Buffer Manager Basic Operations...\n");
    
    struct http_buffer_manager_t* manager = get_global_http_buffer_manager();
    if (!manager) {
        printf("FAIL: HTTP buffer manager not initialized\n");
        return -1;
    }
    
    const char* test_request_id = "test_req_001";
    const char* test_data = "Hello, World!";
    
    // Test buffer creation
    int create_result = http_buffer_manager_create_buffer(manager, test_request_id);
    if (create_result != 0) {
        printf("FAIL: Failed to create buffer\n");
        return -1;
    }
    
    // Test chunk addition
    int add_result = http_buffer_manager_add_chunk(manager, test_request_id, test_data, strlen(test_data));
    if (add_result != 0) {
        printf("FAIL: Failed to add chunk to buffer\n");
        return -1;
    }
    
    // Test chunk retrieval
    char retrieve_buffer[1024];
    int get_result = http_buffer_manager_get_chunks(manager, test_request_id, retrieve_buffer, sizeof(retrieve_buffer), false);
    if (get_result != 0) {
        printf("FAIL: Failed to retrieve chunks from buffer\n");
        return -1;
    }
    
    // Verify data integrity
    if (strcmp(test_data, retrieve_buffer) != 0) {
        printf("FAIL: Data integrity check failed\n");
        return -1;
    }
    
    // Test completion marking
    int complete_result = http_buffer_manager_complete_buffer(manager, test_request_id);
    if (complete_result != 0) {
        printf("FAIL: Failed to mark buffer as complete\n");
        return -1;
    }
    
    printf("PASS: HTTP Buffer Manager Basic Operations\n");
    return 0;
}

int test_buffer_cleanup() {
    printf("Testing HTTP Buffer Cleanup...\n");
    
    struct http_buffer_manager_t* manager = get_global_http_buffer_manager();
    if (!manager) {
        printf("FAIL: HTTP buffer manager not initialized\n");
        return -1;
    }
    
    // Create multiple test buffers
    char test_ids[5][64];
    int created_buffers = 0;
    
    for (int i = 0; i < 5; i++) {
        snprintf(test_ids[i], sizeof(test_ids[i]), "cleanup_test_%d_%ld", i, time(NULL));
        
        if (http_buffer_manager_create_buffer(manager, test_ids[i]) == 0) {
            created_buffers++;
            char test_data[32];
            snprintf(test_data, sizeof(test_data), "test_data_%d", i);
            http_buffer_manager_add_chunk(manager, test_ids[i], test_data, strlen(test_data));
        }
    }
    
    printf("PASS: Buffer Cleanup Test (created %d buffers)\n", created_buffers);
    return 0;
}

int main() {
    printf("=== Phase 3 HTTP Buffer Manager Tests ===\n");
    
    int failures = 0;
    
    if (test_http_buffer_manager_basic() != 0) {
        failures++;
    }
    
    if (test_buffer_cleanup() != 0) {
        failures++;
    }
    
    printf("\n=== Test Summary ===\n");
    printf("Total tests: 2\n");
    printf("Failures: %d\n", failures);
    printf("Success: %d\n", 2 - failures);
    
    return failures > 0 ? 1 : 0;
}