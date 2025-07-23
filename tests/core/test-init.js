const TestClient = require('../lib/test-client');
const { printTestSection, printTestResult, createRKLLMParam, TEST_MODELS } = require('../lib/test-helpers');

/**
 * Test rkllm.init with real model loading and streaming response validation
 * REAL BEHAVIOR: Init triggers immediate inference callback responses
 */
async function testInit(client) {
  printTestSection('TESTING: rkllm.init - Real Model Loading');

  console.log('📋 REAL SYSTEM BEHAVIOR:');
  console.log('   - Model loads and immediately starts generating text');
  console.log('   - Returns streaming callback responses (RKLLMResult)');
  console.log('   - Contains: text, token_id, perf metrics, callback_state');
  console.log(`   - Using real model: ${TEST_MODELS.NORMAL}`);

  // Test with valid model path
  const validParams = createRKLLMParam(TEST_MODELS.NORMAL);
  
  console.log('\n🔍 INPUT STRUCTURE:');
  console.log('   Params[0]: null (handle managed by server)');
  console.log('   Params[1]: RKLLMParam structure');
  console.log('   Params[2]: null (callback managed by server)');

  const result = await client.testFunction('rkllm.init', [
    null,        // LLMHandle (managed by server)
    validParams, // RKLLMParam
    null         // LLMResultCallback (managed by server)
  ]);

  if (result.success && result.response.result) {
    const initResult = result.response.result;
    
    console.log('\n🔍 ANALYZING REAL RESPONSE STRUCTURE:');
    
    // Check for RKLLMResult structure (real behavior)
    const hasText = 'text' in initResult;
    const hasTokenId = 'token_id' in initResult;
    const hasPerf = 'perf' in initResult;
    const hasCallbackState = '_callback_state' in initResult;
    
    if (hasText || hasTokenId) {
      console.log('✅ Model initialization successful - streaming response detected');
      console.log(`✅ Generated text: "${initResult.text || 'N/A'}"`);
      console.log(`✅ Token ID: ${initResult.token_id || 'N/A'}`);
      
      if (hasPerf) {
        console.log('✅ Performance metrics present');
        console.log(`   - Prefill tokens: ${initResult.perf.prefill_tokens || 0}`);
        console.log(`   - Memory usage: ${initResult.perf.memory_usage_mb || 0}MB`);
      }
      
      if (hasCallbackState !== undefined) {
        console.log(`✅ Callback state: ${initResult._callback_state}`);
      }
      
      console.log('✅ Real model inference confirmed - model is working!');
      printTestResult('rkllm.init', true);
      return true;
      
    } else {
      // Fallback check for simple success message format
      const hasSuccess = 'success' in initResult && initResult.success;
      const hasMessage = 'message' in initResult;
      
      if (hasSuccess && hasMessage) {
        console.log('✅ Model initialization successful (simple format)');
        console.log(`✅ Message: ${initResult.message}`);
        printTestResult('rkllm.init', true);
        return true;
      } else {
        console.log('❌ Unexpected response structure');
        console.log('   Expected: RKLLMResult with text/token_id OR success/message');
        console.log(`   Received: ${JSON.stringify(initResult, null, 2)}`);
        printTestResult('rkllm.init', false, 'Unexpected response format');
        return false;
      }
    }
  } else if (result.error) {
    console.log(`❌ Initialization failed: ${result.error.message || result.error}`);
    console.log('   This indicates model file issues or hardware problems');
    printTestResult('rkllm.init', false, 'Model initialization failed');
    return false;
  } else {
    console.log('❌ No response received');
    printTestResult('rkllm.init', false, 'No response received');
    return false;
  }
}

module.exports = testInit;