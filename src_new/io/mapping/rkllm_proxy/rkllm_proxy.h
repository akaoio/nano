#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "../../../libs/rkllm/rkllm.h"
#include "../../../common/json_utils/json_utils.h"

// RKLLM Proxy - Maps RKLLM functions without "rkllm_" prefix
// Handles JSON request/response and LLMHandle ID mapping

// Method function signature
typedef int (*proxy_method_t)(uint32_t handle_id, const char* params, char* result, size_t result_size);

// Method mapping structure
typedef struct {
    const char* method;
    proxy_method_t func;
} method_mapping_t;

/**
 * @brief Initialize RKLLM proxy
 * @return 0 on success, -1 on error
 */
int rkllm_proxy_init(void);

/**
 * @brief Execute method by name
 * @param method Method name (without "rkllm_" prefix)
 * @param handle_id Handle ID
 * @param params JSON parameters
 * @param result Result buffer
 * @param result_size Result buffer size
 * @return 0 on success, -1 on error
 */
int rkllm_proxy_execute(const char* method, uint32_t handle_id, const char* params, char* result, size_t result_size);

/**
 * @brief Shutdown RKLLM proxy
 */
void rkllm_proxy_shutdown(void);

// Individual method implementations (without "rkllm_" prefix)
int proxy_init(uint32_t handle_id, const char* params, char* result, size_t result_size);
int proxy_run(uint32_t handle_id, const char* params, char* result, size_t result_size);
int proxy_destroy(uint32_t handle_id, const char* params, char* result, size_t result_size);
int proxy_status(uint32_t handle_id, const char* params, char* result, size_t result_size);
int proxy_lora_init(uint32_t handle_id, const char* params, char* result, size_t result_size);
int proxy_abort(uint32_t handle_id, const char* params, char* result, size_t result_size);
int proxy_clear_kv_cache(uint32_t handle_id, const char* params, char* result, size_t result_size);
