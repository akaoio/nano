#include "test_io.h"
#include <string.h>
#include <unistd.h>

// Test inference with a prompt
int test_model_inference(uint32_t handle, const char* model_name) {
    char request[1024];
    snprintf(request, sizeof(request),
        "{\"jsonrpc\":\"2.0\",\"id\":10,\"method\":\"run\",\"params\":{\"handle_id\":%u,\"prompt\":\"hello\"}}",
        handle);
    
    printf("🤖 %s: \"hello\" → ", model_name);
    fflush(stdout);
    
    if (io_push_request(request) != 0) {
        printf("❌ Push failed\n");
        return 0;
    }
    
    char response[1024];
    for (int i = 0; i < 50; i++) {
        if (io_pop_response(response, sizeof(response)) == 0) {
            if (strstr(response, "\"error\"")) {
                printf("❌ Error in response: %s\n", response);
                return 0;
            }
            
            const char* text_start = strstr(response, "\"text\":\"");
            if (text_start) {
                text_start += 8;
                const char* text_end = strchr(text_start, '"');
                if (text_end) {
                    size_t len = text_end - text_start;
                    if (len > 0 && len < 200) {
                        printf("\"%.*s\"\n", (int)len, text_start);
                        return 1;
                    }
                }
            }
        }
        usleep(200000);
    }
    
    printf("❌ Timeout\n");
    return 0;
}

// Cleanup model handle
void test_cleanup_model(uint32_t handle, const char* model_name) {
    if (handle == 0) return;
    
    printf("🧹 Cleaning up %s (handle=%u)...\n", model_name, handle);
    
    char request[256];
    snprintf(request, sizeof(request),
        "{\"jsonrpc\":\"2.0\",\"id\":999,\"method\":\"destroy\",\"params\":{\"handle_id\":%u}}", handle);
    
    if (io_push_request(request) == 0) {
        char response[512];
        for (int i = 0; i < 50; i++) {
            if (io_pop_response(response, sizeof(response)) == 0) {
                if (strstr(response, "\"destroyed\"")) {
                    printf("✅ %s cleanup successful\n", model_name);
                    system_force_gc();
                    return;
                }
                if (strstr(response, "\"error\"")) {
                    printf("⚠️  %s cleanup error: %s\n", model_name, response);
                    break;
                }
            }
            usleep(100000);
        }
    }
    
    printf("⚠️  %s cleanup timeout or failed\n", model_name);
    system_force_gc();
}

