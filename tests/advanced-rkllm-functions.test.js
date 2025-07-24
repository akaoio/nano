#!/usr/bin/env node

/**
 * Advanced RKLLM Functions Test Suite
 * Tests all the advanced RKLLM functions not covered in basic tests:
 * - LoRA adapter loading/management
 * - Prompt caching system
 * - KV cache management
 * - Chat template configuration
 * - Function calling tools
 * - Cross-attention parameters
 */

const ServerManager = require('./lib/server-manager');
const TestClient = require('./lib/test-client');
const { 
  createRKLLMParam, 
  createRKLLMInput, 
  createRKLLMInferParam,
  createLoRAAdapter,
  createCrossAttnParams,
  TEST_MODELS,
  RKLLM_CONSTANTS,
  printTestSection,
  sleep
} = require('./lib/test-helpers');

class AdvancedRKLLMTests {
  constructor() {
    this.serverManager = new ServerManager();
    this.client = new TestClient();
    this.testResults = {
      passed: 0,
      failed: 0,
      total: 0
    };
  }

  async runTest(name, testFunction) {
    this.testResults.total++;
    process.stdout.write(`📋 ${name}... `);
    
    try {
      await testFunction();
      console.log('✅ PASS');
      this.testResults.passed++;
    } catch (error) {
      console.log('❌ FAIL');
      console.log(`   Error: ${error.message}`);
      this.testResults.failed++;
    }
  }

  async setup() {
    printTestSection('ADVANCED RKLLM FUNCTIONS TEST SUITE');
    console.log('🎯 Testing advanced RKLLM functionality not covered in basic tests');
    console.log('');

    console.log('🔧 SETUP');
    console.log('=' .repeat(30));
    await this.serverManager.startServer();
    await this.client.connect();
    console.log('✅ Server ready for advanced RKLLM tests\n');
  }

  async teardown() {
    console.log('\n🧹 CLEANUP');
    try {
      await this.client.ensureCleanState();
      this.client.disconnect();
      this.serverManager.stopServer();
      console.log('✅ Cleanup completed');
    } catch (error) {
      console.log(`⚠️  Cleanup warning: ${error.message}`);
    }
  }

  async testLoRAAdapterLoading() {
    // Initialize base model first
    const baseParams = createRKLLMParam(TEST_MODELS.LORA_MODEL);
    const initResult = await this.client.sendRequest('rkllm.init', [null, baseParams, null]);
    if (!initResult.result || !initResult.result.success) {
      throw new Error('Base model initialization failed');
    }
    await sleep(2000);

    const loraAdapter = createLoRAAdapter('test_lora');
    const result = await this.client.sendRequest('rkllm.load_lora', [null, loraAdapter]);
    
    if (!result.result || !result.result.success) {
      throw new Error('LoRA adapter loading failed');
    }

    console.log(`      ✅ LoRA adapter "${loraAdapter.lora_adapter_name}" loaded successfully`);
    
    // Cleanup
    await this.client.sendRequest('rkllm.destroy', [null]);
  }

  async testPromptCacheSystem() {
    // Initialize model
    const params = createRKLLMParam();
    const initResult = await this.client.sendRequest('rkllm.init', [null, params, null]);
    if (!initResult.result || !initResult.result.success) {
      throw new Error('Model initialization failed');
    }
    await sleep(2000);

    // Test load_prompt_cache
    const cacheParams = {
      cache_path: '/tmp/test_prompt_cache.bin',
      prompt_text: 'This is a test prompt for caching.'
    };
    
    const loadResult = await this.client.sendRequest('rkllm.load_prompt_cache', [null, cacheParams]);
    if (!loadResult.result || !loadResult.result.success) {
      throw new Error('Prompt cache loading failed');
    }

    console.log(`      ✅ Prompt cache loaded: ${cacheParams.cache_path}`);

    // Test release_prompt_cache
    const releaseResult = await this.client.sendRequest('rkllm.release_prompt_cache', [null]);
    if (!releaseResult.result || !releaseResult.result.success) {
      throw new Error('Prompt cache release failed');
    }

    console.log(`      ✅ Prompt cache released successfully`);
    
    // Cleanup
    await this.client.sendRequest('rkllm.destroy', [null]);
  }

  async testKVCacheManagement() {
    // Initialize model
    const params = createRKLLMParam();
    const initResult = await this.client.sendRequest('rkllm.init', [null, params, null]);
    if (!initResult.result || !initResult.result.success) {
      throw new Error('Model initialization failed');
    }
    await sleep(2000);

    // Test get_kv_cache_size
    const cacheSizeResult = await this.client.sendRequest('rkllm.get_kv_cache_size', [null]);
    if (!cacheSizeResult.result) {
      throw new Error('KV cache size query failed');
    }

    console.log(`      ✅ KV cache size: ${cacheSizeResult.result.cache_size} bytes`);

    // Test clear_kv_cache
    const clearParams = {
      keep_system_prompt: 1,
      start_pos: [0, 1, 2],
      end_pos: [10, 11, 12]
    };
    
    const clearResult = await this.client.sendRequest('rkllm.clear_kv_cache', [null, clearParams.keep_system_prompt, clearParams.start_pos, clearParams.end_pos]);
    if (!clearResult.result || !clearResult.result.success) {
      throw new Error('KV cache clearing failed');
    }

    console.log(`      ✅ KV cache cleared successfully`);
    
    // Cleanup
    await this.client.sendRequest('rkllm.destroy', [null]);
  }

  async testChatTemplateConfiguration() {
    // Initialize model
    const params = createRKLLMParam();
    const initResult = await this.client.sendRequest('rkllm.init', [null, params, null]);
    if (!initResult.result || !initResult.result.success) {
      throw new Error('Model initialization failed');
    }
    await sleep(2000);

    const templateConfig = {
      chat_template: "<|im_start|>system\nYou are a helpful assistant.<|im_end|>\n<|im_start|>user\n{prompt}<|im_end|>\n<|im_start|>assistant\n",
      stop_words: ["<|im_end|>", "<|endoftext|>"],
      add_generation_prompt: true
    };

    const result = await this.client.sendRequest('rkllm.set_chat_template', [null, templateConfig]);
    if (!result.result || !result.result.success) {
      throw new Error('Chat template configuration failed');
    }

    console.log(`      ✅ Chat template configured successfully`);
    
    // Cleanup
    await this.client.sendRequest('rkllm.destroy', [null]);
  }

  async testFunctionCallingTools() {
    // Initialize model
    const params = createRKLLMParam();
    const initResult = await this.client.sendRequest('rkllm.init', [null, params, null]);
    if (!initResult.result || !initResult.result.success) {
      throw new Error('Model initialization failed');
    }
    await sleep(2000);

    const toolsConfig = {
      functions: [
        {
          name: "get_weather",
          description: "Get the current weather for a location",
          parameters: {
            type: "object",
            properties: {
              location: {
                type: "string",
                description: "The city name"
              }
            },
            required: ["location"]
          }
        }
      ],
      tool_choice: "auto"
    };

    const result = await this.client.sendRequest('rkllm.set_function_tools', [null, toolsConfig]);
    if (!result.result || !result.result.success) {
      throw new Error('Function tools configuration failed');
    }

    console.log(`      ✅ Function calling tools configured (${toolsConfig.functions.length} functions)`);
    
    // Cleanup
    await this.client.sendRequest('rkllm.destroy', [null]);
  }

  async testCrossAttentionParameters() {
    // Initialize model
    const params = createRKLLMParam();
    const initResult = await this.client.sendRequest('rkllm.init', [null, params, null]);
    if (!initResult.result || !initResult.result.success) {
      throw new Error('Model initialization failed');
    }
    await sleep(2000);

    const crossAttnParams = createCrossAttnParams(128);
    const result = await this.client.sendRequest('rkllm.set_cross_attn_params', [null, crossAttnParams]);
    
    if (!result.result || !result.result.success) {
      throw new Error('Cross-attention parameters configuration failed');
    }

    console.log(`      ✅ Cross-attention parameters set (${crossAttnParams.num_tokens} tokens)`);
    
    // Cleanup
    await this.client.sendRequest('rkllm.destroy', [null]);
  }

  async testAdvancedInferenceModes() {
    // Initialize model
    const params = createRKLLMParam();
    const initResult = await this.client.sendRequest('rkllm.init', [null, params, null]);
    if (!initResult.result || !initResult.result.success) {
      throw new Error('Model initialization failed');
    }
    await sleep(2000);

    // Test logits extraction mode
    const input = createRKLLMInput('Test logits extraction.');
    const inferParams = createRKLLMInferParam(RKLLM_CONSTANTS.INFER_MODES.RKLLM_INFER_GET_LOGITS);
    
    const result = await this.client.sendRequest('rkllm.run', [null, input, inferParams, null]);
    
    if (!result.result) {
      throw new Error('Logits extraction failed');
    }

    if (result.result.logits && result.result.logits.vocab_size > 0) {
      console.log(`      ✅ Logits extracted: vocab_size=${result.result.logits.vocab_size}, tokens=${result.result.logits.num_tokens}`);
    } else {
      console.log(`      ⚠️  Logits extraction completed but no logits data returned`);
    }
    
    // Cleanup
    await this.client.sendRequest('rkllm.destroy', [null]);
  }

  async testMultimodalInput() {
    // Initialize model
    const params = createRKLLMParam();
    const initResult = await this.client.sendRequest('rkllm.init', [null, params, null]);
    if (!initResult.result || !initResult.result.success) {
      throw new Error('Model initialization failed');
    }
    await sleep(2000);

    const multimodalInput = createRKLLMInput('Describe this image.', RKLLM_CONSTANTS.INPUT_TYPES.RKLLM_INPUT_MULTIMODAL);
    const inferParams = createRKLLMInferParam();
    
    const result = await this.client.sendRequest('rkllm.run', [null, multimodalInput, inferParams, null]);
    
    if (!result.result) {
      throw new Error('Multimodal input processing failed');
    }

    console.log(`      ✅ Multimodal input processed: "${result.result.text}"`);
    
    // Cleanup
    await this.client.sendRequest('rkllm.destroy', [null]);
  }

  async runAllTests() {
    try {
      await this.setup();

      // Run all advanced tests
      await this.runTest('LoRA Adapter Loading', () => this.testLoRAAdapterLoading());
      await this.runTest('Prompt Cache System', () => this.testPromptCacheSystem());
      await this.runTest('KV Cache Management', () => this.testKVCacheManagement());
      await this.runTest('Chat Template Configuration', () => this.testChatTemplateConfiguration());
      await this.runTest('Function Calling Tools', () => this.testFunctionCallingTools());
      await this.runTest('Cross-Attention Parameters', () => this.testCrossAttentionParameters());
      await this.runTest('Advanced Inference Modes', () => this.testAdvancedInferenceModes());
      await this.runTest('Multimodal Input Processing', () => this.testMultimodalInput());

    } catch (error) {
      console.error(`\n❌ ADVANCED RKLLM TESTS FAILED: ${error.message}`);
      process.exit(1);
    } finally {
      await this.teardown();
    }

    // Results summary
    console.log('\n' + '=' .repeat(60));
    console.log(`🎉 ADVANCED RKLLM TESTS COMPLETED`);
    console.log(`✅ Passed: ${this.testResults.passed}/${this.testResults.total}`);
    console.log(`❌ Failed: ${this.testResults.failed}/${this.testResults.total}`);
    
    if (this.testResults.failed === 0) {
      console.log('🚀 All advanced RKLLM functions working correctly!');
      process.exit(0);
    } else {
      console.log('⚠️  Some advanced functions need attention');
      process.exit(1);
    }
  }
}

// Run tests if called directly
if (require.main === module) {
  const tests = new AdvancedRKLLMTests();
  tests.runAllTests();
}

module.exports = AdvancedRKLLMTests;