#define _GNU_SOURCE
#include "../test_io.h"
#include "../../../src/io/core/io/io.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

void test_io_json_parsing() {
    printf("Testing JSON parsing...\n");
    
    const char* json = "{\"jsonrpc\":\"2.0\",\"id\":123,\"method\":\"test_method\",\"params\":{\"key\":\"value\"}}";
    
    uint32_t request_id, handle_id;
    char method[32], params[4096];
    
    int result = io_parse_json_request(json, &request_id, &handle_id, method, params);
    printf("DEBUG: result=%d, method='%s', expected='test_method'\n", result, method);
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
    
    printf("IO Response: %s\n", response);
    // Check for valid response structure
    assert(strstr(response, "\"id\":456") != NULL);
    
    // Shutdown
    io_shutdown();
    
    printf("Basic IO operations tests passed!\n");
}

int test_io_layer(void) {
    printf("Running IO tests...\n");
    
    test_io_json_parsing();
    test_io_basic_operations();
    
    printf("All IO tests passed!\n");
    return 0;
}
