#include "handle_utils.h"
#include "../../io/mapping/rkllm_proxy/rkllm_proxy.h"
#include "../error_utils/error_utils.h"
#include <string.h>

bool is_valid_handle_id(uint32_t handle_id) {
    return handle_id != 0 && handle_id != UINT32_MAX;
}

LLMHandle get_validated_handle(uint32_t handle_id) {
    if (!is_valid_handle_id(handle_id)) {
        return nullptr;
    }
    
    return rkllm_proxy_get_handle(handle_id);
}

LLMHandle get_validated_handle_or_error(uint32_t handle_id, void* result) {
    LLMHandle handle = get_validated_handle(handle_id);
    if (!handle) {
        SET_ERROR_RESULT((rkllm_result_t*)result, -1, "Invalid handle");
        return nullptr;
    }
    return handle;
}