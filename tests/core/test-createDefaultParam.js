const TestClient = require('../lib/test-client');
const { printTestSection, printTestResult } = require('../lib/test-helpers');

/**
 * Test rkllm.createDefaultParam
 * DESIGN.md line 155: rkllm.createDefaultParam → rkllm_createDefaultParam()
 */
async function testCreateDefaultParam(client) {
  printTestSection('TESTING: rkllm.createDefaultParam');

  console.log('📋 EXPECTED:');
  console.log('   - Method should return default RKLLM parameters');
  console.log('   - Response should contain all required fields per DESIGN.md lines 173-204');
  console.log('   - Should work without model initialization');

  const result = await client.testFunction('rkllm.createDefaultParam', []);

  if (result.success && result.response.result) {
    const defaultParams = result.response.result;
    
    // Verify required fields exist
    const requiredFields = [
      'model_path', 'max_context_len', 'max_new_tokens', 'top_k', 'n_keep',
      'top_p', 'temperature', 'repeat_penalty', 'frequency_penalty', 
      'presence_penalty', 'mirostat', 'mirostat_tau', 'mirostat_eta',
      'skip_special_token', 'is_async', 'extend_param'
    ];

    let allFieldsPresent = true;
    for (const field of requiredFields) {
      if (!(field in defaultParams)) {
        console.log(`❌ Missing required field: ${field}`);
        allFieldsPresent = false;
      }
    }

    if (allFieldsPresent) {
      console.log('✅ All required fields present in default parameters');
      console.log('✅ Matches DESIGN.md RKLLMParam structure');
    }

    printTestResult('rkllm.createDefaultParam', allFieldsPresent);
    return allFieldsPresent;
  } else {
    printTestResult('rkllm.createDefaultParam', false, result.error || 'No result returned');
    return false;
  }
}

module.exports = testCreateDefaultParam;