#include "../test_lora.h"
#include "../../../src/io/mapping/handle_pool/handle_pool.h"
#include "../../../src/io/mapping/rkllm_proxy/rkllm_proxy.h"
#include "../../../src/common/json_utils/json_utils.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

void test_lora_inference(uint32_t handle_id);

int test_lora_model_loading() {
    printf("🔍 Testing LoRA model loading...\n");
    
    // Initialize systems
    handle_pool_t pool;
    assert(handle_pool_init(&pool) == 0);
    assert(rkllm_proxy_init() == 0);
    
    // Test LoRA model loading
    char params[512];
    snprintf(params, sizeof(params), 
             "{\"model_path\":\"/home/x/Projects/nano/models/lora/model.rkllm\","
             "\"lora_adapter_path\":\"/home/x/Projects/nano/models/lora/lora.rkllm\"}");
    
    // Create request structure
    rkllm_request_t request = {
        .operation = rkllm_proxy_get_operation_by_name("init"),
        .handle_id = 0,
        .params_json = params,
        .params_size = strlen(params)
    };
    
    rkllm_result_t result = {0};
    int ret = rkllm_proxy_execute(&request, &result);
    
    if (ret == 0) {
        printf("✅ LoRA model loaded successfully!\n");
        printf("📋 Result: %s\n", result.result_data ? result.result_data : "NULL");
        
        // Extract handle_id from result 
        // Simple JSON parsing for handle_id
        const char* handle_start = strstr(result.result_data, "\"handle_id\":");
        if (handle_start) {
            handle_start += 12; // Skip "handle_id":
            uint32_t handle_id = (uint32_t)strtoul(handle_start, NULL, 10);
            printf("🎯 LoRA handle ID: %u\n", handle_id);
            
            // Test inference with "hello"
            test_lora_inference(handle_id);
            
            // Destroy handle to free NPU memory
            char destroy_params[64];
            snprintf(destroy_params, sizeof(destroy_params), "{\"handle_id\":%u}", handle_id);
            
            rkllm_request_t destroy_request = {
                .operation = rkllm_proxy_get_operation_by_name("destroy"),
                .handle_id = handle_id,
                .params_json = destroy_params,
                .params_size = strlen(destroy_params)
            };
            
            rkllm_result_t destroy_result = {0};
            rkllm_proxy_execute(&destroy_request, &destroy_result);
            rkllm_proxy_free_result(&destroy_result);
            
            rkllm_proxy_free_result(&result);
            rkllm_proxy_shutdown();
            return 0;
        } else {
            printf("⚠️  No handle_id found in result\n");
            rkllm_proxy_free_result(&result);
            rkllm_proxy_shutdown();
            return 1;
        }
    } else {
        printf("❌ Failed to load LoRA model\n");
        printf("📋 Error: %s\n", result.result_data ? result.result_data : "NULL");
        rkllm_proxy_free_result(&result);
        rkllm_proxy_shutdown();
        return 1;
    }
}

void test_lora_inference(uint32_t handle_id) {
    printf("🚀 Testing LoRA inference with 'hello'...\n");
    
    char params[512];
    snprintf(params, sizeof(params), 
             "{\"handle_id\":%u,\"prompt\":\"hello\"}", handle_id);
    
    // Create request structure
    rkllm_request_t request = {
        .operation = rkllm_proxy_get_operation_by_name("run"),
        .handle_id = handle_id,
        .params_json = params,
        .params_size = strlen(params)
    };
    
    rkllm_result_t result = {0};
    int ret = rkllm_proxy_execute(&request, &result);
    
    if (ret == 0) {
        printf("✅ LoRA inference successful!\n");
        printf("🤖 Response: %s\n", result.result_data ? result.result_data : "NULL");
        rkllm_proxy_free_result(&result);
    } else {
        printf("❌ LoRA inference failed\n");
        printf("📋 Error: %s\n", result.result_data ? result.result_data : "Unknown error");
        rkllm_proxy_free_result(&result);
    }
}

int test_integration_lora(void) {
    printf("🧪 LORA INTEGRATION TEST\n");
    printf("========================\n\n");
    
    // Check if models exist
    if (access("/home/x/Projects/nano/models/lora/model.rkllm", F_OK) != 0) {
        printf("❌ LoRA base model not found at /home/x/Projects/nano/models/lora/model.rkllm\n");
        return 1;
    }
    
    if (access("/home/x/Projects/nano/models/lora/lora.rkllm", F_OK) != 0) {
        printf("❌ LoRA adapter not found at /home/x/Projects/nano/models/lora/lora.rkllm\n");
        return 1;
    }
    
    // Add extended delay to allow NPU memory cleanup
    printf("⏳ Waiting for memory cleanup...\n");
    sleep(10); // Extended delay for NPU memory to be fully released
    
    int loading_result = test_lora_model_loading();
    
    printf("\n🎉 LoRA integration test completed!\n");
    return loading_result;
}
