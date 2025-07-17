#include "../test_system.h"
#include "../../../src/nano/system/system_info/system_info.h"
#include "../../../src/nano/system/resource_mgr/resource_mgr.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

void test_system_detect() {
    printf("Testing system_detect...\n");
    
    system_info_t info;
    int result = system_detect(&info);
    assert(result == 0);
    assert(info.total_ram_mb > 0);
    assert(info.available_ram_mb > 0);
    assert(info.cpu_cores > 0);
    assert(info.npu_cores > 0);
    
    printf("system_detect tests passed!\n");
}

void test_resource_mgr_init() {
    printf("Testing resource_mgr_init...\n");
    
    resource_mgr_t mgr;
    int result = resource_mgr_init(&mgr);
    assert(result == 0);
    assert(mgr.model_count == 0);
    assert(mgr.total_memory_used == 0);
    
    printf("resource_mgr_init tests passed!\n");
}

void test_resource_mgr_operations() {
    printf("Testing resource_mgr operations...\n");
    
    resource_mgr_t mgr;
    resource_mgr_init(&mgr);
    
    // Test memory usage
    uint64_t usage = resource_mgr_get_memory_usage(&mgr);
    assert(usage == 0);
    
    // Test cleanup
    int result = resource_mgr_cleanup(&mgr);
    assert(result == 0);
    
    printf("resource_mgr operations tests passed!\n");
}

void test_system_memory_functions() {
    printf("Testing system memory functions...\n");
    
    // Test memory cleanup functions
    int result = system_force_gc();
    assert(result == 0);
    
    result = system_free_memory();
    assert(result == 0);
    
    // Test memory refresh
    system_info_t info;
    system_detect(&info);
    result = system_refresh_memory_info(&info);
    assert(result == 0);
    
    printf("system memory functions tests passed!\n");
}

int test_nano_system(void) {
    printf("Running system tests...\n");
    
    test_system_detect();
    test_resource_mgr_init();
    test_resource_mgr_operations();
    test_system_memory_functions();
    
    printf("All system tests passed!\n");
    return 0;
}
