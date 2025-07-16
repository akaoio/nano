#include "test_io.h"

int test_qwenvl_loading(uint32_t* handle) {
    *handle = 0;
    
    FILE *qwenvl_file = fopen("models/qwenvl/model.rkllm", "r");
    if (!qwenvl_file) {
        printf("❌ QwenVL model not found, skipping test\n");
        return 1;
    }
    fclose(qwenvl_file);
    
    char init_request[1024];
    snprintf(init_request, sizeof(init_request),
        "{\"jsonrpc\":\"2.0\",\"id\":1,\"method\":\"init\",\"params\":{\"model_path\":\"models/qwenvl/model.rkllm\",\"max_context_len\":512,\"temperature\":0.7}}");
    
    printf("Test 3a: QwenVL init... ");
    if (io_push_request(init_request) != 0) {
        printf("FAIL - push failed\n");
        return 1;
    }
    
    char response[1024];
    int retries = 50;
    while (retries-- > 0) {
        if (io_pop_response(response, sizeof(response)) == 0) {
            printf("Response: %s\n", response);
            if (strstr(response, "\"handle_id\"")) {
                sscanf(response, "{\"handle_id\":%u}", handle);
                printf("✅ SUCCESS - QwenVL loaded, handle=%u\n", *handle);
                return 0;
            } else if (strstr(response, "\"error\"")) {
                printf("❌ FAILED - QwenVL loading error: %s\n", response);
                return 1;
            }
        }
        usleep(100000);
    }
    
    printf("❌ FAILED - No response from QwenVL init\n");
    return 1;
}
