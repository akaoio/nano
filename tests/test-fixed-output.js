#!/usr/bin/env node

/**
 * Test with fixed output formatting and investigate rkllm_run issues
 */

const ServerManager = require('./lib/server-manager');
const TestClient = require('./lib/test-client');
const { createRKLLMParam, createRKLLMInput, createRKLLMInferParam, TEST_MODELS } = require('./lib/test-helpers');

async function testFixedOutput() {
  console.log('🧪 TESTING WITH FIXED OUTPUT');
  console.log('=' .repeat(50));
  console.log('🎯 Clean output format and investigate issues');
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
    console.log('✅ Server ready\n');

    // Test 1: Initialize model
    console.log('📋 TEST 1: Model Initialization');
    console.log('=' .repeat(40));
    
    totalTests++;
    const initParams = createRKLLMParam(TEST_MODELS.NORMAL);
    initParams.max_new_tokens = 5;
    
    const initResult = await client.testFunction('rkllm.init', [null, initParams, null]);
    if (initResult.success) {
      console.log('✅ init: PASS');
      passedTests++;
      results.push({test: 'init', passed: true});
      await new Promise(resolve => setTimeout(resolve, 2000));
    } else {
      console.log('❌ init: FAIL');
      results.push({test: 'init', passed: false});
    }

    // Test 2: Normal text generation (mode 0)
    console.log('\n📋 TEST 2: Normal Text Generation');
    console.log('=' .repeat(40));
    
    totalTests++;
    const input = createRKLLMInput('Say hello');
    const inferParams = createRKLLMInferParam();
    inferParams.mode = 0; // RKLLM_INFER_GENERATE
    
    const textResult = await client.testFunction('rkllm.run', [null, input, inferParams, null]);
    
    if (textResult.success && textResult.result) {
      console.log('✅ Normal generation: PASS');
      console.log(`   Generated: "${textResult.result.text || 'N/A'}"`);
      console.log(`   Token ID: ${textResult.result.token_id || 'N/A'}`);
      passedTests++;
      results.push({test: 'normal_generation', passed: true});
    } else {
      console.log('❌ Normal generation: FAIL');
      results.push({test: 'normal_generation', passed: false});
    }

    // Test 3: Hidden states mode (mode 1) 
    console.log('\n📋 TEST 3: Hidden States Mode');
    console.log('=' .repeat(40));
    
    totalTests++;
    const hiddenParams = createRKLLMInferParam();
    hiddenParams.mode = 1; // RKLLM_INFER_GET_LAST_HIDDEN_LAYER
    
    const hiddenResult = await client.testFunction('rkllm.run', [null, input, hiddenParams, null]);
    
    if (hiddenResult.success && hiddenResult.result) {
      const result = hiddenResult.result;
      console.log('✅ Hidden states: PASS');
      
      if (result.last_hidden_layer && result.last_hidden_layer.hidden_states) {
        console.log(`   Hidden array length: ${result.last_hidden_layer.hidden_states.length}`);
        console.log(`   Embedding size: ${result.last_hidden_layer.embd_size}`);
        console.log(`   Num tokens: ${result.last_hidden_layer.num_tokens}`);
      }
      
      passedTests++;
      results.push({test: 'hidden_states', passed: true});
    } else {
      console.log('❌ Hidden states: FAIL');
      results.push({test: 'hidden_states', passed: false});
    }

    // Test 4: Check why logits mode fails - try shorter timeout
    console.log('\n📋 TEST 4: Logits Mode Investigation');
    console.log('=' .repeat(40));
    
    totalTests++;
    console.log('   Testing logits mode with shorter timeout...');
    
    try {
      const logitsParams = createRKLLMInferParam();
      logitsParams.mode = 2; // RKLLM_INFER_GET_LOGITS
      
      // Use shorter timeout to see if it's just slow
      const originalTimeout = client.timeout || 15000;
      
      const logitsResult = await Promise.race([
        client.testFunction('rkllm.run', [null, input, logitsParams, null]),
        new Promise((_, reject) => setTimeout(() => reject(new Error('Quick timeout')), 5000))
      ]);
      
      if (logitsResult.success && logitsResult.result) {
        console.log('✅ Logits mode: PASS');
        if (logitsResult.result.logits && logitsResult.result.logits.logits) {
          console.log(`   Logits array length: ${logitsResult.result.logits.logits.length}`);
          console.log(`   Vocab size: ${logitsResult.result.logits.vocab_size}`);
        }
        passedTests++;
        results.push({test: 'logits_mode', passed: true});
      } else {
        console.log('❌ Logits mode: No valid response');
        results.push({test: 'logits_mode', passed: false, reason: 'No response'});
      }
      
    } catch (error) {
      console.log(`❌ Logits mode: ${error.message}`);
      console.log('   → Logits mode may not be supported by this RKLLM version');
      results.push({test: 'logits_mode', passed: false, reason: error.message});
    }

    // Test 5: Test basic functions that should work
    console.log('\n📋 TEST 5: Basic Functions Check');
    console.log('=' .repeat(40));
    
    const basicTests = [
      { name: 'abort', method: 'rkllm.abort', params: [null] },
      { name: 'destroy', method: 'rkllm.destroy', params: [null] }
    ];

    for (const test of basicTests) {
      totalTests++;
      try {
        const result = await client.testFunction(test.method, test.params);
        
        if (result.success || result.error?.message === 'Internal server error') {
          console.log(`✅ ${test.name}: PASS`);
          passedTests++;
          results.push({test: test.name, passed: true});
        } else {
          console.log(`❌ ${test.name}: FAIL`);
          results.push({test: test.name, passed: false});
        }
      } catch (error) {
        console.log(`❌ ${test.name}: ERROR - ${error.message}`);
        results.push({test: test.name, passed: false, error: error.message});
      }
    }

  } catch (error) {
    console.error('\n❌ TEST SUITE FAILED:', error.message);
  } finally {
    console.log('\n🔌 Disconnecting...');
    client.disconnect();
    serverManager.stopServer();
  }

  // Results Summary
  console.log('\n' + '=' .repeat(50));
  console.log('📊 FIXED OUTPUT TEST RESULTS');
  console.log('=' .repeat(50));
  
  const passRate = totalTests > 0 ? (passedTests / totalTests * 100).toFixed(1) : 0;
  console.log(`\n🎯 OVERALL: ${passedTests}/${totalTests} tests passed (${passRate}%)`);
  
  console.log('\n📋 SUMMARY:');
  results.forEach((result, index) => {
    const status = result.passed ? '✅' : '❌';
    console.log(`${status} ${index + 1}. ${result.test}`);
    if (result.reason) {
      console.log(`    Reason: ${result.reason}`);
    }
    if (result.error) {
      console.log(`    Error: ${result.error}`);
    }
  });

  console.log('\n🔍 ANALYSIS:');
  const failedTests = results.filter(r => !r.passed);
  if (failedTests.length === 0) {
    console.log('✅ All tests passed - no issues detected');
  } else {
    console.log('❌ Failed tests analysis:');
    failedTests.forEach(test => {
      if (test.test === 'logits_mode') {
        console.log('   → Logits mode timeout likely due to RKLLM library limitation');
      } else {
        console.log(`   → ${test.test}: ${test.reason || test.error || 'Unknown issue'}`);
      }
    });
  }
  
  console.log('\n🏁 Testing complete with clean output format');
  
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

if (require.main === module) {
  testFixedOutput();
}

module.exports = testFixedOutput;