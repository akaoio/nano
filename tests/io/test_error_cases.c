#include "test_io.h"

int test_error_cases(void) {
    printf("\n❌ Testing error cases...\n");
    
    // Test non-existent model
    char bad_model[1024];
    snprintf(bad_model, sizeof(bad_model),
        "{\"jsonrpc\":\"2.0\",\"id\":5,\"method\":\"init\",\"params\":{\"model_path\":\"nonexistent.rkllm\"}}");
    
    printf("Test 5a: Non-existent model... ");
    if (io_push_request(bad_model) != 0) {
        printf("FAIL - push failed\n");
        return 1;
    }
    
    char response[1024];
    int retries = 50;
    while (retries-- > 0) {
        if (io_pop_response(response, sizeof(response)) == 0) {
            printf("Response: %s\n", response);
            if (strstr(response, "\"error\"")) {
                printf("✅ SUCCESS - Error handled correctly: %s\n", response);
                break;
            }
        }
        usleep(100000);
    }
    
    // Test invalid handle
    char invalid_handle[256];
    snprintf(invalid_handle, sizeof(invalid_handle),
        "{\"jsonrpc\":\"2.0\",\"id\":6,\"method\":\"run\",\"params\":{\"handle_id\":999,\"prompt\":\"test\"}}");
    
    printf("Test 5b: Invalid handle... ");
    if (io_push_request(invalid_handle) != 0) {
        printf("FAIL - push failed\n");
        return 1;
    }
    
    retries = 50;
    while (retries-- > 0) {
        if (io_pop_response(response, sizeof(response)) == 0) {
            printf("Response: %s\n", response);
            if (strstr(response, "\"error\"")) {
                printf("✅ SUCCESS - Error handled correctly: %s\n", response);
                break;
            }
        }
        usleep(100000);
    }
    
    return 0;
}
