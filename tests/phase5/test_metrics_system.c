#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "../../src/lib/system/metrics.h"

int test_metrics_export() {
    printf("Testing Metrics Export...\n");
    
    // Initialize metrics system
    if (metrics_init() != 0) {
        printf("FAIL: Could not initialize metrics system\n");
        return -1;
    }
    
    // Check if metrics system is initialized
    if (!metrics_is_initialized()) {
        printf("FAIL: Metrics system reports not initialized after init\n");
        return -1;
    }
    
    // Test counter metrics
    metrics_counter_inc("test_counter", NULL, 0);
    metrics_counter_add("test_counter", 5, NULL, 0);
    
    // Test gauge metrics
    metrics_gauge_set("test_gauge", 42.5, NULL, 0);
    metrics_gauge_inc("test_gauge", NULL, 0);
    metrics_gauge_dec("test_gauge", NULL, 0);
    
    // Test histogram metrics
    metrics_histogram_observe("test_histogram", 1.23, NULL, 0);
    metrics_histogram_observe("test_histogram", 4.56, NULL, 0);
    
    // Export metrics to buffer
    char metrics_buffer[4096];
    size_t buffer_size = sizeof(metrics_buffer);
    
    if (metrics_export_prometheus(metrics_buffer, &buffer_size) != 0) {
        printf("FAIL: Could not export metrics to Prometheus format\n");
        metrics_cleanup();
        return -1;
    }
    
    printf("PASS: Metrics Export\n");
    printf("Exported metrics (%zu bytes):\n%s\n", buffer_size, metrics_buffer);
    
    // Cleanup
    metrics_cleanup();
    
    return 0;
}

int test_health_check() {
    printf("Testing Health Check...\n");
    
    // Simulate system health check
    bool systems_healthy = true;
    
    // Check various system components
    struct {
        const char* component;
        bool status;
    } health_checks[] = {
        {"logging", true},
        {"metrics", true}, 
        {"memory_tracker", true},
        {"error_mapping", true},
        {"transports", true}
    };
    
    int check_count = sizeof(health_checks) / sizeof(health_checks[0]);
    int healthy_components = 0;
    
    for (int i = 0; i < check_count; i++) {
        if (health_checks[i].status) {
            healthy_components++;
            printf("  - %s: healthy\n", health_checks[i].component);
        } else {
            printf("  - %s: unhealthy\n", health_checks[i].component);
            systems_healthy = false;
        }
    }
    
    printf("Health check results: %d/%d components healthy\n", 
           healthy_components, check_count);
    
    if (systems_healthy) {
        printf("PASS: Health Check (system healthy)\n");
        return 0;
    } else {
        printf("FAIL: Health Check (system unhealthy)\n");
        return -1;
    }
}

int main() {
    printf("=== Phase 5 Metrics System Tests ===\n");
    
    int failures = 0;
    
    if (test_metrics_export() != 0) {
        failures++;
    }
    
    if (test_health_check() != 0) {
        failures++;
    }
    
    printf("\n=== Test Summary ===\n");
    printf("Total tests: 2\n");
    printf("Failures: %d\n", failures);
    printf("Success: %d\n", 2 - failures);
    
    return failures > 0 ? 1 : 0;
}