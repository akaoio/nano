const TestClient = require('../lib/test-client');
const { 
  printTestSection, 
  printTestResult, 
  createRKLLMParam,
  createRKLLMInput,
  createRKLLMInferParam 
} = require('../lib/test-helpers');

/**
 * Test all core RKLLM functions (without full model initialization)
 * Tests function mapping and basic parameter handling
 */
async function testAllCore(client) {
  printTestSection('TESTING: All Core Functions');

  console.log('📋 EXPECTED:');
  console.log('   - All functions should be properly mapped');
  console.log('   - Should accept correct parameter structures');
  console.log('   - Should return appropriate responses');

  const tests = [
    {
      name: 'rkllm.run',
      params: [null, createRKLLMInput(), createRKLLMInferParam(), null],
      description: 'Synchronous inference'
    },
    {
      name: 'rkllm.run_async',
      params: [null, createRKLLMInput(), createRKLLMInferParam(), null],
      description: 'Asynchronous inference'
    },
    {
      name: 'rkllm.is_running',
      params: [null],
      description: 'Check running status'
    },
    {
      name: 'rkllm.abort',
      params: [null],
      description: 'Abort inference'
    }
  ];

  const results = [];

  for (const test of tests) {
    console.log(`\\n🧪 Testing ${test.name} (${test.description})`);
    
    const result = await client.testFunction(test.name, test.params);
    
    // Check if function is properly mapped
    const isMapped = !(result.response?.error?.code === -32601); // Not "Method not found"
    
    if (isMapped) {
      console.log(`✅ ${test.name} is properly mapped`);
      
      // For functions that require model initialization
      if (result.response?.error?.message === 'Internal server error') {
        console.log('✅ Returns expected error when model not initialized');
      }
    } else {
      console.log(`❌ ${test.name} not mapped - Method not found`);
    }

    results.push({
      name: test.name,
      mapped: isMapped
    });
  }

  // Summary
  console.log('\\n📊 CORE FUNCTIONS MAPPING SUMMARY:');
  const mappedCount = results.filter(r => r.mapped).length;
  
  results.forEach(result => {
    const status = result.mapped ? '✅' : '❌';
    console.log(`${status} ${result.name}`);
  });

  const allMapped = mappedCount === results.length;
  console.log(`\\n🎯 TOTAL: ${mappedCount}/${results.length} core functions mapped`);
  
  printTestResult('All Core Functions', allMapped);
  return allMapped;
}

module.exports = testAllCore;