#include "get_rkllm_constants.h"
#include <stdbool.h>
#include <rkllm.h>
#include <json-c/json.h>

json_object* get_rkllm_constants(void) {
    json_object* constants = json_object_new_object();
    
    // CPU masks for CPU affinity control
    json_object* cpu_masks = json_object_new_object();
    json_object_object_add(cpu_masks, "CPU0", json_object_new_int(CPU0));
    json_object_object_add(cpu_masks, "CPU1", json_object_new_int(CPU1));
    json_object_object_add(cpu_masks, "CPU2", json_object_new_int(CPU2));
    json_object_object_add(cpu_masks, "CPU3", json_object_new_int(CPU3));
    json_object_object_add(cpu_masks, "CPU4", json_object_new_int(CPU4));
    json_object_object_add(cpu_masks, "CPU5", json_object_new_int(CPU5));
    json_object_object_add(cpu_masks, "CPU6", json_object_new_int(CPU6));
    json_object_object_add(cpu_masks, "CPU7", json_object_new_int(CPU7));
    json_object_object_add(constants, "CPU_MASKS", cpu_masks);
    
    // LLMCallState enum values
    json_object* call_states = json_object_new_object();
    json_object_object_add(call_states, "RKLLM_RUN_NORMAL", json_object_new_int(RKLLM_RUN_NORMAL));
    json_object_object_add(call_states, "RKLLM_RUN_WAITING", json_object_new_int(RKLLM_RUN_WAITING));
    json_object_object_add(call_states, "RKLLM_RUN_FINISH", json_object_new_int(RKLLM_RUN_FINISH));
    json_object_object_add(call_states, "RKLLM_RUN_ERROR", json_object_new_int(RKLLM_RUN_ERROR));
    json_object_object_add(constants, "LLM_CALL_STATES", call_states);
    
    // RKLLMInputType enum values
    json_object* input_types = json_object_new_object();
    json_object_object_add(input_types, "RKLLM_INPUT_PROMPT", json_object_new_int(RKLLM_INPUT_PROMPT));
    json_object_object_add(input_types, "RKLLM_INPUT_TOKEN", json_object_new_int(RKLLM_INPUT_TOKEN));
    json_object_object_add(input_types, "RKLLM_INPUT_EMBED", json_object_new_int(RKLLM_INPUT_EMBED));
    json_object_object_add(input_types, "RKLLM_INPUT_MULTIMODAL", json_object_new_int(RKLLM_INPUT_MULTIMODAL));
    json_object_object_add(constants, "INPUT_TYPES", input_types);
    
    // RKLLMInferMode enum values
    json_object* infer_modes = json_object_new_object();
    json_object_object_add(infer_modes, "RKLLM_INFER_GENERATE", json_object_new_int(RKLLM_INFER_GENERATE));
    json_object_object_add(infer_modes, "RKLLM_INFER_GET_LAST_HIDDEN_LAYER", json_object_new_int(RKLLM_INFER_GET_LAST_HIDDEN_LAYER));
    json_object_object_add(infer_modes, "RKLLM_INFER_GET_LOGITS", json_object_new_int(RKLLM_INFER_GET_LOGITS));
    json_object_object_add(constants, "INFER_MODES", infer_modes);
    
    return constants;
}