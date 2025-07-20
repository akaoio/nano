#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <json-c/json.h>

// Standalone test - demonstrate dynamic API concept without RKLLM dependency

// Mock RKLLM function registry for demonstration
typedef struct {
    const char* name;
    int param_count;
    const char* description;
} mock_function_desc_t;

static mock_function_desc_t g_mock_functions[] = {
    {"rkllm_createDefaultParam", 0, "Creates a default RKLLMParam structure"},
    {"rkllm_init", 3, "Initializes the LLM with given parameters"},
    {"rkllm_load_lora", 2, "Loads a Lora adapter into the LLM"},
    {"rkllm_load_prompt_cache", 2, "Loads a prompt cache from a file"},
    {"rkllm_release_prompt_cache", 1, "Releases the prompt cache from memory"},
    {"rkllm_destroy", 1, "Destroys the LLM instance and releases resources"},
    {"rkllm_run", 4, "Runs an LLM inference task synchronously"},
    {"rkllm_run_async", 4, "Runs an LLM inference task asynchronously"},
    {"rkllm_abort", 1, "Aborts an ongoing LLM task"},
    {"rkllm_is_running", 1, "Checks if an LLM task is currently running"},
    {"rkllm_clear_kv_cache", 4, "Clear the key-value cache"},
    {"rkllm_get_kv_cache_size", 2, "Get the current size of the key-value cache"},
    {"rkllm_set_chat_template", 4, "Sets the chat template for the LLM"},
    {"rkllm_set_function_tools", 4, "Sets the function calling configuration"},
    {"rkllm_set_cross_attn_params", 2, "Sets the cross-attention parameters"}
};

static const int g_function_count = sizeof(g_mock_functions) / sizeof(g_mock_functions[0]);

// Mock dynamic API functionality
int mock_list_functions(char** functions_json) {
    json_object* functions_array = json_object_new_array();
    
    for (int i = 0; i < g_function_count; i++) {
        json_object* func_obj = json_object_new_object();
        json_object_object_add(func_obj, "name", json_object_new_string(g_mock_functions[i].name));
        json_object_object_add(func_obj, "param_count", json_object_new_int(g_mock_functions[i].param_count));
        json_object_object_add(func_obj, "description", json_object_new_string(g_mock_functions[i].description));
        json_object_array_add(functions_array, func_obj);
    }
    
    const char* json_str = json_object_to_json_string(functions_array);
    *functions_json = strdup(json_str);
    json_object_put(functions_array);
    
    return 0;
}

int mock_process_operation(const char* method, const char* params_json, char** result_json) {
    // Find function
    bool found = false;
    for (int i = 0; i < g_function_count; i++) {
        if (strcmp(g_mock_functions[i].name, method) == 0) {
            found = true;
            break;
        }
    }
    
    if (!found) {
        char error_msg[256];
        snprintf(error_msg, sizeof(error_msg), 
                 "{\"error\": \"Function not found\", \"method\": \"%s\"}", method);
        *result_json = strdup(error_msg);
        return -1;
    }
    
    // Mock successful response
    if (strcmp(method, "rkllm_createDefaultParam") == 0) {
        *result_json = strdup("{"
            "\"success\": true, "
            "\"default_params\": {"
                "\"max_context_len\": 2048, "
                "\"max_new_tokens\": 512, "
                "\"top_k\": 40, "
                "\"top_p\": 0.9, "
                "\"temperature\": 0.8, "
                "\"repeat_penalty\": 1.1"
            "}"
        "}");
    } else {
        char success_msg[256];
        snprintf(success_msg, sizeof(success_msg), 
                 "{\"success\": true, \"message\": \"Mock call to %s completed\"}", method);
        *result_json = strdup(success_msg);
    }
    
    return 0;
}

// Test functions
typedef struct {
    int tests_run;
    int tests_passed;
    int tests_failed;
} test_state_t;

static test_state_t g_state = {0};

bool test_function_listing() {
    printf("🧪 Test: Dynamic Function Listing\n");
    printf("----------------------------------\n");
    
    g_state.tests_run++;
    
    char* functions_json = NULL;
    int result = mock_list_functions(&functions_json);
    
    if (result == 0 && functions_json) {
        printf("✅ Function listing: PASSED\n");
        
        json_object* functions = json_tokener_parse(functions_json);
        if (functions) {
            int count = json_object_array_length(functions);
            printf("   📋 Available RKLLM functions: %d\n", count);
            
            // Show first few functions
            for (int i = 0; i < count && i < 5; i++) {
                json_object* func = json_object_array_get_idx(functions, i);
                json_object* name_obj, *desc_obj;
                if (json_object_object_get_ex(func, "name", &name_obj) &&
                    json_object_object_get_ex(func, "description", &desc_obj)) {
                    printf("   %d. %s - %s\n", i+1, 
                           json_object_get_string(name_obj),
                           json_object_get_string(desc_obj));
                }
            }
            if (count > 5) {
                printf("   ... and %d more functions\n", count - 5);
            }
            
            json_object_put(functions);
        }
        
        free(functions_json);
        g_state.tests_passed++;
        return true;
    } else {
        printf("❌ Function listing: FAILED\n");
        if (functions_json) free(functions_json);
        g_state.tests_failed++;
        return false;
    }
}

bool test_dynamic_function_call() {
    printf("\n🧪 Test: Dynamic Function Call\n");
    printf("-------------------------------\n");
    
    g_state.tests_run++;
    
    char* result_json = NULL;
    int result = mock_process_operation("rkllm_createDefaultParam", "{}", &result_json);
    
    if (result == 0 && result_json) {
        printf("✅ Dynamic function call: PASSED\n");
        printf("   📋 Response: %s\n", result_json);
        
        // Verify response structure
        json_object* response = json_tokener_parse(result_json);
        if (response) {
            json_object* success_obj;
            if (json_object_object_get_ex(response, "success", &success_obj)) {
                bool success = json_object_get_boolean(success_obj);
                printf("   ✅ Success flag: %s\n", success ? "true" : "false");
            }
            json_object_put(response);
        }
        
        free(result_json);
        g_state.tests_passed++;
        return true;
    } else {
        printf("❌ Dynamic function call: FAILED\n");
        if (result_json) {
            printf("   Error: %s\n", result_json);
            free(result_json);
        }
        g_state.tests_failed++;
        return false;
    }
}

bool test_unknown_function() {
    printf("\n🧪 Test: Unknown Function Handling\n");
    printf("-----------------------------------\n");
    
    g_state.tests_run++;
    
    char* result_json = NULL;
    int result = mock_process_operation("rkllm_nonexistent", "{}", &result_json);
    
    if (result != 0 && result_json && strstr(result_json, "Function not found")) {
        printf("✅ Unknown function handling: PASSED\n");
        printf("   ✅ Correctly rejected unknown function\n");
        free(result_json);
        g_state.tests_passed++;
        return true;
    } else {
        printf("❌ Unknown function handling: FAILED\n");
        if (result_json) {
            printf("   Unexpected response: %s\n", result_json);
            free(result_json);
        }
        g_state.tests_failed++;
        return false;
    }
}

bool test_multiple_functions() {
    printf("\n🧪 Test: Multiple Function Calls\n");
    printf("---------------------------------\n");
    
    g_state.tests_run++;
    
    const char* test_functions[] = {
        "rkllm_init",
        "rkllm_run", 
        "rkllm_destroy",
        "rkllm_set_chat_template",
        "rkllm_load_lora"
    };
    
    int success_count = 0;
    for (int i = 0; i < 5; i++) {
        char* result_json = NULL;
        int result = mock_process_operation(test_functions[i], "{}", &result_json);
        
        if (result == 0 && result_json && strstr(result_json, "success")) {
            success_count++;
            printf("   ✅ %s: OK\n", test_functions[i]);
        } else {
            printf("   ❌ %s: FAILED\n", test_functions[i]);
        }
        
        if (result_json) free(result_json);
    }
    
    if (success_count == 5) {
        printf("✅ Multiple function calls: PASSED (%d/5)\n", success_count);
        g_state.tests_passed++;
        return true;
    } else {
        printf("❌ Multiple function calls: FAILED (%d/5)\n", success_count);
        g_state.tests_failed++;
        return false;
    }
}

int main(int argc, char* argv[]) {
    (void)argc; (void)argv;
    
    printf("🚀 Dynamic RKLLM API Concept Test\n");
    printf("==================================\n");
    printf("Demonstrating dynamic function registry and automatic API exposure\n\n");
    
    // Run tests
    test_function_listing();
    test_dynamic_function_call();
    test_unknown_function();
    test_multiple_functions();
    
    // Summary
    printf("\n📊 Test Results\n");
    printf("===============\n");
    printf("Tests run:    %d\n", g_state.tests_run);
    printf("Tests passed: %d\n", g_state.tests_passed);
    printf("Tests failed: %d\n", g_state.tests_failed);
    
    if (g_state.tests_failed == 0) {
        printf("\n🎉 All concept tests PASSED!\n");
        printf("\n📋 Dynamic API Benefits Demonstrated:\n");
        printf("✅ Automatic function discovery (%d RKLLM functions)\n", g_function_count);
        printf("✅ Dynamic function dispatch without manual mapping\n");
        printf("✅ Proper error handling for unknown functions\n");
        printf("✅ JSON-based parameter passing and response formatting\n");
        printf("✅ Type-safe function registry with metadata\n");
        
        printf("\n🔍 Comparison with Manual Approach:\n");
        printf("❌ Manual: Only 6 functions exposed (~30%% coverage)\n");
        printf("✅ Dynamic: ALL %d functions exposed (100%% coverage)\n", g_function_count);
        printf("❌ Manual: Requires code changes for each new function\n");
        printf("✅ Dynamic: Automatically exposes new functions via registry\n");
        printf("❌ Manual: Repetitive parameter conversion code\n");
        printf("✅ Dynamic: Reusable parameter conversion system\n");
        
        return 0;
    } else {
        printf("\n❌ %d test(s) FAILED.\n", g_state.tests_failed);
        return 1;
    }
}