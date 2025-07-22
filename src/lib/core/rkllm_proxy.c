#include "rkllm_proxy.h"
#include "rkllm_auto_generated.h"
#include "rkllm_array_utils.h"
#include "rkllm_error_mapping.h"
#include "rkllm_streaming_context.h"
#include "settings_global.h"
#include "../../common/string_utils/string_utils.h"
#include "../../external/rkllm/rkllm.h"
// Removed unused streaming.h include
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <errno.h>
#include <math.h>

// Forward declarations for debug functions (not in public header)
extern int rkllm_accuracy_analysis(LLMHandle handle);
extern int rkllm_get_timings(LLMHandle handle);
extern void rkllm_print_timings(LLMHandle handle);
extern void rkllm_print_memorys(LLMHandle handle);

// Global state
static bool g_proxy_initialized = false;
static LLMHandle g_global_handle = NULL;

// Callback state for capturing model responses
static char* g_response_buffer = NULL;
static size_t g_response_buffer_size = 0;
static bool g_response_ready = false;
static LLMCallState g_last_call_state = RKLLM_RUN_NORMAL;

// Global streaming session tracking (Phase 3.1 from fix_rkllm_proxy.md)
static char g_current_request_id[64] = {0};
static bool g_streaming_active = false;

// RKLLM Callback function to capture model responses
static int rkllm_proxy_callback(RKLLMResult* result, void* userdata __attribute__((unused)), LLMCallState state) {
    printf("[CALLBACK] State: %d, Result ptr: %p\n", state, result);
    fflush(stdout);
    
    if (!result) {
        printf("[CALLBACK] RKLLM callback: NULL result pointer\n");
        fflush(stdout);
        return 0;
    }
    
    printf("[CALLBACK] Text ptr: %p, Text len: %zu\n", 
           result->text, result->text ? strlen(result->text) : 0);
    fflush(stdout);
           
    if (result->text && strlen(result->text) > 0) {
        printf("[CALLBACK] Text content: '%.100s'%s\n", result->text, 
               strlen(result->text) > 100 ? "..." : "");
        
        // Existing local buffer logic (keep for compatibility)
        size_t current_len = strlen(g_response_buffer);
        size_t text_len = strlen(result->text);
        printf("[CALLBACK] Buffer state: current_len=%zu, text_len=%zu, buffer_size=%zu\n", 
               current_len, text_len, g_response_buffer_size);
        
        if (current_len + text_len < g_response_buffer_size - 1) {
            strcat(g_response_buffer, result->text);
            printf("[CALLBACK] Text appended to buffer successfully\n");
        } else {
            printf("[CALLBACK] WARNING: Buffer overflow prevented, text truncated\n");
        }
        
        // TODO: Implement proper streaming integration with transport layer
        if (g_streaming_active && g_current_request_id[0] != '\0') {
            bool is_final = (state == RKLLM_RUN_FINISH || state == RKLLM_RUN_ERROR);
            
            printf("[CALLBACK] Streaming session active: request_id=%s, delta='%.50s', is_final=%s\n",
                   g_current_request_id, result->text, is_final ? "true" : "false");
            printf("[CALLBACK] TODO: Forward to actual transport streaming system\n");
        }
    } else {
        printf("[CALLBACK] No text content in result (text=%p, len=%zu)\n", 
               result->text, result->text ? strlen(result->text) : 0);
    }
    
    if (result->token_id != 0) {
        printf("[CALLBACK] Token ID: %d\n", result->token_id);
    }
    
    // Store the call state
    g_last_call_state = state;
    
    if (state == RKLLM_RUN_FINISH || state == RKLLM_RUN_ERROR) {
        printf("[CALLBACK] Inference finished (state=%s). Final buffer content: '%.200s'%s\n", 
               (state == RKLLM_RUN_FINISH) ? "FINISH" : "ERROR",
               g_response_buffer, strlen(g_response_buffer) > 200 ? "..." : "");
        printf("[CALLBACK] Setting response ready flag to true\n");
        g_response_ready = true;
        
        // Clean up streaming session on completion
        if (g_streaming_active) {
            printf("[CALLBACK] Cleaning up streaming session: %s\n", g_current_request_id);
            g_streaming_active = false;
            memset(g_current_request_id, 0, sizeof(g_current_request_id));
        }
    }
    
    fflush(stdout);
    return 0; // Continue inference
}

// Phase 3.1: Streaming Session Management Functions
int rkllm_proxy_start_streaming_session(const char* request_id, const char* method) {
    if (!request_id) return -1;
    
    // Generate unique stream ID based on request ID
    snprintf(g_current_request_id, sizeof(g_current_request_id), "stream_%s", request_id);
    g_streaming_active = true;
    
    printf("[STREAMING] Started streaming session: request_id=%s\\n", 
           g_current_request_id);
    
    // TODO: Create actual streaming session integration with transports
    printf("[STREAMING] Placeholder: Would create streaming session for method=%s\\n", method);
    return 0;
}

void rkllm_proxy_stop_streaming_session(void) {
    if (g_streaming_active) {
        printf("[STREAMING] Stopping streaming session: %s\\n", g_current_request_id);
        g_streaming_active = false;
        memset(g_current_request_id, 0, sizeof(g_current_request_id));
    }
}

bool rkllm_proxy_is_streaming_active(void) {
    return g_streaming_active;
}

const char* rkllm_proxy_get_current_request_id(void) {
    return g_streaming_active ? g_current_request_id : NULL;
}

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
        .name = "rkllm_accuracy_analysis",
        .function_ptr = rkllm_accuracy_analysis,
        .return_type = RKLLM_RETURN_INT,
        .param_count = 1,
        .params = {
            {RKLLM_PARAM_HANDLE, "handle", false}
        },
        .description = "Performs accuracy analysis on the model"
    },
    {
        .name = "rkllm_get_timings",
        .function_ptr = rkllm_get_timings,
        .return_type = RKLLM_RETURN_INT,
        .param_count = 1,
        .params = {
            {RKLLM_PARAM_HANDLE, "handle", false}
        },
        .description = "Gets performance timing data from the model"
    },
    {
        .name = "rkllm_print_timings",
        .function_ptr = rkllm_print_timings,
        .return_type = RKLLM_RETURN_VOID,
        .param_count = 1,
        .params = {
            {RKLLM_PARAM_HANDLE, "handle", false}
        },
        .description = "Prints timing information to console"
    },
    {
        .name = "rkllm_print_memorys",
        .function_ptr = rkllm_print_memorys,
        .return_type = RKLLM_RETURN_VOID,
        .param_count = 1,
        .params = {
            {RKLLM_PARAM_HANDLE, "handle", false}
        },
        .description = "Prints memory usage information to console"
    },
    {
        .name = "rkllm_get_functions",
        .function_ptr = NULL, // Special handler - meta function
        .return_type = RKLLM_RETURN_JSON,
        .param_count = 0,
        .params = {},
        .description = "Get list of all available RKLLM functions"
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
                              void* output, size_t output_size __attribute__((unused))) {
    if (!output) {
        return -1;
    }
    
    // Handle null JSON values for most parameter types
    if (!json_value || json_object_get_type(json_value) == json_type_null) {
        switch (param_type) {
            case RKLLM_PARAM_HANDLE:
                *(LLMHandle*)output = g_global_handle;
                return 0;
            case RKLLM_PARAM_HANDLE_PTR:
                *(LLMHandle**)output = &g_global_handle;
                return 0;
            case RKLLM_PARAM_STRING:
                *(const char**)output = NULL;
                return 0;
            case RKLLM_PARAM_CALLBACK:
                *(LLMResultCallback*)output = rkllm_proxy_callback;
                return 0;
            case RKLLM_PARAM_VOID_PTR:
                *(void**)output = NULL;
                return 0;
            case RKLLM_PARAM_RKLLM_PARAM_PTR:
                *(RKLLMParam**)output = NULL;
                return 0;
            default:
                // For other types, continue with normal processing
                break;
        }
    }
    
    switch (param_type) {
        case RKLLM_PARAM_HANDLE:
            // Use global handle for any JSON value (including null)
            // In JSON-RPC, we always use the global handle maintained by the proxy
            *(LLMHandle*)output = g_global_handle;
            return 0;
            
        case RKLLM_PARAM_HANDLE_PTR:
            // Pointer to global handle
            *(LLMHandle**)output = &g_global_handle;
            return 0;
            
        case RKLLM_PARAM_STRING:
            if (!json_value || json_object_get_type(json_value) == json_type_null) {
                *(const char**)output = NULL;
                return 0;
            }
            if (json_object_get_type(json_value) == json_type_string) {
                *(const char**)output = json_object_get_string(json_value);
                return 0;
            }
            return -1;
            
        case RKLLM_PARAM_INT:
            if (!json_value || json_object_get_type(json_value) == json_type_null) {
                *(int*)output = 0;
                return 0;
            }
            if (json_object_get_type(json_value) == json_type_int) {
                *(int*)output = json_object_get_int(json_value);
                return 0;
            }
            return -1;
            
        case RKLLM_PARAM_BOOL:
            if (!json_value || json_object_get_type(json_value) == json_type_null) {
                *(bool*)output = false;
                return 0;
            }
            if (json_object_get_type(json_value) == json_type_boolean) {
                *(bool*)output = json_object_get_boolean(json_value);
                return 0;
            }
            return -1;
            
        case RKLLM_PARAM_FLOAT:
            if (!json_value || json_object_get_type(json_value) == json_type_null) {
                *(float*)output = 0.0f;
                return 0;
            }
            if (json_object_get_type(json_value) == json_type_double) {
                *(float*)output = (float)json_object_get_double(json_value);
                return 0;
            }
            return -1;
            
        case RKLLM_PARAM_RKLLM_PARAM_PTR: {
            // Convert JSON object to RKLLMParam with priority: user params > settings defaults > hardcoded defaults
            if (!json_value || json_object_get_type(json_value) == json_type_null) {
                *(RKLLMParam**)output = NULL;
                return 0;
            }
            if (json_object_get_type(json_value) != json_type_object) {
                return -1;
            }
            
            RKLLMParam* param = (RKLLMParam*)output;
            
            // Start with hardcoded defaults from RKLLM library
            *param = rkllm_createDefaultParam();
            
            // Apply settings defaults (from settings.json) if available
            // These override the hardcoded defaults but are overridden by user params
            if (param->model_path == NULL) {
                param->model_path = SETTING_RKLLM(default_model_path);
            }
            if (param->max_context_len == 512) {  // Default from rkllm_createDefaultParam
                int setting_val = SETTING_RKLLM(max_context_len);
                if (setting_val > 0) param->max_context_len = setting_val;
            }
            if (param->max_new_tokens == 256) {  // Default from rkllm_createDefaultParam
                int setting_val = SETTING_RKLLM(max_new_tokens);
                if (setting_val > 0) param->max_new_tokens = setting_val;
            }
            if (param->top_k == 40) {  // Default from rkllm_createDefaultParam
                int setting_val = SETTING_RKLLM(top_k);
                if (setting_val > 0) param->top_k = setting_val;
            }
            if (param->n_keep == 0) {  // Default from rkllm_createDefaultParam
                int setting_val = SETTING_RKLLM(n_keep);
                if (setting_val >= 0) param->n_keep = setting_val;
            }
            if (fabsf(param->top_p - 0.9f) < 0.001f) {  // Default from rkllm_createDefaultParam
                float setting_val = SETTING_RKLLM(top_p);
                if (setting_val > 0.0f) param->top_p = setting_val;
            }
            if (fabsf(param->temperature - 0.8f) < 0.001f) {  // Default from rkllm_createDefaultParam
                float setting_val = SETTING_RKLLM(temperature);
                if (setting_val > 0.0f) param->temperature = setting_val;
            }
            if (fabsf(param->repeat_penalty - 1.1f) < 0.001f) {  // Default from rkllm_createDefaultParam
                float setting_val = SETTING_RKLLM(repeat_penalty);
                if (setting_val > 0.0f) param->repeat_penalty = setting_val;
            }
            if (fabsf(param->frequency_penalty - 0.0f) < 0.001f) {  // Default from rkllm_createDefaultParam
                float setting_val = SETTING_RKLLM(frequency_penalty);
                if (setting_val >= 0.0f) param->frequency_penalty = setting_val;
            }
            if (fabsf(param->presence_penalty - 0.0f) < 0.001f) {  // Default from rkllm_createDefaultParam
                float setting_val = SETTING_RKLLM(presence_penalty);
                if (setting_val >= 0.0f) param->presence_penalty = setting_val;
            }
            if (param->mirostat == 0) {  // Default from rkllm_createDefaultParam
                int setting_val = SETTING_RKLLM(mirostat);
                if (setting_val >= 0) param->mirostat = setting_val;
            }
            if (fabsf(param->mirostat_tau - 5.0f) < 0.001f) {  // Default from rkllm_createDefaultParam
                float setting_val = SETTING_RKLLM(mirostat_tau);
                if (setting_val > 0.0f) param->mirostat_tau = setting_val;
            }
            if (fabsf(param->mirostat_eta - 0.1f) < 0.001f) {  // Default from rkllm_createDefaultParam
                float setting_val = SETTING_RKLLM(mirostat_eta);
                if (setting_val > 0.0f) param->mirostat_eta = setting_val;
            }
            if (param->skip_special_token == false) {  // Default from rkllm_createDefaultParam
                bool setting_val = SETTING_RKLLM(skip_special_token);
                param->skip_special_token = setting_val;
            }
            if (param->is_async == false) {  // Default from rkllm_createDefaultParam
                bool setting_val = SETTING_RKLLM(is_async);
                param->is_async = setting_val;
            }
            
            // Apply extend_param settings defaults
            if (param->extend_param.base_domain_id == 0) {
                int setting_val = SETTINGS() ? SETTINGS()->rkllm.extend.base_domain_id : 0;
                if (setting_val >= 0) param->extend_param.base_domain_id = setting_val;
            }
            if (param->extend_param.embed_flash == 0) {
                int8_t setting_val = SETTINGS() ? SETTINGS()->rkllm.extend.embed_flash : 0;
                if (setting_val >= 0) param->extend_param.embed_flash = setting_val;
            }
            if (param->extend_param.enabled_cpus_num == 4) {  // Default assumption
                int8_t setting_val = SETTINGS() ? SETTINGS()->rkllm.extend.enabled_cpus_num : 0;
                if (setting_val > 0) param->extend_param.enabled_cpus_num = setting_val;
            }
            if (param->extend_param.enabled_cpus_mask == 0xF0) {  // Default assumption
                uint32_t setting_val = SETTINGS() ? SETTINGS()->rkllm.extend.enabled_cpus_mask : 0;
                if (setting_val > 0) param->extend_param.enabled_cpus_mask = setting_val;
            }
            // Ensure n_batch is always at least 1 (RKLLM requirement)
            if (param->extend_param.n_batch < 1) {
                uint8_t setting_val = SETTINGS() ? SETTINGS()->rkllm.extend.n_batch : 1;
                param->extend_param.n_batch = (setting_val > 0) ? setting_val : 1;
            } else if (param->extend_param.n_batch == 1) {  // Apply settings override
                uint8_t setting_val = SETTINGS() ? SETTINGS()->rkllm.extend.n_batch : 0;
                if (setting_val > 0) param->extend_param.n_batch = setting_val;
            }
            if (param->extend_param.use_cross_attn == 0) {
                int8_t setting_val = SETTINGS() ? SETTINGS()->rkllm.extend.use_cross_attn : 0;
                if (setting_val >= 0) param->extend_param.use_cross_attn = setting_val;
            }
            
            // Finally, apply user-provided parameters (highest priority)
            // These override both hardcoded defaults and settings defaults
            json_object* obj;
            
            if (json_object_object_get_ex(json_value, "model_path", &obj)) {
                param->model_path = json_object_get_string(obj);
            }
            if (json_object_object_get_ex(json_value, "max_context_len", &obj)) {
                param->max_context_len = json_object_get_int(obj);
            }
            if (json_object_object_get_ex(json_value, "max_new_tokens", &obj)) {
                param->max_new_tokens = json_object_get_int(obj);
            }
            if (json_object_object_get_ex(json_value, "top_k", &obj)) {
                param->top_k = json_object_get_int(obj);
            }
            if (json_object_object_get_ex(json_value, "n_keep", &obj)) {
                param->n_keep = json_object_get_int(obj);
            }
            if (json_object_object_get_ex(json_value, "top_p", &obj)) {
                param->top_p = (float)json_object_get_double(obj);
            }
            if (json_object_object_get_ex(json_value, "temperature", &obj)) {
                param->temperature = (float)json_object_get_double(obj);
            }
            if (json_object_object_get_ex(json_value, "repeat_penalty", &obj)) {
                param->repeat_penalty = (float)json_object_get_double(obj);
            }
            if (json_object_object_get_ex(json_value, "frequency_penalty", &obj)) {
                param->frequency_penalty = (float)json_object_get_double(obj);
            }
            if (json_object_object_get_ex(json_value, "presence_penalty", &obj)) {
                param->presence_penalty = (float)json_object_get_double(obj);
            }
            if (json_object_object_get_ex(json_value, "mirostat", &obj)) {
                param->mirostat = json_object_get_int(obj);
            }
            if (json_object_object_get_ex(json_value, "mirostat_tau", &obj)) {
                param->mirostat_tau = (float)json_object_get_double(obj);
            }
            if (json_object_object_get_ex(json_value, "mirostat_eta", &obj)) {
                param->mirostat_eta = (float)json_object_get_double(obj);
            }
            if (json_object_object_get_ex(json_value, "skip_special_token", &obj)) {
                param->skip_special_token = json_object_get_boolean(obj);
            }
            if (json_object_object_get_ex(json_value, "is_async", &obj)) {
                param->is_async = json_object_get_boolean(obj);
            }
            if (json_object_object_get_ex(json_value, "img_start", &obj)) {
                param->img_start = json_object_get_string(obj);
            }
            if (json_object_object_get_ex(json_value, "img_end", &obj)) {
                param->img_end = json_object_get_string(obj);
            }
            if (json_object_object_get_ex(json_value, "img_content", &obj)) {
                param->img_content = json_object_get_string(obj);
            }
            
            // Handle extend_param nested object (user overrides)
            if (json_object_object_get_ex(json_value, "extend_param", &obj)) {
                json_object* extend_obj;
                if (json_object_object_get_ex(obj, "base_domain_id", &extend_obj)) {
                    param->extend_param.base_domain_id = json_object_get_int(extend_obj);
                }
                if (json_object_object_get_ex(obj, "embed_flash", &extend_obj)) {
                    param->extend_param.embed_flash = json_object_get_int(extend_obj);
                }
                if (json_object_object_get_ex(obj, "enabled_cpus_num", &extend_obj)) {
                    param->extend_param.enabled_cpus_num = json_object_get_int(extend_obj);
                }
                if (json_object_object_get_ex(obj, "enabled_cpus_mask", &extend_obj)) {
                    param->extend_param.enabled_cpus_mask = json_object_get_int(extend_obj);
                }
                if (json_object_object_get_ex(obj, "n_batch", &extend_obj)) {
                    int n_batch_val = json_object_get_int(extend_obj);
                    param->extend_param.n_batch = (n_batch_val > 0 && n_batch_val <= 100) ? n_batch_val : 1;
                }
                if (json_object_object_get_ex(obj, "use_cross_attn", &extend_obj)) {
                    param->extend_param.use_cross_attn = json_object_get_int(extend_obj);
                }
            }
            
            // Final validation and correction of critical parameters
            printf("🔍 DEBUG: Before final validation - n_batch = %d\n", param->extend_param.n_batch);
            if (param->extend_param.n_batch < 1 || param->extend_param.n_batch > 100) {
                printf("⚠️  Correcting invalid n_batch value %d to 1\n", param->extend_param.n_batch);
                param->extend_param.n_batch = 1;
            }
            printf("🔍 DEBUG: After final validation - n_batch = %d\n", param->extend_param.n_batch);
            
            return 0;
        }
        
        case RKLLM_PARAM_RKLLM_INPUT_PTR: {
            // Convert JSON object to RKLLMInput - COMPLETE IMPLEMENTATION
            if (json_object_get_type(json_value) != json_type_object) {
                return -1;
            }
            
            RKLLMInput* input = (RKLLMInput*)output;
            memset(input, 0, sizeof(RKLLMInput));
            
            json_object* obj;
            
            // Get common fields
            if (json_object_object_get_ex(json_value, "role", &obj)) {
                input->role = json_object_get_string(obj);
            }
            if (json_object_object_get_ex(json_value, "enable_thinking", &obj)) {
                input->enable_thinking = json_object_get_boolean(obj);
            }
            
            // Get input type and handle accordingly
            int input_type = RKLLM_INPUT_PROMPT; // default
            if (json_object_object_get_ex(json_value, "input_type", &obj)) {
                input_type = json_object_get_int(obj);
            }
            input->input_type = input_type;
            
            switch (input_type) {
                case RKLLM_INPUT_PROMPT:
                    if (json_object_object_get_ex(json_value, "prompt", &obj)) {
                        input->prompt_input = json_object_get_string(obj);
                    }
                    break;
                    
                case RKLLM_INPUT_EMBED:
                    if (json_object_object_get_ex(json_value, "embed_input", &obj)) {
                        json_object* embed_obj;
                        if (json_object_object_get_ex(obj, "n_tokens", &embed_obj)) {
                            input->embed_input.n_tokens = json_object_get_int(embed_obj);
                        }
                        
                        // Handle embed array data
                        if (json_object_object_get_ex(obj, "embed", &embed_obj)) {
                            if (json_object_get_type(embed_obj) == json_type_array) {
                                size_t embed_length = 0;
                                float* embed_data = NULL;
                                
                                // Use the array utility to convert JSON to float array
                                if (rkllm_convert_float_array(embed_obj, &embed_data, &embed_length) == 0) {
                                    input->embed_input.embed = embed_data;
                                    // Verify the length matches n_tokens if specified
                                    if (input->embed_input.n_tokens > 0 && 
                                        embed_length != input->embed_input.n_tokens * 1024) { // Assuming 1024 embed size
                                        printf("⚠️  Embed array size mismatch: expected %zu, got %zu\n",
                                               input->embed_input.n_tokens * 1024, embed_length);
                                    }
                                } else {
                                    printf("❌ Failed to convert embed array from JSON\n");
                                    input->embed_input.embed = NULL;
                                }
                            }
                        }
                    }
                    break;
                    
                case RKLLM_INPUT_TOKEN:
                    if (json_object_object_get_ex(json_value, "token_input", &obj)) {
                        json_object* token_obj;
                        if (json_object_object_get_ex(obj, "n_tokens", &token_obj)) {
                            input->token_input.n_tokens = json_object_get_int(token_obj);
                        }
                        
                        // Handle token array data
                        if (json_object_object_get_ex(obj, "input_ids", &token_obj)) {
                            if (json_object_get_type(token_obj) == json_type_array) {
                                size_t token_length = 0;
                                int32_t* token_data = NULL;
                                
                                // Use the array utility to convert JSON to int32_t array
                                if (rkllm_convert_int32_array(token_obj, &token_data, &token_length) == 0) {
                                    input->token_input.input_ids = token_data;
                                    // Verify the length matches n_tokens if specified
                                    if (input->token_input.n_tokens > 0 && 
                                        token_length != input->token_input.n_tokens) {
                                        printf("⚠️  Token array size mismatch: expected %zu, got %zu\n",
                                               input->token_input.n_tokens, token_length);
                                    }
                                } else {
                                    printf("❌ Failed to convert token array from JSON\n");
                                    input->token_input.input_ids = NULL;
                                }
                            }
                        }
                    }
                    break;
                    
                case RKLLM_INPUT_MULTIMODAL:
                    if (json_object_object_get_ex(json_value, "multimodal_input", &obj)) {
                        json_object* mm_obj;
                        
                        if (json_object_object_get_ex(obj, "prompt", &mm_obj)) {
                            input->multimodal_input.prompt = (char*)json_object_get_string(mm_obj);
                        }
                        if (json_object_object_get_ex(obj, "n_image_tokens", &mm_obj)) {
                            input->multimodal_input.n_image_tokens = json_object_get_int(mm_obj);
                        }
                        if (json_object_object_get_ex(obj, "n_image", &mm_obj)) {
                            input->multimodal_input.n_image = json_object_get_int(mm_obj);
                        }
                        if (json_object_object_get_ex(obj, "image_width", &mm_obj)) {
                            input->multimodal_input.image_width = json_object_get_int(mm_obj);
                        }
                        if (json_object_object_get_ex(obj, "image_height", &mm_obj)) {
                            input->multimodal_input.image_height = json_object_get_int(mm_obj);
                        }
                        
                        // Handle image_embed array
                        if (json_object_object_get_ex(obj, "image_embed", &mm_obj)) {
                            if (json_object_get_type(mm_obj) == json_type_array) {
                                size_t embed_length = 0;
                                float* embed_data = NULL;
                                
                                // Use the array utility to convert JSON to float array
                                if (rkllm_convert_float_array(mm_obj, &embed_data, &embed_length) == 0) {
                                    input->multimodal_input.image_embed = embed_data;
                                    // Verify the length matches expected dimensions
                                    size_t expected_size = input->multimodal_input.n_image * 
                                                         input->multimodal_input.n_image_tokens * 
                                                         1024; // Assuming 1024 embed length
                                    if (embed_length != expected_size && expected_size > 0) {
                                        printf("⚠️  Image embed array size mismatch: expected %zu, got %zu\n",
                                               expected_size, embed_length);
                                    }
                                } else {
                                    printf("❌ Failed to convert image embed array from JSON\n");
                                    input->multimodal_input.image_embed = NULL;
                                }
                            }
                        }
                    }
                    break;
            }
            
            return 0;
        }
        
        case RKLLM_PARAM_RKLLM_INFER_PARAM_PTR: {
            // Convert JSON object to RKLLMInferParam - COMPLETE IMPLEMENTATION
            if (json_object_get_type(json_value) != json_type_object) {
                return -1;
            }
            
            RKLLMInferParam* infer_param = (RKLLMInferParam*)output;
            memset(infer_param, 0, sizeof(RKLLMInferParam));
            
            // Default values
            infer_param->mode = RKLLM_INFER_GENERATE;
            infer_param->keep_history = 1;
            
            json_object* obj;
            
            if (json_object_object_get_ex(json_value, "mode", &obj)) {
                infer_param->mode = json_object_get_int(obj);
            }
            if (json_object_object_get_ex(json_value, "keep_history", &obj)) {
                infer_param->keep_history = json_object_get_int(obj);
            }
            
            // Handle lora_params (nested structure)
            if (json_object_object_get_ex(json_value, "lora_params", &obj)) {
                static RKLLMLoraParam lora_param_buffer;
                memset(&lora_param_buffer, 0, sizeof(RKLLMLoraParam));
                
                json_object* lora_obj;
                if (json_object_object_get_ex(obj, "lora_adapter_name", &lora_obj)) {
                    lora_param_buffer.lora_adapter_name = json_object_get_string(lora_obj);
                }
                
                infer_param->lora_params = &lora_param_buffer;
            }
            
            // Handle prompt_cache_params (nested structure)
            if (json_object_object_get_ex(json_value, "prompt_cache_params", &obj)) {
                static RKLLMPromptCacheParam cache_param_buffer;
                memset(&cache_param_buffer, 0, sizeof(RKLLMPromptCacheParam));
                
                json_object* cache_obj;
                if (json_object_object_get_ex(obj, "save_prompt_cache", &cache_obj)) {
                    cache_param_buffer.save_prompt_cache = json_object_get_int(cache_obj);
                }
                if (json_object_object_get_ex(obj, "prompt_cache_path", &cache_obj)) {
                    cache_param_buffer.prompt_cache_path = json_object_get_string(cache_obj);
                }
                
                infer_param->prompt_cache_params = &cache_param_buffer;
            }
            
            return 0;
        }
        
        case RKLLM_PARAM_CALLBACK:
            // Use our real callback function to capture model responses
            *(LLMResultCallback*)output = rkllm_proxy_callback;
            return 0;
            
        case RKLLM_PARAM_VOID_PTR:
            // For userdata, use NULL (always null for JSON-RPC)
            *(void**)output = NULL;
            return 0;
            
        case RKLLM_PARAM_RKLLM_LORA_ADAPTER_PTR: {
            // Convert JSON object to RKLLMLoraAdapter
            if (json_object_get_type(json_value) != json_type_object) {
                return -1;
            }
            
            RKLLMLoraAdapter* lora_adapter = (RKLLMLoraAdapter*)output;
            memset(lora_adapter, 0, sizeof(RKLLMLoraAdapter));
            
            json_object* obj;
            
            if (json_object_object_get_ex(json_value, "lora_adapter_path", &obj)) {
                lora_adapter->lora_adapter_path = json_object_get_string(obj);
            }
            if (json_object_object_get_ex(json_value, "lora_adapter_name", &obj)) {
                lora_adapter->lora_adapter_name = json_object_get_string(obj);
            }
            if (json_object_object_get_ex(json_value, "scale", &obj)) {
                lora_adapter->scale = (float)json_object_get_double(obj);
            }
            
            return 0;
        }
        
        case RKLLM_PARAM_RKLLM_CROSS_ATTN_PARAM_PTR: {
            // Convert JSON object to RKLLMCrossAttnParam
            if (json_object_get_type(json_value) != json_type_object) {
                return -1;
            }
            
            RKLLMCrossAttnParam* cross_attn = (RKLLMCrossAttnParam*)output;
            memset(cross_attn, 0, sizeof(RKLLMCrossAttnParam));
            
            json_object* obj;
            
            if (json_object_object_get_ex(json_value, "num_tokens", &obj)) {
                cross_attn->num_tokens = json_object_get_int(obj);
            }
            
            // Handle encoder_k_cache (4D array: [num_layers][num_tokens][num_kv_heads][head_dim])
            json_object* k_cache_obj;
            if (json_object_object_get_ex(json_value, "encoder_k_cache", &k_cache_obj)) {
                if (json_object_get_type(k_cache_obj) == json_type_array) {
                    size_t d1, d2, d3, d4;
                    float* k_cache_data = NULL;
                    if (rkllm_convert_float_array_4d(k_cache_obj, &k_cache_data, &d1, &d2, &d3, &d4) == 0) {
                        cross_attn->encoder_k_cache = k_cache_data;
                        printf("✅ Loaded encoder_k_cache: [%zu][%zu][%zu][%zu]\n", d1, d2, d3, d4);
                    } else {
                        printf("❌ Failed to convert encoder_k_cache from JSON\n");
                        cross_attn->encoder_k_cache = NULL;
                    }
                }
            }
            
            // Handle encoder_v_cache (4D array: [num_layers][num_kv_heads][head_dim][num_tokens])
            json_object* v_cache_obj;
            if (json_object_object_get_ex(json_value, "encoder_v_cache", &v_cache_obj)) {
                if (json_object_get_type(v_cache_obj) == json_type_array) {
                    size_t d1, d2, d3, d4;
                    float* v_cache_data = NULL;
                    if (rkllm_convert_float_array_4d(v_cache_obj, &v_cache_data, &d1, &d2, &d3, &d4) == 0) {
                        cross_attn->encoder_v_cache = v_cache_data;
                        printf("✅ Loaded encoder_v_cache: [%zu][%zu][%zu][%zu]\n", d1, d2, d3, d4);
                    } else {
                        printf("❌ Failed to convert encoder_v_cache from JSON\n");
                        cross_attn->encoder_v_cache = NULL;
                    }
                }
            }
            
            // Handle encoder_mask (1D float array)
            json_object* mask_obj;
            if (json_object_object_get_ex(json_value, "encoder_mask", &mask_obj)) {
                if (json_object_get_type(mask_obj) == json_type_array) {
                    size_t mask_length = 0;
                    float* mask_data = NULL;
                    if (rkllm_convert_float_array(mask_obj, &mask_data, &mask_length) == 0) {
                        cross_attn->encoder_mask = mask_data;
                        if (mask_length != (size_t)cross_attn->num_tokens) {
                            printf("⚠️  Encoder mask size mismatch: expected %d, got %zu\n",
                                   cross_attn->num_tokens, mask_length);
                        }
                    } else {
                        printf("❌ Failed to convert encoder_mask from JSON\n");
                        cross_attn->encoder_mask = NULL;
                    }
                }
            }
            
            // Handle encoder_pos (1D int32_t array)
            json_object* pos_obj;
            if (json_object_object_get_ex(json_value, "encoder_pos", &pos_obj)) {
                if (json_object_get_type(pos_obj) == json_type_array) {
                    size_t pos_length = 0;
                    int32_t* pos_data = NULL;
                    if (rkllm_convert_int32_array(pos_obj, &pos_data, &pos_length) == 0) {
                        cross_attn->encoder_pos = pos_data;
                        if (pos_length != (size_t)cross_attn->num_tokens) {
                            printf("⚠️  Encoder pos size mismatch: expected %d, got %zu\n",
                                   cross_attn->num_tokens, pos_length);
                        }
                    } else {
                        printf("❌ Failed to convert encoder_pos from JSON\n");
                        cross_attn->encoder_pos = NULL;
                    }
                }
            }
            
            return 0;
        }
        
        case RKLLM_PARAM_RKLLM_LORA_PARAM_PTR: {
            // Convert JSON object to RKLLMLoraParam
            if (json_object_get_type(json_value) != json_type_object) {
                return -1;
            }
            
            RKLLMLoraParam* lora_param = (RKLLMLoraParam*)output;
            memset(lora_param, 0, sizeof(RKLLMLoraParam));
            
            json_object* obj;
            
            if (json_object_object_get_ex(json_value, "lora_adapter_name", &obj)) {
                lora_param->lora_adapter_name = json_object_get_string(obj);
            }
            
            return 0;
        }
        
        case RKLLM_PARAM_RKLLM_PROMPT_CACHE_PARAM_PTR: {
            // Convert JSON object to RKLLMPromptCacheParam
            if (json_object_get_type(json_value) != json_type_object) {
                return -1;
            }
            
            RKLLMPromptCacheParam* cache_param = (RKLLMPromptCacheParam*)output;
            memset(cache_param, 0, sizeof(RKLLMPromptCacheParam));
            
            json_object* obj;
            
            if (json_object_object_get_ex(json_value, "save_prompt_cache", &obj)) {
                cache_param->save_prompt_cache = json_object_get_int(obj);
            }
            if (json_object_object_get_ex(json_value, "prompt_cache_path", &obj)) {
                cache_param->prompt_cache_path = json_object_get_string(obj);
            }
            
            return 0;
        }
        
        case RKLLM_PARAM_SIZE_T:
            if (json_object_get_type(json_value) == json_type_int) {
                *(size_t*)output = (size_t)json_object_get_int64(json_value);
                return 0;
            }
            return -1;
            
        case RKLLM_PARAM_INT32_T:
            if (json_object_get_type(json_value) == json_type_int) {
                *(int32_t*)output = (int32_t)json_object_get_int(json_value);
                return 0;
            }
            return -1;
            
        case RKLLM_PARAM_UINT32_T:
            if (json_object_get_type(json_value) == json_type_int) {
                *(uint32_t*)output = (uint32_t)json_object_get_int(json_value);
                return 0;
            }
            return -1;
            
        case RKLLM_PARAM_UINT8_T:
            if (json_object_get_type(json_value) == json_type_int) {
                *(uint8_t*)output = (uint8_t)json_object_get_int(json_value);
                return 0;
            }
            return -1;
            
        case RKLLM_PARAM_FLOAT_PTR:
            // For float arrays, simplified implementation
            *(float**)output = NULL;
            return 0;
            
        case RKLLM_PARAM_INT32_T_PTR:
            // For int32_t arrays, simplified implementation
            *(int32_t**)output = NULL;
            return 0;
            
        case RKLLM_PARAM_INT_PTR:
            // For int pointers, allocate buffer or use NULL
            *(int**)output = NULL;
            return 0;
            
        default:
            return -1;
    }
}


// Initialize the dynamic RKLLM proxy system
int rkllm_proxy_init(void) {
    if (g_proxy_initialized) {
        return 0;
    }
    
    // Initialize response buffer with size from settings
    g_response_buffer_size = SETTING_BUFFER(proxy_response_buffer_size);
    if (g_response_buffer_size == 0) g_response_buffer_size = 8192; // fallback
    
    g_response_buffer = calloc(1, g_response_buffer_size);
    if (!g_response_buffer) {
        printf("❌ Failed to allocate RKLLM response buffer\n");
        return -1;
    }
    
    // Memory management removed - RKLLM handles NPU memory internally
    
    // Initialize memory pool for arrays (default 64MB)
    size_t pool_size = 64 * 1024 * 1024; // 64MB default
    // TODO: Add array_pool_size to settings if needed
    
    // Pool management removed - RKLLM handles NPU memory internally
    
    // Create simple array pool for temporary buffers
    g_rkllm_array_pool = rkllm_pool_create(pool_size);
    if (g_rkllm_array_pool) {
        printf("✅ RKLLM array pool created: %zu MB\n", pool_size / (1024 * 1024));
    }
    if (!g_rkllm_array_pool) {
        printf("⚠️  RKLLM array pool creation failed, will use malloc instead\n");
    }
    
    // Initialize streaming context manager
    if (rkllm_stream_manager_init() != 0) {
        printf("⚠️  RKLLM streaming context manager initialization failed\n");
        // Continue without streaming support
    }
    
    // Performance monitoring removed - simplified architecture
    
    // Performance monitoring removed - simplified architecture
    
    g_proxy_initialized = true;
    printf("✅ RKLLM proxy initialized with %d functions (direct calls), buffer size: %zu\n", g_function_count, g_response_buffer_size);
    return 0;
}

// Shutdown the dynamic RKLLM proxy system
void rkllm_proxy_shutdown(void) {
    if (!g_proxy_initialized) {
        return;
    }
    
    // Free response buffer
    free(g_response_buffer);
    g_response_buffer = NULL;
    g_response_buffer_size = 0;
    
    // Destroy memory pool for arrays
    if (g_rkllm_array_pool) {
        rkllm_pool_destroy(g_rkllm_array_pool);
        g_rkllm_array_pool = NULL;
    }
    
    // Shutdown streaming context manager
    rkllm_stream_manager_shutdown();
    
    // Memory management removed - RKLLM handles NPU memory internally
    
    // Performance monitoring removed
    
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
    printf("[PROXY] rkllm_proxy_call entered with function: %s\n", function_name ? function_name : "NULL");
    fflush(stdout);
    
    if (!g_proxy_initialized || !function_name || !result_json) {
        printf("🔧 DEBUG: Early return - proxy_init:%d, func_name:%p, result:%p\n", 
               g_proxy_initialized, function_name, result_json);
        fflush(stdout);
        return -1;
    }
    
    // Special handler for meta functions
    if (strcmp(function_name, "rkllm_get_constants") == 0) {
        return rkllm_proxy_get_constants(result_json);
    }
    
    if (strcmp(function_name, "rkllm_get_functions") == 0) {
        return rkllm_proxy_get_functions(result_json);
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
        
        // Set argument pointer based on parameter type
        switch (func_desc->params[i].type) {
            case RKLLM_PARAM_HANDLE:
                args[i] = *(LLMHandle*)arg_buffers[i];  // Pass handle by value
                break;
            case RKLLM_PARAM_HANDLE_PTR:
                args[i] = (LLMHandle*)arg_buffers[i];   // Pass pointer to handle
                break;
            case RKLLM_PARAM_STRING:
                args[i] = (void*)*(const char**)arg_buffers[i]; // Pass string pointer
                break;
            case RKLLM_PARAM_INT:
                args[i] = (void*)(intptr_t)*(int*)arg_buffers[i]; // Pass int by value
                break;
            case RKLLM_PARAM_BOOL:
                args[i] = (void*)(intptr_t)*(bool*)arg_buffers[i]; // Pass bool by value
                break;
            case RKLLM_PARAM_FLOAT:
                args[i] = (void*)(intptr_t)*(float*)arg_buffers[i]; // Pass float by value (cast to avoid precision loss)
                break;
            case RKLLM_PARAM_CALLBACK:
                args[i] = *(LLMResultCallback*)arg_buffers[i]; // Pass callback function pointer
                break;
            case RKLLM_PARAM_VOID_PTR:
                args[i] = *(void**)arg_buffers[i]; // Pass void pointer
                break;
            default:
                // For all complex structure pointers, pass as pointer to buffer
                args[i] = arg_buffers[i];
                break;
        }
    }
    
    // Special pre-call validation for rkllm_init 
    if (strcmp(function_name, "rkllm_init") == 0) {
        RKLLMParam* param = (RKLLMParam*)args[1];
        if (param) {
            printf("🔧 BEFORE validation - n_batch: %d\n", param->extend_param.n_batch);
            
            // Apply 3-tier priority system validation for critical parameters
            if (param->extend_param.n_batch < 1 || param->extend_param.n_batch > 100) {
                uint8_t setting_val = SETTINGS() ? SETTINGS()->rkllm.extend.n_batch : 1;
                param->extend_param.n_batch = (setting_val > 0 && setting_val <= 100) ? setting_val : 1;
                printf("⚠️  Applied n_batch from settings/default: %d\n", param->extend_param.n_batch);
            }
            
            printf("🔧 AFTER validation - n_batch: %d\n", param->extend_param.n_batch);
            fflush(stdout);
        }
    }
    
    // Phase 3.1: Initialize streaming session for rkllm_run_async
    char temp_request_id[32] = {0};
    if (strcmp(function_name, "rkllm_run_async") == 0) {
        // Generate a simple request ID for this async operation
        snprintf(temp_request_id, sizeof(temp_request_id), "%ld", (long)time(NULL));
        
        printf("[STREAMING] Pre-call: Starting streaming session for %s (req_id: %s)\n", 
               function_name, temp_request_id);
        
        // Start streaming session before RKLLM call
        if (rkllm_proxy_start_streaming_session(temp_request_id, function_name) != 0) {
            printf("[STREAMING] WARNING: Failed to start streaming session, proceeding without streaming\n");
        } else {
            printf("[STREAMING] Streaming session initialized successfully\n");
        }
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
            status = func1(args[0]);
            break;
        }
        case 2: {
            int (*func2)(void*, void*) = (int (*)(void*, void*))func_desc->function_ptr;
            status = func2(args[0], args[1]);
            break;
        }
        case 3: {
            int (*func3)(void*, void*, void*) = (int (*)(void*, void*, void*))func_desc->function_ptr;
            status = func3(args[0], args[1], args[2]);
            break;
        }
        case 4: {
            int (*func4)(void*, void*, void*, void*) = (int (*)(void*, void*, void*, void*))func_desc->function_ptr;
            status = func4(args[0], args[1], args[2], args[3]);
            break;
        }
        default:
            if (params) json_object_put(params);
            *result_json = strdup("{\"error\": \"Unsupported parameter count\"}");
            return -1;
    }
    
    // Special handling for rkllm_init - save the handle and add detailed error logging
    if (strcmp(function_name, "rkllm_init") == 0) {
        printf("🔧 DEBUG: rkllm_init detected, status=%d\n", status);
        
        // Debug parameter values that WILL BE passed to rkllm_init (before call)
        RKLLMParam* param = (RKLLMParam*)args[1];
        if (param) {
            printf("🔧 BEFORE rkllm_init call - n_batch: %d\n", param->extend_param.n_batch);
        }
        fflush(stdout);
        
        if (status != 0) {
            printf("❌ rkllm_init failed with error code: %d\n", status);
            
            // Debug parameter values that were passed to rkllm_init
            RKLLMParam* param = (RKLLMParam*)args[1];
            if (param) {
                printf("📋 RKLLM Parameters used:\n");
                printf("   Model path: %s\n", param->model_path ? param->model_path : "(null)");
                printf("   Max context: %d\n", param->max_context_len);
                printf("   Max new tokens: %d\n", param->max_new_tokens);
                printf("   Temperature: %.3f\n", param->temperature);
                printf("   Top K: %d\n", param->top_k);
                printf("   Top P: %.3f\n", param->top_p);
                printf("   CPU mask: 0x%X\n", param->extend_param.enabled_cpus_mask);
                printf("   Base domain ID: %d\n", param->extend_param.base_domain_id);
                printf("   Enabled CPUs: %d\n", param->extend_param.enabled_cpus_num);
                printf("   n_batch: %d\n", param->extend_param.n_batch);
                
                // Check model file accessibility
                if (param->model_path) {
                    if (access(param->model_path, R_OK) != 0) {
                        printf("❌ Cannot read model file: %s (error: %s)\n", param->model_path, strerror(errno));
                    } else {
                        printf("✅ Model file accessible: %s\n", param->model_path);
                    }
                }
            }
            fflush(stdout);
        } else {
            // The handle was set via the pointer in args[0] (RKLLM_PARAM_HANDLE_PTR)
            // Extract the initialized handle and save it globally
            LLMHandle* handle_ptr = (LLMHandle*)args[0];
            printf("🔧 DEBUG: handle_ptr=%p\n", handle_ptr);
            fflush(stdout);
            if (handle_ptr) {
                g_global_handle = *handle_ptr;
                printf("🔧 RKLLM handle initialized and saved globally: %p\n", g_global_handle);
                fflush(stdout);
            }
        }
    }
    
    // Special handling for rkllm_run - prepare for callback responses
    if (strcmp(function_name, "rkllm_run") == 0 || strcmp(function_name, "rkllm_run_async") == 0) {
        printf("🔄 DEBUG: %s detected, status=%d\n", function_name, status);
        fflush(stdout);
        
        // Clear previous response buffer
        memset(g_response_buffer, 0, g_response_buffer_size);
        g_response_ready = false;
        g_last_call_state = RKLLM_RUN_NORMAL;
        
        if (status == 0) {
            printf("🔄 RKLLM inference started, waiting for callback responses...\n");
            fflush(stdout);
            
            // For synchronous calls, the callback should have been called by now
            // For async calls, responses will come later via callback
            
            // Add buffer state validation after inference
            printf("📊 Response buffer state after %s:\n", function_name);
            printf("   Buffer ready: %s\n", g_response_ready ? "true" : "false");
            printf("   Buffer content length: %zu\n", strlen(g_response_buffer));
            printf("   Buffer content: '%.100s'%s\n", g_response_buffer, 
                   strlen(g_response_buffer) > 100 ? "..." : "");
            printf("   Last call state: %d\n", g_last_call_state);
            fflush(stdout);
        } else {
            printf("❌ RKLLM inference failed with status: %d\n", status);
            fflush(stdout);
        }
    }
    
    // Special handling for rkllm_destroy - clear the handle
    if (strcmp(function_name, "rkllm_destroy") == 0 && status == 0) {
        g_global_handle = NULL;
        printf("🧹 RKLLM handle destroyed and cleared globally\n");
    }
    
    // Convert result to JSON with error mapping
    int convert_result = rkllm_proxy_convert_result(func_desc->return_type, result_data, result_json, function_name, status);
    
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

// Convert RKLLM result to JSON with error mapping
int rkllm_proxy_convert_result(rkllm_return_type_t return_type, void* result_data, 
                               char** result_json, const char* function_name, int status) {
    if (!result_json) {
        return -1;
    }
    
    // Handle error cases first
    if (status != 0) {
        // Use error mapping to create proper JSON-RPC error response
        uint32_t request_id = 0; // We don't have request_id at this level
        *result_json = rkllm_create_error_response(request_id, status, function_name);
        rkllm_log_error(status, function_name, "Operation failed");
        return -1; // Indicate this is an error response
    }
    
    // Handle success cases based on return type
    switch (return_type) {
        case RKLLM_RETURN_INT: {
            // Integer result (typically status code)
            int* int_result = (int*)result_data;
            json_object* response = json_object_new_object();
            json_object_object_add(response, "status", json_object_new_string("success"));
            json_object_object_add(response, "return_code", json_object_new_int(*int_result));
            
            const char* json_str = json_object_to_json_string(response);
            *result_json = strdup(json_str);
            json_object_put(response);
            break;
        }
        
        case RKLLM_RETURN_VOID: {
            // Void result (success only)
            json_object* response = json_object_new_object();
            json_object_object_add(response, "status", json_object_new_string("success"));
            json_object_object_add(response, "message", json_object_new_string("Operation completed"));
            
            const char* json_str = json_object_to_json_string(response);
            *result_json = strdup(json_str);
            json_object_put(response);
            break;
        }
        
        case RKLLM_RETURN_RKLLM_PARAM: {
            // RKLLMParam struct result (for createDefaultParam)
            RKLLMParam* param = (RKLLMParam*)result_data;
            json_object* response = json_object_new_object();
            json_object_object_add(response, "status", json_object_new_string("success"));
            
            // Convert RKLLMParam to JSON
            json_object* param_obj = json_object_new_object();
            json_object_object_add(param_obj, "model_path", json_object_new_string(param->model_path ? param->model_path : ""));
            json_object_object_add(param_obj, "max_context_len", json_object_new_int(param->max_context_len));
            json_object_object_add(param_obj, "max_new_tokens", json_object_new_int(param->max_new_tokens));
            json_object_object_add(param_obj, "top_k", json_object_new_int(param->top_k));
            json_object_object_add(param_obj, "n_keep", json_object_new_int(param->n_keep));
            json_object_object_add(param_obj, "top_p", json_object_new_double(param->top_p));
            json_object_object_add(param_obj, "temperature", json_object_new_double(param->temperature));
            json_object_object_add(param_obj, "repeat_penalty", json_object_new_double(param->repeat_penalty));
            json_object_object_add(param_obj, "frequency_penalty", json_object_new_double(param->frequency_penalty));
            json_object_object_add(param_obj, "presence_penalty", json_object_new_double(param->presence_penalty));
            json_object_object_add(param_obj, "mirostat", json_object_new_int(param->mirostat));
            json_object_object_add(param_obj, "mirostat_tau", json_object_new_double(param->mirostat_tau));
            json_object_object_add(param_obj, "mirostat_eta", json_object_new_double(param->mirostat_eta));
            json_object_object_add(param_obj, "skip_special_token", json_object_new_boolean(param->skip_special_token));
            json_object_object_add(param_obj, "is_async", json_object_new_boolean(param->is_async));
            
            // Add extend parameters
            json_object* extend_obj = json_object_new_object();
            json_object_object_add(extend_obj, "base_domain_id", json_object_new_int(param->extend_param.base_domain_id));
            json_object_object_add(extend_obj, "embed_flash", json_object_new_int(param->extend_param.embed_flash));
            json_object_object_add(extend_obj, "enabled_cpus_num", json_object_new_int(param->extend_param.enabled_cpus_num));
            json_object_object_add(extend_obj, "enabled_cpus_mask", json_object_new_int64(param->extend_param.enabled_cpus_mask));
            json_object_object_add(extend_obj, "n_batch", json_object_new_int(param->extend_param.n_batch));
            json_object_object_add(extend_obj, "use_cross_attn", json_object_new_int(param->extend_param.use_cross_attn));
            json_object_object_add(param_obj, "extend_param", extend_obj);
            
            json_object_object_add(response, "param", param_obj);
            
            const char* json_str = json_object_to_json_string(response);
            *result_json = strdup(json_str);
            json_object_put(response);
            break;
        }
        
        case RKLLM_RETURN_JSON: {
            // Already JSON (for meta functions like get_functions, get_constants)
            if (function_name && strcmp(function_name, "rkllm_get_functions") == 0) {
                return rkllm_proxy_get_functions(result_json);
            } else if (function_name && strcmp(function_name, "rkllm_get_constants") == 0) {
                return rkllm_proxy_get_constants(result_json);
            } else {
                // Generic JSON response
                json_object* response = json_object_new_object();
                json_object_object_add(response, "status", json_object_new_string("success"));
                json_object_object_add(response, "data", json_object_new_string("JSON result"));
                
                const char* json_str = json_object_to_json_string(response);
                *result_json = strdup(json_str);
                json_object_put(response);
            }
            break;
        }
        
        default:
            // Unknown return type
            json_object* response = json_object_new_object();
            json_object_object_add(response, "status", json_object_new_string("error"));
            json_object_object_add(response, "message", json_object_new_string("Unknown return type"));
            
            const char* json_str = json_object_to_json_string(response);
            *result_json = strdup(json_str);
            json_object_put(response);
            return -1;
    }
    
    return 0;
}

// Set global RKLLM handle
void rkllm_proxy_set_handle(LLMHandle handle) {
    g_global_handle = handle;
}