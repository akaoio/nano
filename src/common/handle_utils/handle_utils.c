#include "handle_utils.h"
#include "../../io/operations.h"
#include "../error_utils/error_utils.h"
#include <string.h>

bool is_valid_handle_id(uint32_t handle_id) {
    return handle_id != 0 && handle_id != UINT32_MAX;
}

LLMHandle get_validated_handle(uint32_t handle_id) {
    (void)handle_id; // Ignore handle_id - hardware only supports 1 model
    return io_get_rkllm_handle();
}

LLMHandle get_validated_handle_or_error(uint32_t handle_id, void* result) {
    (void)result; // Suppress unused parameter warning for lightweight approach
    return get_validated_handle(handle_id);
}