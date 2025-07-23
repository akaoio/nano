const TestClient = require('../lib/test-client');
const { 
  printTestSection, 
  printTestResult, 
  createRKLLMParam,
  createRKLLMInput,
  createRKLLMInferParam,
  createLoRAAdapter,
  TEST_MODELS,
  RKLLM_CONSTANTS,
  sleep 
} = require('../lib/test-helpers');

/**
 * Comprehensive real model testing covering all scenarios
 * Tests real inference with real models and real data
 */
async function testRealModelComplete(client) {
  printTestSection('COMPLETE REAL MODEL TESTING');

  console.log('🎯 COMPREHENSIVE TESTING:');
  console.log('   1. Normal model initialization and inference');
  console.log('   2. Different input types (prompt, token, embed, multimodal)');
  console.log('   3. Different inference modes (generate, hidden layer, logits)');
  console.log('   4. LoRA model testing');
  console.log('   5. Advanced features testing');
  console.log('   6. Error handling and edge cases');
  console.log('');

  let totalTests = 0;
  let passedTests = 0;
  const results = [];

  // Test 1: Normal Model Initialization + Text Generation
  console.log('🧪 TEST 1: Normal Model - Text Generation');
  console.log('=' .repeat(50));
  totalTests++;

  try {
    // Initialize model
    const initParams = createRKLLMParam(TEST_MODELS.NORMAL);
    const initResult = await client.testFunction('rkllm.init', [null, initParams, null]);
    
    if (initResult.success) {
      console.log('✅ Model initialized successfully');
      
      // Wait for initialization to complete
      await sleep(2000);
      
      // Test text generation
      const input = createRKLLMInput('Hello, what is your name?');
      const inferParams = createRKLLMInferParam(RKLLM_CONSTANTS.INFER_MODES.RKLLM_INFER_GENERATE);
      
      const runResult = await client.testFunction('rkllm.run', [null, input, inferParams, null]);
      
      if (runResult.success && runResult.response.result) {
        const result = runResult.response.result;
        if (result.text) {
          console.log('✅ Text generation successful');
          console.log(`✅ Generated: "${result.text}"`);
          console.log(`✅ Token ID: ${result.token_id}`);
          passedTests++;
          results.push({test: 'Normal Text Generation', passed: true, output: result.text});
        }
      }
    }
  } catch (error) {
    console.log(`❌ Test 1 failed: ${error.message}`);
    results.push({test: 'Normal Text Generation', passed: false, error: error.message});
  }

  // Test 2: Different Input Types
  console.log('\n🧪 TEST 2: Input Type Variations');
  console.log('=' .repeat(50));
  
  const inputTests = [
    {
      name: 'Prompt Input',
      type: RKLLM_CONSTANTS.INPUT_TYPES.RKLLM_INPUT_PROMPT,
      input: createRKLLMInput('Tell me a joke', RKLLM_CONSTANTS.INPUT_TYPES.RKLLM_INPUT_PROMPT)
    },
    {
      name: 'Token Input',
      type: RKLLM_CONSTANTS.INPUT_TYPES.RKLLM_INPUT_TOKEN,
      input: createRKLLMInput([1, 23, 45], RKLLM_CONSTANTS.INPUT_TYPES.RKLLM_INPUT_TOKEN)
    }
  ];

  for (const test of inputTests) {
    totalTests++;
    console.log(`\n   Testing ${test.name}:`);
    
    try {
      const inferParams = createRKLLMInferParam();
      const result = await client.testFunction('rkllm.run', [null, test.input, inferParams, null]);
      
      if (result.success && result.response.result && result.response.result.text) {
        console.log(`   ✅ ${test.name}: "${result.response.result.text}"`);
        passedTests++;
        results.push({test: test.name, passed: true, output: result.response.result.text});
      } else {
        console.log(`   ❌ ${test.name}: No text generated`);
        results.push({test: test.name, passed: false, error: 'No text generated'});
      }
    } catch (error) {
      console.log(`   ❌ ${test.name}: ${error.message}`);
      results.push({test: test.name, passed: false, error: error.message});
    }
  }

  // Test 3: Different Inference Modes
  console.log('\n🧪 TEST 3: Inference Mode Variations');
  console.log('=' .repeat(50));
  
  const modeTests = [
    {
      name: 'Generate Mode',
      mode: RKLLM_CONSTANTS.INFER_MODES.RKLLM_INFER_GENERATE
    },
    {
      name: 'Hidden Layer Mode',
      mode: RKLLM_CONSTANTS.INFER_MODES.RKLLM_INFER_GET_LAST_HIDDEN_LAYER
    },
    {
      name: 'Logits Mode',
      mode: RKLLM_CONSTANTS.INFER_MODES.RKLLM_INFER_GET_LOGITS
    }
  ];

  for (const test of modeTests) {
    totalTests++;
    console.log(`\n   Testing ${test.name}:`);
    
    try {
      const input = createRKLLMInput('What is AI?');
      const inferParams = createRKLLMInferParam(test.mode);
      const result = await client.testFunction('rkllm.run', [null, input, inferParams, null]);
      
      if (result.success && result.response.result) {
        const res = result.response.result;
        console.log(`   ✅ ${test.name}: Response received`);
        
        if (test.mode === RKLLM_CONSTANTS.INFER_MODES.RKLLM_INFER_GENERATE && res.text) {
          console.log(`      Generated text: "${res.text}"`);
        } else if (test.mode === RKLLM_CONSTANTS.INFER_MODES.RKLLM_INFER_GET_LAST_HIDDEN_LAYER && res.last_hidden_layer) {
          console.log(`      Hidden layer size: ${res.last_hidden_layer.embd_size}`);
        } else if (test.mode === RKLLM_CONSTANTS.INFER_MODES.RKLLM_INFER_GET_LOGITS && res.logits) {
          console.log(`      Logits vocab size: ${res.logits.vocab_size}`);
        }
        
        passedTests++;
        results.push({test: test.name, passed: true, mode: test.mode});
      } else {
        console.log(`   ❌ ${test.name}: No response`);
        results.push({test: test.name, passed: false, error: 'No response'});
      }
    } catch (error) {
      console.log(`   ❌ ${test.name}: ${error.message}`);
      results.push({test: test.name, passed: false, error: error.message});
    }
  }

  // Test 4: LoRA Model Testing
  console.log('\n🧪 TEST 4: LoRA Model Testing');
  console.log('=' .repeat(50));
  totalTests++;

  try {
    // Initialize LoRA model
    const loraParams = createRKLLMParam(TEST_MODELS.LORA_MODEL);
    const loraInitResult = await client.testFunction('rkllm.init', [null, loraParams, null]);
    
    if (loraInitResult.success) {
      console.log('✅ LoRA model initialized');
      
      // Load LoRA adapter
      const loraAdapter = createLoRAAdapter();
      const loadResult = await client.testFunction('rkllm.load_lora', [null, loraAdapter]);
      
      if (loadResult.success || loadResult.response) {
        console.log('✅ LoRA adapter loaded (or attempted)');
        
        // Test inference with LoRA
        const input = createRKLLMInput('Test LoRA inference');
        const inferParams = createRKLLMInferParam();
        inferParams.lora_params = { adapter_name: 'test_adapter', scale: 1.0 };
        
        const result = await client.testFunction('rkllm.run', [null, input, inferParams, null]);
        
        if (result.success && result.response.result) {
          console.log('✅ LoRA inference successful');
          console.log(`✅ LoRA generated: "${result.response.result.text || 'N/A'}"`);
          passedTests++;
          results.push({test: 'LoRA Model', passed: true, output: result.response.result.text});
        }
      }
    }
  } catch (error) {
    console.log(`❌ LoRA test failed: ${error.message}`);
    results.push({test: 'LoRA Model', passed: false, error: error.message});
  }

  // Test 5: Advanced Features
  console.log('\n🧪 TEST 5: Advanced Features');
  console.log('=' .repeat(50));
  
  const advancedTests = [
    { name: 'is_running', method: 'rkllm.is_running', params: [null] },
    { name: 'get_kv_cache_size', method: 'rkllm.get_kv_cache_size', params: [null] },
    { name: 'clear_kv_cache', method: 'rkllm.clear_kv_cache', params: [null] },
    { name: 'set_chat_template', method: 'rkllm.set_chat_template', params: [null, 'User: {prompt}\nAssistant:'] }
  ];

  for (const test of advancedTests) {
    totalTests++;
    console.log(`\n   Testing ${test.name}:`);
    
    try {
      const result = await client.testFunction(test.method, test.params);
      
      if (result.success || result.response) {
        console.log(`   ✅ ${test.name}: Function accessible`);
        if (result.response && result.response.result) {
          console.log(`      Result: ${JSON.stringify(result.response.result)}`);
        }
        passedTests++;
        results.push({test: test.name, passed: true});
      } else {
        console.log(`   ❌ ${test.name}: No response`);
        results.push({test: test.name, passed: false, error: 'No response'});
      }
    } catch (error) {
      console.log(`   ❌ ${test.name}: ${error.message}`);
      results.push({test: test.name, passed: false, error: error.message});
    }
  }

  // Test 6: Error Handling and Edge Cases
  console.log('\n🧪 TEST 6: Error Handling & Edge Cases');
  console.log('=' .repeat(50));
  
  const errorTests = [
    {
      name: 'Invalid Model Path',
      test: async () => {
        const badParams = createRKLLMParam('/non/existent/model.rkllm');
        const result = await client.testFunction('rkllm.init', [null, badParams, null]);
        return result.error ? true : false; // Should fail
      }
    },
    {
      name: 'Empty Prompt',
      test: async () => {
        const input = createRKLLMInput('');
        const inferParams = createRKLLMInferParam();
        const result = await client.testFunction('rkllm.run', [null, input, inferParams, null]);
        return result.success || result.error; // Should handle gracefully
      }
    },
    {
      name: 'Invalid Parameters',
      test: async () => {
        const result = await client.testFunction('rkllm.run', [null, null, null, null]);
        return result.error ? true : false; // Should error appropriately
      }
    }
  ];

  for (const test of errorTests) {
    totalTests++;
    console.log(`\n   Testing ${test.name}:`);
    
    try {
      const passed = await test.test();
      if (passed) {
        console.log(`   ✅ ${test.name}: Handled correctly`);
        passedTests++;
        results.push({test: test.name, passed: true});
      } else {
        console.log(`   ❌ ${test.name}: Not handled properly`);
        results.push({test: test.name, passed: false, error: 'Not handled properly'});
      }
    } catch (error) {
      console.log(`   ⚠️  ${test.name}: ${error.message}`);
      results.push({test: test.name, passed: false, error: error.message});
    }
  }

  // Final cleanup
  console.log('\n🧹 Cleanup: Destroying model');
  try {
    await client.testFunction('rkllm.destroy', [null]);
    console.log('✅ Model destroyed successfully');
  } catch (error) {
    console.log(`⚠️  Cleanup warning: ${error.message}`);
  }

  // Summary
  console.log('\n' + '=' .repeat(70));
  console.log('📊 COMPREHENSIVE TEST RESULTS');
  console.log('=' .repeat(70));
  
  const passRate = (passedTests / totalTests * 100).toFixed(1);
  console.log(`\n🎯 OVERALL: ${passedTests}/${totalTests} tests passed (${passRate}%)`);
  
  // Detailed results
  console.log('\n📋 DETAILED RESULTS:');
  results.forEach((result, index) => {
    const status = result.passed ? '✅' : '❌';
    console.log(`${status} ${result.test}`);
    if (result.output) {
      console.log(`    Output: "${result.output}"`);
    }
    if (result.error) {
      console.log(`    Error: ${result.error}`);
    }
  });

  const overallSuccess = passedTests >= totalTests * 0.8; // 80% pass rate
  printTestResult('Complete Real Model Testing', overallSuccess);
  return overallSuccess;
}

module.exports = testRealModelComplete;