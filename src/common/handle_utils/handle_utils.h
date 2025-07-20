#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "../../external/rkllm/rkllm.h"

/**
 * @brief Validate handle ID
 * @param handle_id Handle ID to validate
 * @return true if valid, false otherwise
 */
bool is_valid_handle_id(uint32_t handle_id);

/**
 * @brief Get handle from handle ID with validation
 * @param handle_id Handle ID
 * @return LLMHandle on success, nullptr on failure
 */
LLMHandle get_validated_handle(uint32_t handle_id);

/**
 * @brief Validate handle and get it in one operation
 * @param handle_id Handle ID to validate and retrieve
 * @param result Result structure to populate on error
 * @return LLMHandle on success, nullptr on failure (result populated)
 */
LLMHandle get_validated_handle_or_error(uint32_t handle_id, void* result);


