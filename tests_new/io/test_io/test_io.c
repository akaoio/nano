#define _GNU_SOURCE
#include "io.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

// Mock external dependencies
void* g_handle_pool = NULL;

int handle_pool_init(void* pool) {
    (void)pool;
    return 0;
}

int execute_method(const char* method, uint32_t handle_id, 
                  const char* params, char* result, size_t result_size) {
    (void)handle_id;
    (void)params;
    
    // Simple mock implementation
    if (strcmp(method, "test_method") == 0) {
        snprintf(result, result_size, "\"success\"");
        return 0;
    }
    
    snprintf(result, result_size, "\"unknown_method\"");
    return -1;
}

void test_io_json_parsing() {
    printf("Testing JSON parsing...\n");
    
    const char* json = "{\"jsonrpc\":\"2.0\",\"id\":123,\"method\":\"test_method\",\"params\":{\"key\":\"value\"}}";
    
    uint32_t request_id, handle_id;
    char method[32], params[4096];
    
    int result = io_parse_json_request(json, &request_id, &handle_id, method, params);
    assert(result == IO_OK);
    assert(request_id == 123);
    assert(strcmp(method, "test_method") == 0);
    assert(strstr(params, "key") != NULL);
    
    printf("JSON parsing tests passed!\n");
}

void test_io_basic_operations() {
    printf("Testing basic IO operations...\n");
    
    // Initialize IO system
    int result = io_init();
    assert(result == IO_OK);
    
    // Push a request
    const char* request = "{\"jsonrpc\":\"2.0\",\"id\":456,\"method\":\"test_method\",\"params\":{\"test\":\"data\"}}";
    result = io_push_request(request);
    assert(result == IO_OK);
    
    // Wait a bit for processing
    usleep(100000); // 100ms
    
    // Pop response
    char response[1024];
    result = io_pop_response(response, sizeof(response));
    assert(result == IO_OK);
    assert(strstr(response, "\"id\":456") != NULL);
    assert(strstr(response, "success") != NULL);
    
    // Shutdown
    io_shutdown();
    
    printf("Basic IO operations tests passed!\n");
}

int main() {
    printf("Running IO tests...\n");
    
    test_io_json_parsing();
    test_io_basic_operations();
    
    printf("All IO tests passed!\n");
    return 0;
}
