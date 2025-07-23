const TestClient = require('../lib/test-client');
const { 
  printTestSection, 
  printTestResult, 
  createRKLLMParam,
  createRKLLMInput,
  createRKLLMInferParam,
  TEST_MODELS,
  RKLLM_CONSTANTS,
  sleep 
} = require('../lib/test-helpers');

/**
 * Test async operations and streaming behaviors
 * Validates real-time streaming, async inference, and callback handling
 */
async function testAsyncOperations(client) {
  printTestSection('ASYNC OPERATIONS & STREAMING');

  console.log('⚡ ASYNC TESTING:');
  console.log('   - Async vs Sync inference modes');
  console.log('   - Real-time streaming responses');
  console.log('   - Callback state validation');
  console.log('   - Concurrent inference operations');
  console.log('   - Abort operations');
  console.log('');

  let totalTests = 0;
  let passedTests = 0;
  const results = [];

  // Initialize model for async testing
  console.log('🔧 Setting up model for async testing...');
  const initParams = createRKLLMParam(TEST_MODELS.NORMAL);
  const initResult = await client.testFunction('rkllm.init', [null, initParams, null]);
  
  if (!initResult.success && !initResult.response) {
    console.log('❌ Failed to initialize model for async testing');
    printTestResult('Async Operations', false);
    return false;
  }
  
  console.log('✅ Model ready for async testing');
  await sleep(1000);

  // Test 1: Sync vs Async Inference
  console.log('\n🧪 TEST 1: Sync vs Async Inference');
  console.log('=' .repeat(50));
  
  const syncAsyncTests = [
    {
      name: 'Synchronous Inference',
      async: false,
      method: 'rkllm.run'
    },
    {
      name: 'Asynchronous Inference',
      async: true,
      method: 'rkllm.run_async'
    }
  ];

  for (const test of syncAsyncTests) {
    totalTests++;
    console.log(`\n   Testing: ${test.name}`);
    
    try {
      const input = createRKLLMInput('Explain quantum computing briefly');
      const inferParams = createRKLLMInferParam();
      
      const startTime = Date.now();
      const result = await client.testFunction(test.method, [null, input, inferParams, null]);
      const duration = Date.now() - startTime;
      
      if (result.success && result.response && result.response.result) {
        const res = result.response.result;
        console.log(`   ✅ ${test.name}: Success`);
        console.log(`      Duration: ${duration}ms`);
        console.log(`      Generated: "${res.text || 'N/A'}"`);
        console.log(`      Token ID: ${res.token_id || 'N/A'}`);
        
        if (res._callback_state !== undefined) {
          console.log(`      Callback state: ${res._callback_state}`);
        }
        
        passedTests++;
        results.push({
          test: test.name, 
          passed: true, 
          duration, 
          output: res.text,
          tokenId: res.token_id
        });
      } else {
        console.log(`   ❌ ${test.name}: No response`);
        results.push({test: test.name, passed: false, error: 'No response'});
      }
    } catch (error) {
      console.log(`   ❌ ${test.name}: ${error.message}`);
      results.push({test: test.name, passed: false, error: error.message});
    }
  }

  // Test 2: Streaming Response Validation
  console.log('\n🧪 TEST 2: Streaming Response Structure');
  console.log('=' .repeat(50));
  totalTests++;
  
  try {
    const input = createRKLLMInput('Tell me about artificial intelligence');
    const inferParams = createRKLLMInferParam();
    
    const result = await client.testFunction('rkllm.run', [null, input, inferParams, null]);
    
    if (result.success && result.response && result.response.result) {
      const res = result.response.result;
      
      console.log('\n   🔍 Validating streaming response structure:');
      
      // Check all expected fields
      const expectedFields = [
        'text', 'token_id', 'last_hidden_layer', 'logits', 'perf', '_callback_state'
      ];
      
      let validFields = 0;
      for (const field of expectedFields) {
        if (field in res) {
          console.log(`   ✅ Field present: ${field}`);
          validFields++;
        } else {
          console.log(`   ⚠️  Field missing: ${field}`);
        }
      }
      
      // Validate performance metrics
      if (res.perf) {
        console.log('\n   🔍 Performance metrics:');
        const perfFields = ['prefill_time_ms', 'prefill_tokens', 'generate_time_ms', 'generate_tokens', 'memory_usage_mb'];
        for (const field of perfFields) {
          if (field in res.perf) {
            console.log(`   ✅ Perf ${field}: ${res.perf[field]}`);
          }
        }
      }
      
      if (validFields >= expectedFields.length * 0.8) { // 80% of fields present
        console.log(`\n   ✅ Streaming structure valid: ${validFields}/${expectedFields.length} fields`);
        passedTests++;
        results.push({test: 'Streaming structure', passed: true, fields: validFields});
      } else {
        console.log(`\n   ❌ Streaming structure incomplete: ${validFields}/${expectedFields.length} fields`);
        results.push({test: 'Streaming structure', passed: false, fields: validFields});
      }
    } else {
      console.log('   ❌ No streaming response received');
      results.push({test: 'Streaming structure', passed: false, error: 'No response'});
    }
  } catch (error) {
    console.log(`   ❌ Streaming test error: ${error.message}`);
    results.push({test: 'Streaming structure', passed: false, error: error.message});
  }

  // Test 3: Running Status Check
  console.log('\n🧪 TEST 3: Running Status Validation');
  console.log('=' .repeat(50));
  totalTests++;
  
  try {
    // Start an inference
    const input = createRKLLMInput('Generate a longer response about the history of computers');
    const inferParams = createRKLLMInferParam();
    
    // Check running status before, during, and after
    const beforeStatus = await client.testFunction('rkllm.is_running', [null]);
    console.log(`   Status before inference: ${JSON.stringify(beforeStatus.response?.result || 'unknown')}`);
    
    // Start inference (don't await immediately)
    const inferencePromise = client.testFunction('rkllm.run', [null, input, inferParams, null]);
    
    // Small delay then check status
    await sleep(100);
    const duringStatus = await client.testFunction('rkllm.is_running', [null]);
    console.log(`   Status during inference: ${JSON.stringify(duringStatus.response?.result || 'unknown')}`);
    
    // Wait for inference to complete
    const inferenceResult = await inferencePromise;
    
    // Check status after
    const afterStatus = await client.testFunction('rkllm.is_running', [null]);
    console.log(`   Status after inference: ${JSON.stringify(afterStatus.response?.result || 'unknown')}`);
    
    if (beforeStatus.success || duringStatus.success || afterStatus.success) {
      console.log('   ✅ Running status checks working');
      passedTests++;
      results.push({test: 'Running status', passed: true});
    } else {
      console.log('   ❌ Running status checks failed');
      results.push({test: 'Running status', passed: false, error: 'Status checks failed'});
    }
    
  } catch (error) {
    console.log(`   ❌ Running status test error: ${error.message}`);
    results.push({test: 'Running status', passed: false, error: error.message});
  }

  // Test 4: Abort Operation
  console.log('\n🧪 TEST 4: Abort Operation');
  console.log('=' .repeat(50));
  totalTests++;
  
  try {
    // Start a long inference
    const input = createRKLLMInput('Write a very detailed essay about the evolution of technology over the past century, covering at least 10 different technological advances and their impacts on society');
    const inferParams = createRKLLMInferParam();
    
    // Start inference but don't await
    const inferencePromise = client.testFunction('rkllm.run', [null, input, inferParams, null]);
    
    // Small delay then abort
    await sleep(200);
    const abortResult = await client.testFunction('rkllm.abort', [null]);
    
    console.log(`   Abort result: ${JSON.stringify(abortResult.response?.result || abortResult.error || 'no response')}`);
    
    // Wait for original inference to complete (it should be aborted)
    try {
      const finalResult = await inferencePromise;
      console.log(`   Final inference result: ${finalResult.success ? 'completed' : 'failed/aborted'}`);
    } catch (e) {
      console.log(`   Inference properly aborted: ${e.message}`);
    }
    
    if (abortResult.success || abortResult.response) {
      console.log('   ✅ Abort operation accessible');
      passedTests++;
      results.push({test: 'Abort operation', passed: true});
    } else {
      console.log('   ❌ Abort operation failed');
      results.push({test: 'Abort operation', passed: false, error: 'Abort failed'});
    }
    
  } catch (error) {
    console.log(`   ❌ Abort test error: ${error.message}`);
    results.push({test: 'Abort operation', passed: false, error: error.message});
  }

  // Test 5: Callback State Transitions
  console.log('\n🧪 TEST 5: Callback State Analysis');
  console.log('=' .repeat(50));
  totalTests++;
  
  try {
    const input = createRKLLMInput('What is machine learning?');
    const inferParams = createRKLLMInferParam();
    
    const result = await client.testFunction('rkllm.run', [null, input, inferParams, null]);
    
    if (result.success && result.response && result.response.result) {
      const res = result.response.result;
      
      console.log('\n   🔍 Analyzing callback state:');
      
      if ('_callback_state' in res) {
        const state = res._callback_state;
        console.log(`   ✅ Callback state present: ${state}`);
        
        // Interpret state based on RKLLM constants
        const stateNames = {
          0: 'RKLLM_RUN_NORMAL',
          1: 'RKLLM_RUN_WAITING', 
          2: 'RKLLM_RUN_FINISH',
          3: 'RKLLM_RUN_ERROR'
        };
        
        const stateName = stateNames[state] || 'UNKNOWN';
        console.log(`   State meaning: ${stateName} (${state})`);
        
        if (state >= 0 && state <= 3) {
          console.log('   ✅ Valid callback state range');
          passedTests++;
          results.push({test: 'Callback state', passed: true, state: stateName});
        } else {
          console.log('   ⚠️  Callback state outside expected range');
          results.push({test: 'Callback state', passed: false, state: state});
        }
      } else {
        console.log('   ⚠️  No callback state in response');
        results.push({test: 'Callback state', passed: false, error: 'No callback state'});
      }
    } else {
      console.log('   ❌ No response for state analysis');
      results.push({test: 'Callback state', passed: false, error: 'No response'});
    }
    
  } catch (error) {
    console.log(`   ❌ Callback state test error: ${error.message}`);
    results.push({test: 'Callback state', passed: false, error: error.message});
  }

  // Test 6: Performance Metrics Validation
  console.log('\n🧪 TEST 6: Performance Metrics');
  console.log('=' .repeat(50));
  totalTests++;
  
  try {
    const input = createRKLLMInput('Explain neural networks');
    const inferParams = createRKLLMInferParam();
    
    const startTime = Date.now();
    const result = await client.testFunction('rkllm.run', [null, input, inferParams, null]);
    const totalTime = Date.now() - startTime;
    
    if (result.success && result.response && result.response.result && result.response.result.perf) {
      const perf = result.response.result.perf;
      
      console.log('\n   🔍 Performance metrics analysis:');
      console.log(`   Total request time: ${totalTime}ms`);
      console.log(`   Prefill time: ${perf.prefill_time_ms}ms`);
      console.log(`   Generate time: ${perf.generate_time_ms}ms`);
      console.log(`   Prefill tokens: ${perf.prefill_tokens}`);
      console.log(`   Generated tokens: ${perf.generate_tokens}`);
      console.log(`   Memory usage: ${perf.memory_usage_mb}MB`);
      
      // Validate metrics make sense
      const metricsValid = (
        typeof perf.prefill_time_ms === 'number' &&
        typeof perf.generate_time_ms === 'number' &&
        typeof perf.memory_usage_mb === 'number' &&
        perf.prefill_time_ms >= 0 &&
        perf.generate_time_ms >= 0 &&
        perf.memory_usage_mb >= 0
      );
      
      if (metricsValid) {
        console.log('   ✅ Performance metrics valid');
        passedTests++;
        results.push({test: 'Performance metrics', passed: true, metrics: perf});
      } else {
        console.log('   ❌ Performance metrics invalid');
        results.push({test: 'Performance metrics', passed: false, error: 'Invalid metrics'});
      }
    } else {
      console.log('   ❌ No performance metrics in response');
      results.push({test: 'Performance metrics', passed: false, error: 'No metrics'});
    }
    
  } catch (error) {
    console.log(`   ❌ Performance metrics test error: ${error.message}`);
    results.push({test: 'Performance metrics', passed: false, error: error.message});
  }

  // Cleanup
  console.log('\n🧹 Cleaning up async test model...');
  try {
    await client.testFunction('rkllm.destroy', [null]);
    console.log('✅ Model destroyed');
  } catch (error) {
    console.log(`⚠️  Cleanup warning: ${error.message}`);
  }

  // Summary
  console.log('\n' + '=' .repeat(60));
  console.log('📊 ASYNC OPERATIONS TEST RESULTS');
  console.log('=' .repeat(60));
  
  const passRate = (passedTests / totalTests * 100).toFixed(1);
  console.log(`\n🎯 OVERALL: ${passedTests}/${totalTests} async tests passed (${passRate}%)`);
  
  console.log('\n📋 ASYNC TESTING SUMMARY:');
  results.forEach(result => {
    const status = result.passed ? '✅' : '❌';
    console.log(`${status} ${result.test}`);
    if (result.duration) {
      console.log(`    Duration: ${result.duration}ms`);
    }
    if (result.output) {
      console.log(`    Output: "${result.output.substring(0, 50)}..."`);
    }
    if (result.state) {
      console.log(`    State: ${result.state}`);
    }
    if (result.error) {
      console.log(`    Error: ${result.error}`);
    }
  });

  const overallSuccess = passedTests >= totalTests * 0.8; // 80% pass rate
  printTestResult('Async Operations & Streaming', overallSuccess);
  return overallSuccess;
}

module.exports = testAsyncOperations;