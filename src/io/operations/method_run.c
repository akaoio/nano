#include "operations.h"
#include "../handle_pool.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>

extern handle_pool_t g_pool;

int method_run(uint32_t handle_id, const char* params, char* result, size_t result_size) {
    if (!handle_pool_is_valid(&g_pool, handle_id)) return -1;
    
    LLMHandle* handle = handle_pool_get(&g_pool, handle_id);
    if (!handle) return -1;
    
    char prompt[1024];
    if (!json_get_string(params, "prompt", prompt, sizeof(prompt))) return -1;
    
    RKLLMInput input = {0};
    input.input_type = RKLLM_INPUT_PROMPT;
    input.prompt_input = prompt;
    
    RKLLMInferParam infer_param = {0};
    infer_param.mode = RKLLM_INFER_GENERATE;
    infer_param.keep_history = 0;
    
    char output_buffer[1024] = {0};
    CallbackData callback_data = {output_buffer, sizeof(output_buffer), 0};
    
    int ret = rkllm_run(*handle, &input, &infer_param, &callback_data);
    if (ret != 0) return ret;
    
    // Wait for completion (30s timeout)
    int timeout = 300;
    while (!callback_data.finished && timeout-- > 0) {
        usleep(100000);
    }
    
    if (!callback_data.finished) return -1;
    if (strlen(output_buffer) == 0) return -1;
    
    snprintf(result, result_size, "{\"text\":\"%s\"}", output_buffer);
    return 0;
}
