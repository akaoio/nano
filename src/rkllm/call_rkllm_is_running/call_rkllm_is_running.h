#ifndef CALL_RKLLM_IS_RUNNING_H
#define CALL_RKLLM_IS_RUNNING_H

#include <json-c/json.h>

/**
 * @brief Check if RKLLM task is currently running
 * @return JSON object with running status
 */
json_object* call_rkllm_is_running(void);

#endif