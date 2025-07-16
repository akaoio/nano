#include "rkllm_proxy.h"
#include "../handle_pool/handle_pool.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

// Global handle pool for testing
handle_pool_t g_handle_pool;

void test_rkllm_proxy_init() {
    printf("Testing rkllm_proxy_init...\n");
    
    int result = rkllm_proxy_init();
    assert(result == 0);
    
    printf("rkllm_proxy_init tests passed!\n");
}

void test_proxy_execute_unknown_method() {
    printf("Testing unknown method execution...\n");
    
    char result[256];
    int ret = rkllm_proxy_execute("unknown_method", 1, "{}", result, sizeof(result));
    assert(ret == -1);
    assert(strstr(result, "Unknown method") != NULL);
    
    printf("Unknown method tests passed!\n");
}

void test_proxy_init_method() {
    printf("Testing proxy_init method...\n");
    
    // Initialize handle pool
    handle_pool_init(&g_handle_pool);
    
    // Test valid init
    char result[256];
    const char* params = "{\"model_path\":\"test_model.rkllm\"}";
    int ret = rkllm_proxy_execute("init", 0, params, result, sizeof(result));
    
    // Should succeed and return handle_id
    assert(ret == 0);
    assert(strstr(result, "handle_id") != NULL);
    assert(strstr(result, "success") != NULL);
    
    printf("proxy_init method tests passed!\n");
}

void test_proxy_status_method() {
    printf("Testing proxy_status method...\n");
    
    char result[256];
    
    // Test invalid handle_id
    int ret = rkllm_proxy_execute("status", 999, "{}", result, sizeof(result));
    assert(ret == 0);
    assert(strstr(result, "invalid") != NULL);
    
    // Test valid handle_id (assuming handle_id 1 exists from previous test)
    ret = rkllm_proxy_execute("status", 1, "{}", result, sizeof(result));
    assert(ret == 0);
    assert(strstr(result, "handle_id") != NULL);
    
    printf("proxy_status method tests passed!\n");
}

void test_proxy_run_method() {
    printf("Testing proxy_run method...\n");
    
    char result[512];
    const char* params = "{\"prompt\":\"Hello, world!\"}";
    
    // Test with valid handle_id
    int ret = rkllm_proxy_execute("run", 1, params, result, sizeof(result));
    assert(ret == 0);
    assert(strstr(result, "response") != NULL);
    
    // Test with invalid handle_id
    ret = rkllm_proxy_execute("run", 999, params, result, sizeof(result));
    assert(ret == -1);
    assert(strstr(result, "Invalid handle_id") != NULL);
    
    printf("proxy_run method tests passed!\n");
}

void test_proxy_destroy_method() {
    printf("Testing proxy_destroy method...\n");
    
    char result[256];
    
    // Test destroying valid handle
    int ret = rkllm_proxy_execute("destroy", 1, "{}", result, sizeof(result));
    assert(ret == 0);
    assert(strstr(result, "destroyed") != NULL);
    
    // Test destroying invalid handle
    ret = rkllm_proxy_execute("destroy", 999, "{}", result, sizeof(result));
    assert(ret == -1);
    assert(strstr(result, "Failed to destroy") != NULL);
    
    printf("proxy_destroy method tests passed!\n");
}

int main() {
    printf("Running RKLLM Proxy tests...\n");
    
    test_rkllm_proxy_init();
    test_proxy_execute_unknown_method();
    test_proxy_init_method();
    test_proxy_status_method();
    test_proxy_run_method();
    test_proxy_destroy_method();
    
    printf("All RKLLM Proxy tests passed!\n");
    return 0;
}
