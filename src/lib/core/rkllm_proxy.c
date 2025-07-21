#include "rkllm_proxy.h"
#include "rkllm_auto_generated.h"
#include "../../common/string_utils/string_utils.h"
#include "../../external/rkllm/rkllm.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Global state
static bool g_proxy_initialized = false;
static LLMHandle g_global_handle = NULL;

// RKLLM Function Registry - All available RKLLM functions
static rkllm_function_desc_t g_rkllm_functions[] = {
    {
        .name = "rkllm_createDefaultParam",
        .function_ptr = rkllm_createDefaultParam,
        .return_type = RKLLM_RETURN_RKLLM_PARAM,
        .param_count = 0,
        .params = {},
        .description = "Creates a default RKLLMParam structure with preset values"
    },
    {
        .name = "rkllm_init",
        .function_ptr = rkllm_init,
        .return_type = RKLLM_RETURN_INT,
        .param_count = 3,
        .params = {
            {RKLLM_PARAM_HANDLE_PTR, "handle", true},
            {RKLLM_PARAM_RKLLM_PARAM_PTR, "param", false},
            {RKLLM_PARAM_CALLBACK, "callback", false}
        },
        .description = "Initializes the LLM with the given parameters"
    },
    {
        .name = "rkllm_load_lora",
        .function_ptr = rkllm_load_lora,
        .return_type = RKLLM_RETURN_INT,
        .param_count = 2,
        .params = {
            {RKLLM_PARAM_HANDLE, "handle", false},
            {RKLLM_PARAM_RKLLM_LORA_ADAPTER_PTR, "lora_adapter", false}
        },
        .description = "Loads a Lora adapter into the LLM"
    },
    {
        .name = "rkllm_load_prompt_cache",
        .function_ptr = rkllm_load_prompt_cache,
        .return_type = RKLLM_RETURN_INT,
        .param_count = 2,
        .params = {
            {RKLLM_PARAM_HANDLE, "handle", false},
            {RKLLM_PARAM_STRING, "prompt_cache_path", false}
        },
        .description = "Loads a prompt cache from a file"
    },
    {
        .name = "rkllm_release_prompt_cache",
        .function_ptr = rkllm_release_prompt_cache,
        .return_type = RKLLM_RETURN_INT,
        .param_count = 1,
        .params = {
            {RKLLM_PARAM_HANDLE, "handle", false}
        },
        .description = "Releases the prompt cache from memory"
    },
    {
        .name = "rkllm_destroy",
        .function_ptr = rkllm_destroy,
        .return_type = RKLLM_RETURN_INT,
        .param_count = 1,
        .params = {
            {RKLLM_PARAM_HANDLE, "handle", false}
        },
        .description = "Destroys the LLM instance and releases resources"
    },
    {
        .name = "rkllm_run",
        .function_ptr = rkllm_run,
        .return_type = RKLLM_RETURN_INT,
        .param_count = 4,
        .params = {
            {RKLLM_PARAM_HANDLE, "handle", false},
            {RKLLM_PARAM_RKLLM_INPUT_PTR, "rkllm_input", false},
            {RKLLM_PARAM_RKLLM_INFER_PARAM_PTR, "rkllm_infer_params", false},
            {RKLLM_PARAM_VOID_PTR, "userdata", false}
        },
        .description = "Runs an LLM inference task synchronously"
    },
    {
        .name = "rkllm_run_async",
        .function_ptr = rkllm_run_async,
        .return_type = RKLLM_RETURN_INT,
        .param_count = 4,
        .params = {
            {RKLLM_PARAM_HANDLE, "handle", false},
            {RKLLM_PARAM_RKLLM_INPUT_PTR, "rkllm_input", false},
            {RKLLM_PARAM_RKLLM_INFER_PARAM_PTR, "rkllm_infer_params", false},
            {RKLLM_PARAM_VOID_PTR, "userdata", false}
        },
        .description = "Runs an LLM inference task asynchronously"
    },
    {
        .name = "rkllm_abort",
        .function_ptr = rkllm_abort,
        .return_type = RKLLM_RETURN_INT,
        .param_count = 1,
        .params = {
            {RKLLM_PARAM_HANDLE, "handle", false}
        },
        .description = "Aborts an ongoing LLM task"
    },
    {
        .name = "rkllm_is_running",
        .function_ptr = rkllm_is_running,
        .return_type = RKLLM_RETURN_INT,
        .param_count = 1,
        .params = {
            {RKLLM_PARAM_HANDLE, "handle", false}
        },
        .description = "Checks if an LLM task is currently running"
    },
    {
        .name = "rkllm_clear_kv_cache",
        .function_ptr = rkllm_clear_kv_cache,
        .return_type = RKLLM_RETURN_INT,
        .param_count = 4,
        .params = {
            {RKLLM_PARAM_HANDLE, "handle", false},
            {RKLLM_PARAM_INT, "keep_system_prompt", false},
            {RKLLM_PARAM_INT_PTR, "start_pos", false},
            {RKLLM_PARAM_INT_PTR, "end_pos", false}
        },
        .description = "Clear the key-value cache for a given LLM handle"
    },
    {
        .name = "rkllm_get_kv_cache_size",
        .function_ptr = rkllm_get_kv_cache_size,
        .return_type = RKLLM_RETURN_INT,
        .param_count = 2,
        .params = {
            {RKLLM_PARAM_HANDLE, "handle", false},
            {RKLLM_PARAM_INT_PTR, "cache_sizes", true}
        },
        .description = "Get the current size of the key-value cache"
    },
    {
        .name = "rkllm_set_chat_template",
        .function_ptr = rkllm_set_chat_template,
        .return_type = RKLLM_RETURN_INT,
        .param_count = 4,
        .params = {
            {RKLLM_PARAM_HANDLE, "handle", false},
            {RKLLM_PARAM_STRING, "system_prompt", false},
            {RKLLM_PARAM_STRING, "prompt_prefix", false},
            {RKLLM_PARAM_STRING, "prompt_postfix", false}
        },
        .description = "Sets the chat template for the LLM"
    },
    {
        .name = "rkllm_set_function_tools",
        .function_ptr = rkllm_set_function_tools,
        .return_type = RKLLM_RETURN_INT,
        .param_count = 4,
        .params = {
            {RKLLM_PARAM_HANDLE, "handle", false},
            {RKLLM_PARAM_STRING, "system_prompt", false},
            {RKLLM_PARAM_STRING, "tools", false},
            {RKLLM_PARAM_STRING, "tool_response_str", false}
        },
        .description = "Sets the function calling configuration for the LLM"
    },
    {
        .name = "rkllm_set_cross_attn_params",
        .function_ptr = rkllm_set_cross_attn_params,
        .return_type = RKLLM_RETURN_INT,
        .param_count = 2,
        .params = {
            {RKLLM_PARAM_HANDLE, "handle", false},
            {RKLLM_PARAM_RKLLM_CROSS_ATTN_PARAM_PTR, "cross_attn_params", false}
        },
        .description = "Sets the cross-attention parameters for the LLM decoder"
    },
    {
        .name = "rkllm_get_constants",
        .function_ptr = NULL, // Special handler - not a real RKLLM function
        .return_type = RKLLM_RETURN_JSON,
        .param_count = 0,
        .params = {},
        .description = "Get all RKLLM constants, enums, and defines"
    }
};

static const int g_function_count = sizeof(g_rkllm_functions) / sizeof(g_rkllm_functions[0]);

// Helper function to find function descriptor by name
static rkllm_function_desc_t* find_function(const char* name) {
    for (int i = 0; i < g_function_count; i++) {
        if (strcmp(g_rkllm_functions[i].name, name) == 0) {
            return &g_rkllm_functions[i];
        }
    }
    return NULL;
}

// Convert JSON parameter to RKLLM structure
int rkllm_proxy_convert_param(json_object* json_value, rkllm_param_type_t param_type, 
                              void* output, size_t output_size) {
    if (!json_value || !output) {
        return -1;
    }
    
    switch (param_type) {
        case RKLLM_PARAM_HANDLE:
            // Use global handle
            *(LLMHandle*)output = g_global_handle;
            return 0;
            
        case RKLLM_PARAM_HANDLE_PTR:
            // Pointer to global handle
            *(LLMHandle**)output = &g_global_handle;
            return 0;
            
        case RKLLM_PARAM_STRING:
            if (json_object_get_type(json_value) == json_type_string) {
                *(const char**)output = json_object_get_string(json_value);
                return 0;
            }
            return -1;
            
        case RKLLM_PARAM_INT:
            if (json_object_get_type(json_value) == json_type_int) {
                *(int*)output = json_object_get_int(json_value);
                return 0;
            }
            return -1;
            
        case RKLLM_PARAM_BOOL:
            if (json_object_get_type(json_value) == json_type_boolean) {
                *(bool*)output = json_object_get_boolean(json_value);
                return 0;
            }
            return -1;
            
        case RKLLM_PARAM_FLOAT:
            if (json_object_get_type(json_value) == json_type_double) {
                *(float*)output = (float)json_object_get_double(json_value);
                return 0;
            }
            return -1;
            
        case RKLLM_PARAM_RKLLM_PARAM_PTR: {
            // Convert JSON object to RKLLMParam
            if (json_object_get_type(json_value) != json_type_object) {
                return -1;
            }
            
            RKLLMParam* param = (RKLLMParam*)output;
            *param = rkllm_createDefaultParam();
            
            // Extract fields from JSON
            json_object* model_path_obj;
            if (json_object_object_get_ex(json_value, "model_path", &model_path_obj)) {
                param->model_path = json_object_get_string(model_path_obj);
            }
            
            json_object* max_tokens_obj;
            if (json_object_object_get_ex(json_value, "max_new_tokens", &max_tokens_obj)) {
                param->max_new_tokens = json_object_get_int(max_tokens_obj);
            }
            
            json_object* temp_obj;
            if (json_object_object_get_ex(json_value, "temperature", &temp_obj)) {
                param->temperature = (float)json_object_get_double(temp_obj);
            }
            
            json_object* top_k_obj;
            if (json_object_object_get_ex(json_value, "top_k", &top_k_obj)) {
                param->top_k = json_object_get_int(top_k_obj);
            }
            
            json_object* top_p_obj;
            if (json_object_object_get_ex(json_value, "top_p", &top_p_obj)) {
                param->top_p = (float)json_object_get_double(top_p_obj);
            }
            
            return 0;
        }
        
        case RKLLM_PARAM_RKLLM_INPUT_PTR: {
            // Convert JSON object to RKLLMInput
            if (json_object_get_type(json_value) != json_type_object) {
                return -1;
            }
            
            RKLLMInput* input = (RKLLMInput*)output;
            memset(input, 0, sizeof(RKLLMInput));
            
            // Default to prompt input
            input->input_type = RKLLM_INPUT_PROMPT;
            
            json_object* prompt_obj;
            if (json_object_object_get_ex(json_value, "prompt", &prompt_obj)) {
                input->prompt_input = json_object_get_string(prompt_obj);
            }
            
            json_object* role_obj;
            if (json_object_object_get_ex(json_value, "role", &role_obj)) {
                input->role = json_object_get_string(role_obj);
            }
            
            json_object* thinking_obj;
            if (json_object_object_get_ex(json_value, "enable_thinking", &thinking_obj)) {
                input->enable_thinking = json_object_get_boolean(thinking_obj);
            }
            
            return 0;
        }
        
        case RKLLM_PARAM_RKLLM_INFER_PARAM_PTR: {
            // Convert JSON object to RKLLMInferParam
            if (json_object_get_type(json_value) != json_type_object) {
                return -1;
            }
            
            RKLLMInferParam* infer_param = (RKLLMInferParam*)output;
            memset(infer_param, 0, sizeof(RKLLMInferParam));
            
            // Default mode
            infer_param->mode = RKLLM_INFER_GENERATE;
            infer_param->keep_history = 1;
            
            json_object* mode_obj;
            if (json_object_object_get_ex(json_value, "mode", &mode_obj)) {
                infer_param->mode = json_object_get_int(mode_obj);
            }
            
            json_object* keep_history_obj;
            if (json_object_object_get_ex(json_value, "keep_history", &keep_history_obj)) {
                infer_param->keep_history = json_object_get_int(keep_history_obj);
            }
            
            return 0;
        }
        
        case RKLLM_PARAM_CALLBACK:
            // For callbacks, we'll use NULL for now (can be extended later)
            *(LLMResultCallback*)output = NULL;
            return 0;
            
        case RKLLM_PARAM_VOID_PTR:
            // For userdata, use NULL
            *(void**)output = NULL;
            return 0;
            
        case RKLLM_PARAM_INT_PTR:
            // For int pointers, allocate buffer or use NULL
            *(int**)output = NULL;
            return 0;
            
        default:
            return -1;
    }
}

// Convert RKLLM result to JSON
int rkllm_proxy_convert_result(rkllm_return_type_t return_type, void* result_data, char** result_json) {
    if (!result_json) {
        return -1;
    }
    
    json_object* result_obj = json_object_new_object();
    
    switch (return_type) {
        case RKLLM_RETURN_INT: {
            int status = *(int*)result_data;
            json_object_object_add(result_obj, "status", json_object_new_int(status));
            json_object_object_add(result_obj, "success", json_object_new_boolean(status == 0));
            if (status == 0) {
                json_object_object_add(result_obj, "message", json_object_new_string("Operation completed successfully"));
            } else {
                json_object_object_add(result_obj, "message", json_object_new_string("Operation failed"));
            }
            break;
        }
        
        case RKLLM_RETURN_VOID:
            json_object_object_add(result_obj, "success", json_object_new_boolean(true));
            json_object_object_add(result_obj, "message", json_object_new_string("Operation completed"));
            break;
            
        case RKLLM_RETURN_RKLLM_PARAM: {
            // For createDefaultParam, return the parameter structure as JSON
            RKLLMParam* param = (RKLLMParam*)result_data;
            json_object* param_obj = json_object_new_object();
            
            json_object_object_add(param_obj, "max_context_len", json_object_new_int(param->max_context_len));
            json_object_object_add(param_obj, "max_new_tokens", json_object_new_int(param->max_new_tokens));
            json_object_object_add(param_obj, "top_k", json_object_new_int(param->top_k));
            json_object_object_add(param_obj, "top_p", json_object_new_double(param->top_p));
            json_object_object_add(param_obj, "temperature", json_object_new_double(param->temperature));
            json_object_object_add(param_obj, "repeat_penalty", json_object_new_double(param->repeat_penalty));
            
            json_object_object_add(result_obj, "success", json_object_new_boolean(true));
            json_object_object_add(result_obj, "default_params", param_obj);
            break;
        }
        
        case RKLLM_RETURN_JSON:
            // For JSON return types (like constants), the result_data is already a JSON string
            json_object_put(result_obj);  // Don't need wrapper object
            *result_json = strdup((char*)result_data);
            return 0;
    }
    
    const char* json_str = json_object_to_json_string(result_obj);
    *result_json = strdup(json_str);
    json_object_put(result_obj);
    
    return 0;
}

// Initialize the dynamic RKLLM proxy system
int rkllm_proxy_init(void) {
    if (g_proxy_initialized) {
        return 0;
    }
    
    g_proxy_initialized = true;
    printf("✅ RKLLM proxy initialized with %d functions (direct calls)\n", g_function_count);
    return 0;
}

// Shutdown the dynamic RKLLM proxy system
void rkllm_proxy_shutdown(void) {
    if (!g_proxy_initialized) {
        return;
    }
    
    g_proxy_initialized = false;
    g_global_handle = NULL;
    printf("✅ RKLLM proxy shutdown\n");
}

// Get all RKLLM constants and enums
int rkllm_proxy_get_constants(char** result_json) {
    // Use the auto-generated comprehensive constants function
    return rkllm_get_constants_auto(result_json);
}

// Process RKLLM function call dynamically
int rkllm_proxy_call(const char* function_name, const char* params_json, char** result_json) {
    if (!g_proxy_initialized || !function_name || !result_json) {
        return -1;
    }
    
    // Special handler for constants function
    if (strcmp(function_name, "rkllm_get_constants") == 0) {
        return rkllm_proxy_get_constants(result_json);
    }
    
    // Find function descriptor
    rkllm_function_desc_t* func_desc = find_function(function_name);
    if (!func_desc) {
        char error_msg[256];
        snprintf(error_msg, sizeof(error_msg), 
                 "{\"error\": \"Function not found\", \"function\": \"%s\"}", function_name);
        *result_json = strdup(error_msg);
        return -1;
    }
    
    // Parse parameters JSON
    json_object* params = NULL;
    if (params_json && strlen(params_json) > 0) {
        params = json_tokener_parse(params_json);
        if (!params) {
            *result_json = strdup("{\"error\": \"Invalid parameters JSON\"}");
            return -1;
        }
    }
    
    // Prepare function arguments
    void* args[8] = {0};  // Max 8 arguments
    char arg_buffers[8][1024] = {0};  // Buffers for complex arguments
    
    for (int i = 0; i < func_desc->param_count; i++) {
        json_object* param_value = NULL;
        if (params) {
            json_object_object_get_ex(params, func_desc->params[i].name, &param_value);
        }
        
        if (rkllm_proxy_convert_param(param_value, func_desc->params[i].type, 
                                      arg_buffers[i], sizeof(arg_buffers[i])) != 0) {
            if (params) json_object_put(params);
            *result_json = strdup("{\"error\": \"Parameter conversion failed\"}");
            return -1;
        }
        
        args[i] = arg_buffers[i];
    }
    
    // Call function dynamically based on parameter count and return type
    int status = 0;
    void* result_data = &status;
    
    switch (func_desc->param_count) {
        case 0: {
            if (func_desc->return_type == RKLLM_RETURN_RKLLM_PARAM) {
                RKLLMParam (*func0)() = (RKLLMParam (*)())func_desc->function_ptr;
                static RKLLMParam param_result;
                param_result = func0();
                result_data = &param_result;
            } else {
                int (*func0)() = (int (*)())func_desc->function_ptr;
                status = func0();
            }
            break;
        }
        case 1: {
            int (*func1)(void*) = (int (*)(void*))func_desc->function_ptr;
            status = func1(args[0] ? *(void**)args[0] : NULL);
            break;
        }
        case 2: {
            int (*func2)(void*, void*) = (int (*)(void*, void*))func_desc->function_ptr;
            status = func2(args[0] ? *(void**)args[0] : NULL, 
                          args[1] ? *(void**)args[1] : NULL);
            break;
        }
        case 3: {
            int (*func3)(void*, void*, void*) = (int (*)(void*, void*, void*))func_desc->function_ptr;
            status = func3(args[0] ? *(void**)args[0] : NULL,
                          args[1] ? *(void**)args[1] : NULL,
                          args[2] ? *(void**)args[2] : NULL);
            break;
        }
        case 4: {
            int (*func4)(void*, void*, void*, void*) = (int (*)(void*, void*, void*, void*))func_desc->function_ptr;
            status = func4(args[0] ? *(void**)args[0] : NULL,
                          args[1] ? *(void**)args[1] : NULL,
                          args[2] ? *(void**)args[2] : NULL,
                          args[3] ? *(void**)args[3] : NULL);
            break;
        }
        default:
            if (params) json_object_put(params);
            *result_json = strdup("{\"error\": \"Unsupported parameter count\"}");
            return -1;
    }
    
    // Special handling for rkllm_init - save the handle
    if (strcmp(function_name, "rkllm_init") == 0 && status == 0) {
        // The handle was set via the pointer in args[0]
        printf("🔧 RKLLM handle initialized and saved globally\n");
    }
    
    // Special handling for rkllm_destroy - clear the handle
    if (strcmp(function_name, "rkllm_destroy") == 0 && status == 0) {
        g_global_handle = NULL;
        printf("🧹 RKLLM handle destroyed and cleared globally\n");
    }
    
    // Convert result to JSON
    int convert_result = rkllm_proxy_convert_result(func_desc->return_type, result_data, result_json);
    
    if (params) json_object_put(params);
    return convert_result;
}

// Get list of available RKLLM functions
int rkllm_proxy_get_functions(char** functions_json) {
    if (!functions_json) {
        return -1;
    }
    
    json_object* functions_array = json_object_new_array();
    
    for (int i = 0; i < g_function_count; i++) {
        json_object* func_obj = json_object_new_object();
        json_object_object_add(func_obj, "name", json_object_new_string(g_rkllm_functions[i].name));
        json_object_object_add(func_obj, "description", json_object_new_string(g_rkllm_functions[i].description));
        
        // Add parameters
        json_object* params_array = json_object_new_array();
        for (int j = 0; j < g_rkllm_functions[i].param_count; j++) {
            json_object* param_obj = json_object_new_object();
            json_object_object_add(param_obj, "name", json_object_new_string(g_rkllm_functions[i].params[j].name));
            json_object_object_add(param_obj, "type", json_object_new_int(g_rkllm_functions[i].params[j].type));
            json_object_object_add(param_obj, "is_output", json_object_new_boolean(g_rkllm_functions[i].params[j].is_output));
            json_object_array_add(params_array, param_obj);
        }
        json_object_object_add(func_obj, "parameters", params_array);
        
        json_object_array_add(functions_array, func_obj);
    }
    
    const char* json_str = json_object_to_json_string(functions_array);
    *functions_json = strdup(json_str);
    json_object_put(functions_array);
    
    return 0;
}

// Get global RKLLM handle
LLMHandle rkllm_proxy_get_handle(void) {
    return g_global_handle;
}

// Set global RKLLM handle
void rkllm_proxy_set_handle(LLMHandle handle) {
    g_global_handle = handle;
}