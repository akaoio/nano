#include "../test_lora.h"
#include "../../../src/io/mapping/handle_pool/handle_pool.h"
#include "../../../src/io/mapping/rkllm_proxy/rkllm_proxy.h"
#include <json-c/json.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

// Forward declaration
int test_lora_inference(uint32_t handle_id);

int test_lora_model_loading() {
    printf("🔍 Testing LoRA model loading...\n");
    
    // Initialize systems
    handle_pool_t pool;
    assert(handle_pool_init(&pool) == 0);
    assert(rkllm_proxy_init() == 0);
    
    // Test LoRA model loading - using json-c
    json_object *params_obj = json_object_new_object();
    json_object *model_path = json_object_new_string("/home/x/Projects/nano/models/lora/model.rkllm");
    json_object *lora_path = json_object_new_string("/home/x/Projects/nano/models/lora/lora.rkllm");
    
    json_object_object_add(params_obj, "model_path", model_path);
    json_object_object_add(params_obj, "lora_adapter_path", lora_path);
    
    const char *params_json = json_object_to_json_string(params_obj);
    
    // Create request structure
    rkllm_request_t request = {
        .operation = rkllm_proxy_get_operation_by_name("init"),
        .handle_id = 0,
        .params_json = (char*)params_json,
        .params_size = strlen(params_json)
    };
    
    rkllm_result_t result = {0};
    int ret = rkllm_proxy_execute(&request, &result);
    
    if (ret == 0) {
        printf("✅ LoRA model loaded successfully!\n");
        printf("📋 Result: %s\n", result.result_data ? result.result_data : "NULL");
        
        // Extract handle_id from result using json-c
        if (result.result_data) {
            json_object *result_obj = json_tokener_parse(result.result_data);
            if (result_obj) {
                json_object *handle_id_obj;
                if (json_object_object_get_ex(result_obj, "handle_id", &handle_id_obj)) {
                    uint32_t handle_id = (uint32_t)json_object_get_int(handle_id_obj);
                    printf("🎯 LoRA handle ID: %u\n", handle_id);
                    
                    // Test inference with "hello"
                    int inference_result = test_lora_inference(handle_id);
                    
                    // Destroy handle to free NPU memory - using json-c
                    json_object *destroy_params_obj = json_object_new_object();
                    json_object *handle_id_destroy = json_object_new_int(handle_id);
                    json_object_object_add(destroy_params_obj, "handle_id", handle_id_destroy);
                    
                    const char *destroy_params_json = json_object_to_json_string(destroy_params_obj);
                    
                    rkllm_request_t destroy_request = {
                        .operation = rkllm_proxy_get_operation_by_name("destroy"),
                        .handle_id = handle_id,
                        .params_json = (char*)destroy_params_json,
                        .params_size = strlen(destroy_params_json)
                    };
                    
                    rkllm_result_t destroy_result = {0};
                    rkllm_proxy_execute(&destroy_request, &destroy_result);
                    rkllm_proxy_free_result(&destroy_result);
                    
                    json_object_put(destroy_params_obj);
                    json_object_put(result_obj);
                    
                    json_object_put(params_obj);
                    rkllm_proxy_free_result(&result);
                    rkllm_proxy_shutdown();
                    return inference_result;
                }
                json_object_put(result_obj);
            }
        }
        
        printf("⚠️  No handle_id found in result\n");
        json_object_put(params_obj);
        rkllm_proxy_free_result(&result);
        rkllm_proxy_shutdown();
        return 1;
    } else {
        printf("❌ Failed to load LoRA model\n");
        printf("📋 Error: %s\n", result.result_data ? result.result_data : "NULL");
        json_object_put(params_obj);
        rkllm_proxy_free_result(&result);
        rkllm_proxy_shutdown();
        return 1;
    }
}

int test_lora_inference(uint32_t handle_id) {
    printf("🚀 Testing LoRA inference with 'hello'...\n");
    
    // Create params using json-c
    json_object *params_obj = json_object_new_object();
    json_object *handle_id_obj = json_object_new_int(handle_id);
    json_object *prompt_obj = json_object_new_string("hello");
    
    json_object_object_add(params_obj, "handle_id", handle_id_obj);
    json_object_object_add(params_obj, "prompt", prompt_obj);
    
    const char *params_json = json_object_to_json_string(params_obj);
    
    // Create request structure
    rkllm_request_t request = {
        .operation = rkllm_proxy_get_operation_by_name("run"),
        .handle_id = handle_id,
        .params_json = (char*)params_json,
        .params_size = strlen(params_json)
    };
    
    rkllm_result_t result = {0};
    int ret = rkllm_proxy_execute(&request, &result);
    
    if (ret == 0) {
        printf("✅ LoRA inference successful!\n");
        printf("🤖 Response: %s\n", result.result_data ? result.result_data : "NULL");
    } else {
        printf("❌ LoRA inference failed: %d\n", ret);
        printf("📋 Error: %s\n", result.result_data ? result.result_data : "NULL");
    }
    
    json_object_put(params_obj);
    rkllm_proxy_free_result(&result);
    return ret;
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
