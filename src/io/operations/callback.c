#include "operations.h"
#include <stdio.h>

// Inference callback for streaming output
int inference_callback(RKLLMResult* result, void* userdata, LLMCallState state) {
    if (!result || !userdata) return 0;
    CallbackData* data = (CallbackData*)userdata;
    
    if (state == RKLLM_RUN_NORMAL && result->text) {
        // Append to buffer
        if (data->output_buffer && data->buffer_size > 0) {
            size_t current_len = strlen(data->output_buffer);
            size_t text_len = strlen(result->text);
            
            if (current_len + text_len + 1 < data->buffer_size) {
                strcat(data->output_buffer, result->text);
            }
        }
    } else if (state == RKLLM_RUN_FINISH) {
        data->finished = 1;
    } else if (state == RKLLM_RUN_ERROR) {
        printf("❌ Inference error occurred\n");
        data->finished = 1;
    }
    return 0;
}
