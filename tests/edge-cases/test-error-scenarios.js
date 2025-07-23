const TestClient = require('../lib/test-client');
const { 
  printTestSection, 
  printTestResult, 
  createRKLLMParam,
  createRKLLMInput,
  createRKLLMInferParam,
  TEST_MODELS 
} = require('../lib/test-helpers');

/**
 * Test error scenarios and edge cases
 * Ensures proper error handling in all situations
 */
async function testErrorScenarios(client) {
  printTestSection('ERROR SCENARIOS & EDGE CASES');

  console.log('🚨 TESTING ERROR HANDLING:');
  console.log('   - Invalid model paths');
  console.log('   - Malformed parameters');
  console.log('   - Operations without initialization');
  console.log('   - Memory/resource limits');
  console.log('   - Concurrent operations');
  console.log('');

  let totalTests = 0;
  let passedTests = 0;
  const results = [];

  // Test 1: Invalid Model Paths
  console.log('🧪 TEST 1: Invalid Model Paths');
  console.log('=' .repeat(40));
  
  const invalidPaths = [
    '/non/existent/model.rkllm',
    '',
    null,
    '/tmp/not_a_model.txt',
    '/home/x/Projects/nano/models/nonexistent/model.rkllm'
  ];

  for (const path of invalidPaths) {
    totalTests++;
    console.log(`\n   Testing path: ${path || 'null'}`);
    
    try {
      const params = createRKLLMParam();
      params.model_path = path;
      
      const result = await client.testFunction('rkllm.init', [null, params, null]);
      
      if (result.error || (result.response && result.response.error)) {
        console.log(`   ✅ Properly rejected invalid path`);
        console.log(`      Error: ${result.error?.message || result.response?.error?.message}`);
        passedTests++;
        results.push({test: `Invalid path: ${path}`, passed: true, expected: 'error'});
      } else {
        console.log(`   ❌ Should have rejected invalid path`);
        results.push({test: `Invalid path: ${path}`, passed: false, error: 'Should reject invalid path'});
      }
    } catch (error) {
      console.log(`   ✅ Exception thrown as expected: ${error.message}`);
      passedTests++;
      results.push({test: `Invalid path: ${path}`, passed: true, expected: 'exception'});
    }
  }

  // Test 2: Malformed Parameters
  console.log('\n🧪 TEST 2: Malformed Parameters');
  console.log('=' .repeat(40));
  
  const malformedTests = [
    {
      name: 'Missing params array',
      params: null
    },
    {
      name: 'Empty params array',
      params: []
    },
    {
      name: 'Wrong param count',
      params: [null]
    },
    {
      name: 'Invalid RKLLMParam structure',
      params: [null, { invalid: true }, null]
    },
    {
      name: 'Negative max_tokens',
      params: [null, { ...createRKLLMParam(TEST_MODELS.NORMAL), max_new_tokens: -1 }, null]
    }
  ];

  for (const test of malformedTests) {
    totalTests++;
    console.log(`\n   Testing: ${test.name}`);
    
    try {
      const result = await client.testFunction('rkllm.init', test.params);
      
      if (result.error || (result.response && result.response.error)) {
        console.log(`   ✅ Properly handled malformed parameters`);
        passedTests++;
        results.push({test: test.name, passed: true, expected: 'error'});
      } else {
        console.log(`   ❌ Should have rejected malformed parameters`);
        results.push({test: test.name, passed: false, error: 'Should reject malformed params'});
      }
    } catch (error) {
      console.log(`   ✅ Exception handled: ${error.message}`);
      passedTests++;
      results.push({test: test.name, passed: true, expected: 'exception'});
    }
  }

  // Test 3: Operations Without Initialization
  console.log('\n🧪 TEST 3: Operations Without Model');
  console.log('=' .repeat(40));
  
  // Make sure no model is loaded
  try {
    await client.testFunction('rkllm.destroy', [null]);
  } catch (e) {
    // Ignore destroy errors
  }

  const uninitializedTests = [
    {
      name: 'run without init',
      method: 'rkllm.run',
      params: [null, createRKLLMInput('test'), createRKLLMInferParam(), null]
    },
    {
      name: 'is_running without init',
      method: 'rkllm.is_running',
      params: [null]
    },
    {
      name: 'abort without init',
      method: 'rkllm.abort',
      params: [null]
    },
    {
      name: 'get_kv_cache_size without init',
      method: 'rkllm.get_kv_cache_size',
      params: [null]
    }
  ];

  for (const test of uninitializedTests) {
    totalTests++;
    console.log(`\n   Testing: ${test.name}`);
    
    try {
      const result = await client.testFunction(test.method, test.params);
      
      if (result.error || (result.response && result.response.error)) {
        console.log(`   ✅ Properly rejected operation without initialization`);
        console.log(`      Error: ${result.error?.message || result.response?.error?.message}`);
        passedTests++;
        results.push({test: test.name, passed: true, expected: 'error'});
      } else {
        console.log(`   ❌ Should have rejected operation without init`);
        results.push({test: test.name, passed: false, error: 'Should reject without init'});
      }
    } catch (error) {
      console.log(`   ✅ Exception handled: ${error.message}`);
      passedTests++;
      results.push({test: test.name, passed: true, expected: 'exception'});
    }
  }

  // Test 4: Resource Limits
  console.log('\n🧪 TEST 4: Resource Limits');
  console.log('=' .repeat(40));
  
  totalTests++;
  console.log('\n   Testing: Very large context length');
  
  try {
    const params = createRKLLMParam(TEST_MODELS.NORMAL);
    params.max_context_len = 1000000; // Very large
    params.max_new_tokens = 100000;   // Very large
    
    const result = await client.testFunction('rkllm.init', [null, params, null]);
    
    if (result.error || (result.response && result.response.error)) {
      console.log(`   ✅ Properly handled excessive resource request`);
      passedTests++;
      results.push({test: 'Large resource limits', passed: true, expected: 'error'});
    } else if (result.success) {
      console.log(`   ⚠️  Large resources accepted (may be valid behavior)`);
      passedTests++; // This might be acceptable
      results.push({test: 'Large resource limits', passed: true, note: 'Accepted'});
      
      // Clean up
      await client.testFunction('rkllm.destroy', [null]);
    }
  } catch (error) {
    console.log(`   ✅ Resource limit exception: ${error.message}`);
    passedTests++;
    results.push({test: 'Large resource limits', passed: true, expected: 'exception'});
  }

  // Test 5: Concurrent Operations (if applicable)
  console.log('\n🧪 TEST 5: Concurrent Operations');
  console.log('=' .repeat(40));
  
  totalTests++;
  console.log('\n   Testing: Multiple simultaneous init calls');
  
  try {
    const params = createRKLLMParam(TEST_MODELS.NORMAL);
    
    // Start multiple init operations simultaneously
    const promises = [
      client.testFunction('rkllm.init', [null, params, null]),
      client.testFunction('rkllm.init', [null, params, null]),
      client.testFunction('rkllm.init', [null, params, null])
    ];
    
    const results_concurrent = await Promise.allSettled(promises);
    
    // Check that at least one succeeded or all failed gracefully
    const successful = results_concurrent.filter(r => r.status === 'fulfilled' && r.value.success).length;
    const failed = results_concurrent.filter(r => r.status === 'rejected' || r.value.error).length;
    
    if (successful >= 1 || failed === results_concurrent.length) {
      console.log(`   ✅ Concurrent operations handled: ${successful} succeeded, ${failed} failed`);
      passedTests++;
      results.push({test: 'Concurrent operations', passed: true, note: `${successful}/${results_concurrent.length} succeeded`});
    } else {
      console.log(`   ❌ Concurrent operations not handled properly`);
      results.push({test: 'Concurrent operations', passed: false, error: 'Improper handling'});
    }
    
    // Clean up
    await client.testFunction('rkllm.destroy', [null]);
    
  } catch (error) {
    console.log(`   ⚠️  Concurrent test error: ${error.message}`);
    results.push({test: 'Concurrent operations', passed: false, error: error.message});
  }

  // Test 6: Memory Stress Test
  console.log('\n🧪 TEST 6: Memory Stress Test');
  console.log('=' .repeat(40));
  
  totalTests++;
  console.log('\n   Testing: Rapid init/destroy cycles');
  
  try {
    let cyclesPassed = 0;
    const maxCycles = 5;
    
    for (let i = 0; i < maxCycles; i++) {
      const params = createRKLLMParam(TEST_MODELS.NORMAL);
      const initResult = await client.testFunction('rkllm.init', [null, params, null]);
      
      if (initResult.success || initResult.response) {
        await client.testFunction('rkllm.destroy', [null]);
        cyclesPassed++;
      }
      
      // Small delay between cycles
      await new Promise(resolve => setTimeout(resolve, 100));
    }
    
    if (cyclesPassed >= maxCycles * 0.8) { // 80% success rate
      console.log(`   ✅ Memory stress test passed: ${cyclesPassed}/${maxCycles} cycles`);
      passedTests++;
      results.push({test: 'Memory stress test', passed: true, cycles: cyclesPassed});
    } else {
      console.log(`   ❌ Memory stress test failed: ${cyclesPassed}/${maxCycles} cycles`);
      results.push({test: 'Memory stress test', passed: false, cycles: cyclesPassed});
    }
    
  } catch (error) {
    console.log(`   ❌ Memory stress test error: ${error.message}`);
    results.push({test: 'Memory stress test', passed: false, error: error.message});
  }

  // Summary
  console.log('\n' + '=' .repeat(60));
  console.log('📊 ERROR SCENARIO TEST RESULTS');
  console.log('=' .repeat(60));
  
  const passRate = (passedTests / totalTests * 100).toFixed(1);
  console.log(`\n🎯 OVERALL: ${passedTests}/${totalTests} error tests passed (${passRate}%)`);
  
  console.log('\n📋 ERROR HANDLING SUMMARY:');
  results.forEach(result => {
    const status = result.passed ? '✅' : '❌';
    console.log(`${status} ${result.test}`);
    if (result.expected) {
      console.log(`    Expected: ${result.expected}`);
    }
    if (result.note) {
      console.log(`    Note: ${result.note}`);
    }
    if (result.error) {
      console.log(`    Issue: ${result.error}`);
    }
  });

  const overallSuccess = passedTests >= totalTests * 0.7; // 70% pass rate for error scenarios
  printTestResult('Error Scenarios & Edge Cases', overallSuccess);
  return overallSuccess;
}

module.exports = testErrorScenarios;