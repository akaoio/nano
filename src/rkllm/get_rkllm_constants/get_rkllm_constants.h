#ifndef GET_RKLLM_CONSTANTS_H
#define GET_RKLLM_CONSTANTS_H

#include <json-c/json.h>

/**
 * @brief Get RKLLM constants and enums as JSON object
 * @return JSON object containing all RKLLM constants, enums, and state values
 */
json_object* get_rkllm_constants(void);

#endif