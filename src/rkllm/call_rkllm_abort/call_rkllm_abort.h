#ifndef CALL_RKLLM_ABORT_H
#define CALL_RKLLM_ABORT_H

#include <json-c/json.h>

/**
 * @brief Abort ongoing RKLLM task
 * @return JSON object with abort result
 */
json_object* call_rkllm_abort(void);

#endif