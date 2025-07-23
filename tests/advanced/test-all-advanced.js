const TestClient = require('../lib/test-client');
const { 
  printTestSection, 
  printTestResult, 
  createLoRAAdapter, 
  createCrossAttnParams 
} = require('../lib/test-helpers');

/**
 * Test all advanced RKLLM functions (without model initialization)
 * Tests function mapping and parameter handling
 */
async function testAllAdvanced(client) {
  printTestSection('TESTING: All Advanced Functions');

  console.log('📋 EXPECTED:');
  console.log('   - All functions should be properly mapped (not "Method not found")');
  console.log('   - Should return "Internal server error" when model not initialized');
  console.log('   - Parameter structures should be accepted');

  const tests = [
    {
      name: 'rkllm.destroy',
      params: [null],
      description: 'Model cleanup'
    },
    {
      name: 'rkllm.load_lora', 
      params: [null, createLoRAAdapter()],
      description: 'LoRA adapter loading'
    },
    {
      name: 'rkllm.load_prompt_cache',
      params: [null, '/tmp/test_cache.bin'],
      description: 'Prompt cache loading'
    },
    {
      name: 'rkllm.release_prompt_cache',
      params: [null],
      description: 'Prompt cache cleanup'
    },
    {
      name: 'rkllm.clear_kv_cache',
      params: [null, 0, null, null],
      description: 'KV cache management'
    },
    {
      name: 'rkllm.get_kv_cache_size',
      params: [null],
      description: 'Cache size query'
    },
    {
      name: 'rkllm.set_chat_template',
      params: [null, 'You are helpful', '<user>', '</user>'],
      description: 'Chat template configuration'
    },
    {
      name: 'rkllm.set_function_tools',
      params: [null, 'System prompt', '[]', '<tool>'],
      description: 'Function calling setup'
    },
    {
      name: 'rkllm.set_cross_attn_params',
      params: [null, createCrossAttnParams()],
      description: 'Cross-attention parameters'
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
  console.log('\\n📊 ADVANCED FUNCTIONS MAPPING SUMMARY:');
  const mappedCount = results.filter(r => r.mapped).length;
  
  results.forEach(result => {
    const status = result.mapped ? '✅' : '❌';
    console.log(`${status} ${result.name}`);
  });

  const allMapped = mappedCount === results.length;
  console.log(`\\n🎯 TOTAL: ${mappedCount}/${results.length} advanced functions mapped`);
  
  printTestResult('All Advanced Functions', allMapped);
  return allMapped;
}

module.exports = testAllAdvanced;