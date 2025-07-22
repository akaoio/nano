#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include "../../src/lib/system/logger.h"

int test_logging_system_status() {
    printf("Testing Logging System Status...\n");
    
    // Initialize logging system
    if (logger_init(LOG_INFO, false, "test_mcp_server.log", 1024*1024) != 0) {
        printf("FAIL: Could not initialize logging system\n");
        return -1;
    }
    
    // Check if logger is initialized
    if (!logger_is_initialized()) {
        printf("FAIL: Logger reports not initialized after init\n");
        return -1;
    }
    
    // Test different log levels
    logger_info("Testing INFO level log");
    logger_warn("Testing WARN level log");
    logger_error("Testing ERROR level log");
    logger_debug("Testing DEBUG level log (might not appear depending on log level)");
    
    // Test structured logging
    logger_info("JSON test: {\"component\": \"test\", \"status\": \"running\"}");
    
    printf("PASS: Logging System Status\n");
    
    // Cleanup
    logger_cleanup();
    
    return 0;
}

int main() {
    printf("=== Phase 5 Logging System Tests ===\n");
    
    int failures = 0;
    
    if (test_logging_system_status() != 0) {
        failures++;
    }
    
    printf("\n=== Test Summary ===\n");
    printf("Total tests: 1\n");
    printf("Failures: %d\n", failures);
    printf("Success: %d\n", 1 - failures);
    
    return failures > 0 ? 1 : 0;
}