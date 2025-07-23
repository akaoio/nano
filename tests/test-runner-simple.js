#!/usr/bin/env node

/**
 * Simple Sequential Test Runner - Proper Test Execution
 * Ensures tests run one at a time with clean state between tests
 */

const ServerManager = require('./lib/server-manager');
const TestClient = require('./lib/test-client');
const { createRKLLMParam, createRKLLMInput, createRKLLMInferParam, TEST_MODELS } = require('./lib/test-helpers');

/**
 * Simple test runner with proper sequential execution
 */
async function runSimpleTests() {
  console.log('🚀 RKLLM SIMPLE TEST SUITE - SEQUENTIAL EXECUTION');
  console.log('=' .repeat(60));
  console.log('🎯 Testing with real models and proper sequential execution');
  console.log('');

  const serverManager = new ServerManager();
  const client = new TestClient();
  
  let totalTests = 0;
  let passedTests = 0;
  const results = [];

  try {
    // Start server and connect
    console.log('🔧 SETUP');
    console.log('=' .repeat(30));
    await serverManager.startServer();
    await client.connect();
    console.log('✅ Server ready for testing\n');

    // Test 1: Basic Functions
    console.log('📋 TEST 1: Basic Functions');
    console.log('=' .repeat(40));
    
    totalTests++;
    console.log('\n[1.1] Testing rkllm.createDefaultParam');
    try {
      const result = await client.testFunction('rkllm.createDefaultParam', []);
      if (result.success && result.result && result.result.model_path !== undefined) {
        console.log('✅ createDefaultParam: PASS');
        passedTests++;
        results.push({test: 'createDefaultParam', passed: true});
      } else {
        console.log('❌ createDefaultParam: FAIL');
        results.push({test: 'createDefaultParam', passed: false});
      }
    } catch (error) {
      console.log(`❌ createDefaultParam: ERROR - ${error.message}`);
      results.push({test: 'createDefaultParam', passed: false, error: error.message});
    }

    totalTests++;
    console.log('\n[1.2] Testing rkllm.get_constants');
    try {
      const result = await client.testFunction('rkllm.get_constants', []);
      if (result.success && result.result && result.result.CPU_MASKS) {
        console.log('✅ get_constants: PASS');
        passedTests++;
        results.push({test: 'get_constants', passed: true});
      } else {
        console.log('❌ get_constants: FAIL');
        results.push({test: 'get_constants', passed: false});
      }
    } catch (error) {
      console.log(`❌ get_constants: ERROR - ${error.message}`);
      results.push({test: 'get_constants', passed: false, error: error.message});
    }

    // Test 2: Model Initialization with Real Model
    console.log('\n📋 TEST 2: Real Model Initialization');
    console.log('=' .repeat(40));
    
    totalTests++;
    console.log('\n[2.1] Testing rkllm.init with real model');
    console.log(`Model: ${TEST_MODELS.NORMAL}`);
    
    try {
      const initParams = createRKLLMParam(TEST_MODELS.NORMAL);
      const result = await client.testFunction('rkllm.init', [null, initParams, null]);
      
      if (result.success && result.result) {
        // Check if we got a streaming response (indicates model loaded and started generating)
        if (result.result.text !== undefined || result.result.token_id !== undefined) {
          console.log('✅ init: PASS (Streaming response - model working!)');
          console.log(`   Generated text: "${result.result.text || ''}"`);
          console.log(`   Token ID: ${result.result.token_id || 'N/A'}`);
          passedTests++;
          results.push({test: 'init', passed: true, note: 'Real model loaded and generating'});
        } else if (result.result.success) {
          console.log('✅ init: PASS (Success response)');
          passedTests++;
          results.push({test: 'init', passed: true, note: 'Model initialized'});
        } else {
          console.log('⚠️  init: Unexpected response format');
          results.push({test: 'init', passed: false, note: 'Unexpected format'});
        }
      } else {
        console.log('❌ init: FAIL - No success response');
        results.push({test: 'init', passed: false, note: 'No success response'});
      }
    } catch (error) {
      console.log(`❌ init: ERROR - ${error.message}`);
      results.push({test: 'init', passed: false, error: error.message});
    }

    // Wait for model to be ready
    await new Promise(resolve => setTimeout(resolve, 2000));

    // Test 3: Real Inference
    console.log('\n📋 TEST 3: Real Inference');
    console.log('=' .repeat(40));
    
    totalTests++;
    console.log('\n[3.1] Testing rkllm.run with real inference');
    
    try {
      const input = createRKLLMInput('Hello, what is your name?');
      const inferParams = createRKLLMInferParam();
      const result = await client.testFunction('rkllm.run', [null, input, inferParams, null]);
      
      if (result.success && result.result) {
        if (result.result.text !== undefined) {
          console.log('✅ run: PASS (Real text generated!)');
          console.log(`   Generated: "${result.result.text}"`);
          console.log(`   Token ID: ${result.result.token_id || 'N/A'}`);
          passedTests++;
          results.push({test: 'run', passed: true, output: result.result.text});
        } else {
          console.log('⚠️  run: Response received but no text');
          results.push({test: 'run', passed: false, note: 'No text generated'});
        }
      } else if (result.error) {
        console.log(`⚠️  run: Error (expected if model not initialized) - ${result.error.message}`);
        results.push({test: 'run', passed: false, note: 'Model not initialized'});
      } else {
        console.log('❌ run: No response');
        results.push({test: 'run', passed: false, note: 'No response'});
      }
    } catch (error) {
      console.log(`❌ run: ERROR - ${error.message}`);
      results.push({test: 'run', passed: false, error: error.message});
    }

    // Test 4: Core Function Mapping
    console.log('\n📋 TEST 4: Core Function Mapping');
    console.log('=' .repeat(40));
    
    const coreFunctions = [
      { name: 'is_running', params: [null] },
      { name: 'abort', params: [null] },
      { name: 'run_async', params: [null, createRKLLMInput('test'), createRKLLMInferParam(), null] }
    ];

    for (const func of coreFunctions) {
      totalTests++;
      console.log(`\n[4.${coreFunctions.indexOf(func) + 1}] Testing rkllm.${func.name}`);
      
      try {
        const result = await client.testFunction(`rkllm.${func.name}`, func.params);
        
        // For mapping tests, we just check if the function is recognized (not "Method not found")
        if (result.error && result.error.message === 'Method not found') {
          console.log(`❌ ${func.name}: NOT MAPPED`);
          results.push({test: func.name, passed: false, note: 'Method not found'});
        } else {
          console.log(`✅ ${func.name}: MAPPED`);
          passedTests++;
          results.push({test: func.name, passed: true, note: 'Function mapped'});
        }
      } catch (error) {
        console.log(`❌ ${func.name}: ERROR - ${error.message}`);
        results.push({test: func.name, passed: false, error: error.message});
      }
    }

    // Test 5: Advanced Function Mapping
    console.log('\n📋 TEST 5: Advanced Functions');
    console.log('=' .repeat(40));
    
    const advancedFunctions = [
      { name: 'destroy', params: [null] },
      { name: 'get_kv_cache_size', params: [null] },
      { name: 'clear_kv_cache', params: [null] },
      { name: 'load_lora', params: [null, { lora_adapter_path: '/test/path' }] },
      { name: 'set_chat_template', params: [null, 'User: {prompt}\nAssistant:'] }
    ];

    for (const func of advancedFunctions) {
      totalTests++;
      console.log(`\n[5.${advancedFunctions.indexOf(func) + 1}] Testing rkllm.${func.name}`);
      
      try {
        const result = await client.testFunction(`rkllm.${func.name}`, func.params);
        
        if (result.error && result.error.message === 'Method not found') {
          console.log(`❌ ${func.name}: NOT MAPPED`);
          results.push({test: func.name, passed: false, note: 'Method not found'});
        } else {
          console.log(`✅ ${func.name}: MAPPED`);
          passedTests++;
          results.push({test: func.name, passed: true, note: 'Function mapped'});
        }
      } catch (error) {
        console.log(`❌ ${func.name}: ERROR - ${error.message}`);
        results.push({test: func.name, passed: false, error: error.message});
      }
    }

    // Clean up
    console.log('\n🧹 CLEANUP');
    console.log('=' .repeat(30));
    try {
      await client.ensureCleanState();
      console.log('✅ Model destroyed');
    } catch (error) {
      console.log(`⚠️  Cleanup warning: ${error.message}`);
    }

  } catch (error) {
    console.error('\n❌ TEST SUITE FAILED:', error.message);
  } finally {
    // Always cleanup
    console.log('\n🔌 Disconnecting...');
    client.disconnect();
    serverManager.stopServer();
  }

  // Results Summary
  console.log('\n' + '=' .repeat(60));
  console.log('📊 SIMPLE TEST RESULTS');
  console.log('=' .repeat(60));
  
  const passRate = totalTests > 0 ? (passedTests / totalTests * 100).toFixed(1) : 0;
  console.log(`\n🎯 OVERALL: ${passedTests}/${totalTests} tests passed (${passRate}%)`);
  
  console.log('\n📋 DETAILED RESULTS:');
  results.forEach((result, index) => {
    const status = result.passed ? '✅' : '❌';
    console.log(`${status} ${index + 1}. ${result.test}`);
    
    if (result.output) {
      console.log(`    Generated: "${result.output}"`);
    }
    if (result.note) {
      console.log(`    Note: ${result.note}`);
    }
    if (result.error) {
      console.log(`    Error: ${result.error}`);
    }
  });

  console.log('\n' + '=' .repeat(60));
  
  if (passedTests >= totalTests * 0.8) {
    console.log('🎉 TEST SUITE PASSED! Most functions working correctly.');
  } else if (passedTests >= totalTests * 0.5) {
    console.log('⚠️  TEST SUITE PARTIAL: Some functions need attention.');
  } else {
    console.log('❌ TEST SUITE FAILED: Major issues detected.');
  }
  
  console.log('🏁 Testing complete');
  
  // Exit with appropriate code
  process.exit(passedTests >= totalTests * 0.8 ? 0 : 1);
}

// Handle errors
process.on('uncaughtException', (error) => {
  console.error('\n💥 Uncaught Exception:', error.message);
  process.exit(1);
});

process.on('unhandledRejection', (reason, promise) => {
  console.error('\n💥 Unhandled Rejection:', reason);
  process.exit(1);
});

// Run the simple test suite
if (require.main === module) {
  runSimpleTests();
}

module.exports = runSimpleTests;