/**
 * Test Helpers - Common utilities for RKLLM server testing
 */

/**
 * RKLLM Constants from DESIGN.md - Exact mapping
 */
const RKLLM_CONSTANTS = {
  // Input Types
  INPUT_TYPES: {
    RKLLM_INPUT_PROMPT: 0,
    RKLLM_INPUT_TOKEN: 1,
    RKLLM_INPUT_EMBED: 2,
    RKLLM_INPUT_MULTIMODAL: 3
  },

  // Inference Modes
  INFER_MODES: {
    RKLLM_INFER_GENERATE: 0,
    RKLLM_INFER_GET_LAST_HIDDEN_LAYER: 1,
    RKLLM_INFER_GET_LOGITS: 2
  },

  // Call States
  CALL_STATES: {
    RKLLM_RUN_NORMAL: 0,
    RKLLM_RUN_WAITING: 1,
    RKLLM_RUN_FINISH: 2,
    RKLLM_RUN_ERROR: 3
  }
};

/**
 * Model paths for testing (from PROMPT.md)
 */
const TEST_MODELS = {
  NORMAL: '/home/x/Projects/nano/models/qwen3/model.rkllm',
  LORA_MODEL: '/home/x/Projects/nano/models/lora/model.rkllm',
  LORA_ADAPTER: '/home/x/Projects/nano/models/lora/lora.rkllm'
};

/**
 * Create standard RKLLMParam structure (DESIGN.md lines 173-204)
 */
function createRKLLMParam(modelPath = TEST_MODELS.NORMAL) {
  return {
    model_path: modelPath,
    max_context_len: 512,
    max_new_tokens: 10, // Small for testing
    top_k: 40,
    n_keep: 0,
    top_p: 0.9,
    temperature: 0.8,
    repeat_penalty: 1.1,
    frequency_penalty: 0.0,
    presence_penalty: 0.0,
    mirostat: 0,
    mirostat_tau: 5.0,
    mirostat_eta: 0.1,
    skip_special_token: false,
    is_async: false,
    img_start: null,
    img_end: null,
    img_content: null,
    extend_param: {
      base_domain_id: 0,
      embed_flash: 0,
      enabled_cpus_num: 4,
      enabled_cpus_mask: 15,
      n_batch: 1,
      use_cross_attn: 0,
      reserved: null
    }
  };
}

/**
 * Create standard RKLLMInput structure (DESIGN.md lines 206-234)
 */
function createRKLLMInput(prompt = 'Hello', inputType = RKLLM_CONSTANTS.INPUT_TYPES.RKLLM_INPUT_PROMPT) {
  const input = {
    role: 'user',
    enable_thinking: false,
    input_type: inputType
  };

  switch (inputType) {
    case RKLLM_CONSTANTS.INPUT_TYPES.RKLLM_INPUT_PROMPT:
      input.prompt_input = prompt;
      break;
    case RKLLM_CONSTANTS.INPUT_TYPES.RKLLM_INPUT_TOKEN:
      input.token_input = {
        input_ids: [1, 2, 3, 4],
        n_tokens: 4
      };
      break;
    case RKLLM_CONSTANTS.INPUT_TYPES.RKLLM_INPUT_EMBED:
      input.embed_input = {
        embed: [0.1, 0.2, 0.3],
        n_tokens: 3
      };
      break;
    case RKLLM_CONSTANTS.INPUT_TYPES.RKLLM_INPUT_MULTIMODAL:
      input.multimodal_input = {
        prompt: prompt,
        image_embed: [0.1, 0.2, 0.3],
        n_image_tokens: 196,
        n_image: 1,
        image_width: 224,
        image_height: 224
      };
      break;
  }

  return input;
}

/**
 * Create standard RKLLMInferParam structure (DESIGN.md lines 236-249)
 */
function createRKLLMInferParam(mode = RKLLM_CONSTANTS.INFER_MODES.RKLLM_INFER_GENERATE) {
  return {
    mode: mode,
    lora_params: null,
    prompt_cache_params: null,
    keep_history: 0
  };
}

/**
 * Create LoRA adapter structure (DESIGN.md lines 251-258)
 */
function createLoRAAdapter(name = 'test_adapter') {
  return {
    lora_adapter_path: TEST_MODELS.LORA_ADAPTER,
    lora_adapter_name: name,
    scale: 1.0
  };
}

/**
 * Create cross-attention parameters (DESIGN.md lines 260-269)
 */
function createCrossAttnParams(numTokens = 128) {
  return {
    encoder_k_cache: [0.1, 0.2, 0.3],
    encoder_v_cache: [0.4, 0.5, 0.6],
    encoder_mask: [1.0, 1.0, 0.0],
    encoder_pos: [0, 1, 2],
    num_tokens: numTokens
  };
}

/**
 * Pretty print test results
 */
function printTestResult(testName, success, details = '') {
  const status = success ? '✅' : '❌';
  const result = success ? 'PASS' : 'FAIL';
  console.log(`${status} ${testName}: ${result}${details ? ' - ' + details : ''}`);
}

/**
 * Print test section header
 */
function printTestSection(title) {
  console.log('\\n' + '='.repeat(60));
  console.log(`🧪 ${title}`);
  console.log('='.repeat(60));
}

/**
 * Sleep utility for async operations
 */
function sleep(ms) {
  return new Promise(resolve => setTimeout(resolve, ms));
}

module.exports = {
  RKLLM_CONSTANTS,
  TEST_MODELS,
  createRKLLMParam,
  createRKLLMInput,
  createRKLLMInferParam,
  createLoRAAdapter,
  createCrossAttnParams,
  printTestResult,
  printTestSection,
  sleep
};