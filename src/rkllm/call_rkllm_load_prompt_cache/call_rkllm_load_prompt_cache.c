#include "call_rkllm_load_prompt_cache.h"
#include "../call_rkllm_init/call_rkllm_init.h"
#include "../../utils/log_message/log_message.h"
#include "../../utils/constants/constants.h"
#include "../../utils/safe_string/safe_string.h"
#include <rkllm.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>

// External reference to global LLM handle and state from call_rkllm_init
extern LLMHandle global_llm_handle;
extern int global_llm_initialized;

json_object* call_rkllm_load_prompt_cache(json_object* params) {
    json_object* error_obj = json_object_new_object();
    
    // Validate that model is initialized
    if (!global_llm_initialized || !global_llm_handle) {
        json_object_object_add(error_obj, "code", json_object_new_int(-32000));
        json_object_object_add(error_obj, "message", json_object_new_string("Model not initialized - call rkllm.init first"));
        return error_obj;
    }
    
    if (!params || !json_object_is_type(params, json_type_array)) {
        json_object_object_add(error_obj, "code", json_object_new_int(-32602));
        json_object_object_add(error_obj, "message", json_object_new_string("Invalid parameters - expected array"));
        return error_obj;
    }
    
    // Expect 2 parameters: [handle, prompt_cache_path]
    if (json_object_array_length(params) < 2) {
        json_object_object_add(error_obj, "code", json_object_new_int(-32602));
        json_object_object_add(error_obj, "message", json_object_new_string("Insufficient parameters - expected [handle, prompt_cache_path]"));
        return error_obj;
    }
    
    // Get prompt_cache_path (parameter 1)
    json_object* path_obj = json_object_array_get_idx(params, 1);
    if (!path_obj || !json_object_is_type(path_obj, json_type_string)) {
        json_object_object_add(error_obj, "code", json_object_new_int(-32602));
        json_object_object_add(error_obj, "message", json_object_new_string("Invalid prompt_cache_path parameter - expected string"));
        return error_obj;
    }
    
    const char* prompt_cache_path = json_object_get_string(path_obj);
    if (!prompt_cache_path || strlen(prompt_cache_path) == 0) {
        json_object_object_add(error_obj, "code", json_object_new_int(-32602));
        json_object_object_add(error_obj, "message", json_object_new_string("Empty prompt cache path provided"));
        return error_obj;
    }
    
    // Check if file exists before calling RKLLM function to prevent crashes
    struct stat stat_buf;
    if (stat(prompt_cache_path, &stat_buf) != 0) {
        char error_msg[ERROR_MESSAGE_BUFFER_SIZE];
        if (safe_snprintf(error_msg, sizeof(error_msg), "Prompt cache file does not exist: %s (errno: %d - %s)", 
                         prompt_cache_path, errno, strerror(errno)) < 0) {
            // Fallback error message if formatting fails
            safe_strcpy(error_msg, "Prompt cache file does not exist", sizeof(error_msg));
        }
        
        json_object_object_add(error_obj, "code", json_object_new_int(-32000));
        json_object_object_add(error_obj, "message", json_object_new_string(error_msg));
        return error_obj;
    }
    
    // Additional safety check - ensure it's a regular file
    if (!S_ISREG(stat_buf.st_mode)) {
        json_object_object_add(error_obj, "code", json_object_new_int(-32000));
        json_object_object_add(error_obj, "message", json_object_new_string("Prompt cache path is not a regular file"));
        return error_obj;
    }
    
    LOG_INFO_MSG("Loading prompt cache from: %s", prompt_cache_path);
    
    // Call rkllm_load_prompt_cache with proper error handling
    int result = rkllm_load_prompt_cache(global_llm_handle, prompt_cache_path);
    
    // Clean up error object since we're returning success
    json_object_put(error_obj);
    
    if (result == 0) {
        // Success
        json_object* result_obj = json_object_new_object();
        json_object_object_add(result_obj, "success", json_object_new_boolean(1));
        json_object_object_add(result_obj, "message", json_object_new_string("Prompt cache loaded successfully"));
        json_object_object_add(result_obj, "cache_path", json_object_new_string(prompt_cache_path));
        
        LOG_INFO_MSG("Prompt cache loaded successfully: %s", prompt_cache_path);
        return result_obj;
    } else {
        // RKLLM function failed - return proper error
        json_object* error_result = json_object_new_object();
        char error_msg[SMALL_ERROR_BUFFER_SIZE];
        if (safe_snprintf(error_msg, sizeof(error_msg), "RKLLM load_prompt_cache failed with code: %d", result) < 0) {
            safe_strcpy(error_msg, "RKLLM load_prompt_cache failed", sizeof(error_msg));
        }
        
        json_object_object_add(error_result, "code", json_object_new_int(-32000));
        json_object_object_add(error_result, "message", json_object_new_string(error_msg));
        
        LOG_ERROR_MSG("Prompt cache loading failed: %s (code: %d)", prompt_cache_path, result);
        return error_result;
    }
}