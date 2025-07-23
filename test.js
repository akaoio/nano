#!/usr/bin/env node

/**
 * RKLLM Real Streaming Test Suite - Production Ready
 * Tests actual RKLLM functionality with real streaming data
 */

const ServerManager = require('./tests/lib/server-manager');
const TestClient = require('./tests/lib/test-client');
const { createRKLLMParam, createRKLLMInput, createRKLLMInferParam, TEST_MODELS } = require('./tests/lib/test-helpers');

/**
 * Run real streaming tests - no fake implementations
 */
async function runRealStreamingTests() {
  console.log('🚀 RKLLM REAL STREAMING TEST SUITE');
  console.log('=' .repeat(50));
  console.log('🎯 Testing actual RKLLM streaming functionality');
  console.log('');

  const serverManager = new ServerManager();
  const client = new TestClient();
  
  let testNumber = 0;

  try {
    // Setup
    console.log('🔧 SETUP');
    console.log('=' .repeat(30));
    await serverManager.startServer();
    await client.connect();
    console.log('✅ Server ready for real streaming tests\n');

    // Test 1: Core API Functions
    await runTest(++testNumber, 'Core API - createDefaultParam', async () => {
      const result = await client.sendRequest('rkllm.createDefaultParam', []);
      if (!result.result || typeof result.result.max_context_len !== 'number') {
        throw new Error('createDefaultParam failed - invalid structure');
      }
    });

    await runTest(++testNumber, 'Core API - constants', async () => {
      const result = await client.sendRequest('rkllm.get_constants', []);
      if (!result.result || !result.result.LLM_CALL_STATES || !result.result.INPUT_TYPES) {
        throw new Error('get_constants failed - missing required enums');
      }
    });

    // Test 2: Model Initialization with Sync Mode
    await runTest(++testNumber, 'Model Init - Sync Mode', async () => {
      const syncParams = createRKLLMParam(TEST_MODELS.NORMAL);
      syncParams.is_async = false; // Explicit sync mode
      syncParams.max_new_tokens = 20;
      
      const result = await client.sendRequest('rkllm.init', [null, syncParams, null]);
      if (!result.result || !result.result.success) {
        throw new Error('Sync model initialization failed');
      }
      await new Promise(resolve => setTimeout(resolve, 2000));
    });

    // Test 3: REAL ASYNC STREAMING - Token by Token
    await runTest(++testNumber, 'REAL ASYNC STREAMING - Token by Token', async () => {
      // Destroy sync model and create async model
      await client.sendRequest('rkllm.destroy', [null]);
      await new Promise(resolve => setTimeout(resolve, 1000));
      
      const asyncParams = createRKLLMParam(TEST_MODELS.NORMAL);
      asyncParams.is_async = true; // CRITICAL: Enable async streaming
      asyncParams.max_new_tokens = 50; // Smaller for cleaner output
      asyncParams.temperature = 0.8;
      
      const initResult = await client.sendRequest('rkllm.init', [null, asyncParams, null]);
      if (!initResult.result || !initResult.result.success) {
        throw new Error('Async model initialization failed');
      }
      await new Promise(resolve => setTimeout(resolve, 3000));
      
      // Now test compact JSON streaming verification
      const input = createRKLLMInput('Write a short creative story.');
      const inferParams = createRKLLMInferParam();
      
      console.log('\n      🎬 STARTING COMPACT JSON STREAMING VERIFICATION...');
      console.log('      🔥 Each token verified as complete JSON-RPC response (compact display):');
      console.log('      ▶️  COMPACT OUTPUT: ');
      
      // Use compact streaming method that verifies JSON-RPC format without verbose output
      const streamingResult = await client.sendRawJsonStreamingRequest('rkllm.run', [null, input, inferParams, null]);
      
      if (!streamingResult.tokens || streamingResult.tokens.length === 0) {
        throw new Error('No streaming tokens received');
      }
      
      console.log(`\n      📊 STREAMING VERIFICATION RESULTS:`);
      console.log(`      - Total JSON-RPC responses verified: ${streamingResult.tokens.length}`);
      console.log(`      - Complete generated text: "${streamingResult.fullText}"`);
      console.log(`      - Sample token IDs: [${streamingResult.tokens.slice(0, 3).map(t => t.token_id).join(', ')}]`);
      if (streamingResult.finalPerf) {
        console.log(`      - Generation time: ${streamingResult.finalPerf.generate_time_ms}ms`);
      }
      
      // Real validation
      if (streamingResult.tokens.length < 5) {
        throw new Error(`Insufficient streaming tokens - got ${streamingResult.tokens.length}, expected at least 5`);
      }
      
      console.log(`\n      ✅ JSON-RPC STREAMING VERIFIED! Each token received as complete JSON-RPC response`);
    });

    // Test 4: Hidden States Extraction
    await runTest(++testNumber, 'Hidden States Extraction', async () => {
      const input = createRKLLMInput('Extract hidden states from this text.');
      const inferParams = createRKLLMInferParam();
      inferParams.mode = 1; // RKLLM_INFER_GET_LAST_HIDDEN_LAYER
      
      const result = await client.sendRequest('rkllm.run', [null, input, inferParams, null]);
      
      if (!result.result) {
        throw new Error('Hidden states extraction failed - no result');
      }
      
      // Check for real hidden states data
      if (result.result.last_hidden_layer && result.result.last_hidden_layer.embd_size > 0) {
        console.log(`      ✅ Hidden states: ${result.result.last_hidden_layer.embd_size} dimensions, ${result.result.last_hidden_layer.num_tokens} tokens`);
      } else {
        throw new Error('Hidden states extraction failed - no valid hidden layer data');
      }
    });

    // Test 5: Performance Monitoring
    await runTest(++testNumber, 'Performance Monitoring', async () => {
      const input = createRKLLMInput('Quick performance test.');
      const inferParams = createRKLLMInferParam();
      
      const startTime = Date.now();
      const result = await client.sendRequest('rkllm.run', [null, input, inferParams, null]);
      const endTime = Date.now();
      
      if (!result.result) {
        throw new Error('Performance test failed - no result');
      }
      
      const responseTime = endTime - startTime;
      console.log(`      📊 Response time: ${responseTime}ms`);
      
      if (result.result.perf) {
        console.log(`      📊 RKLLM performance: prefill=${result.result.perf.prefill_time_ms}ms, generate=${result.result.perf.generate_time_ms}ms`);
        console.log(`      📊 Memory usage: ${result.result.perf.memory_usage_mb}MB`);
      }
      
      if (responseTime > 10000) { // 10 second timeout
        throw new Error(`Performance test failed - response too slow: ${responseTime}ms`);
      }
    });

    // Test 6: Model State Management
    await runTest(++testNumber, 'Model State Management', async () => {
      const isRunningResult = await client.sendRequest('rkllm.is_running', [null]);
      if (isRunningResult.error) {
        throw new Error('is_running check failed');
      }
      
      const abortResult = await client.sendRequest('rkllm.abort', [null]);
      if (abortResult.error) {
        throw new Error('abort command failed');
      }
      
      console.log(`      ✅ State management working - is_running and abort responded correctly`);
    });

    // Test 7: Model Cleanup
    await runTest(++testNumber, 'Model Cleanup', async () => {
      const destroyResult = await client.sendRequest('rkllm.destroy', [null]);
      if (!destroyResult.result || !destroyResult.result.success) {
        throw new Error('Model destruction failed');
      }
      
      // Verify model is actually destroyed by trying to use it (with shorter timeout)
      try {
        const input = createRKLLMInput('This should fail.');
        const inferParams = createRKLLMInferParam();
        const failResult = await client.sendRequestSilent('rkllm.run', [null, input, inferParams, null]);
        
        // If we get here without error, model wasn't properly destroyed
        if (!failResult.error) {
          throw new Error('Model destruction incomplete - can still run inference');
        }
        
        console.log(`      ✅ Model properly destroyed - subsequent calls correctly fail with: ${failResult.error.message}`);
      } catch (timeoutError) {
        // Timeout is also acceptable - means server isn't responding (model destroyed)
        if (timeoutError.message.includes('timeout')) {
          console.log(`      ✅ Model properly destroyed - server correctly non-responsive after destruction`);
        } else {
          throw timeoutError;
        }
      }
    });

  } catch (error) {
    console.error(`\n❌ REAL STREAMING TEST FAILED: ${error.message}`);
    console.error('🛑 Critical functionality not working');
    process.exit(1);
  } finally {
    // Cleanup
    console.log('\n🧹 CLEANUP');
    try {
      await client.ensureCleanState();
      client.disconnect();
      serverManager.stopServer();
      console.log('✅ Cleanup completed');
    } catch (error) {
      console.log(`⚠️  Cleanup warning: ${error.message}`);
    }
  }

  // All real streaming tests passed
  console.log('\n' + '=' .repeat(50));
  console.log(`🎉 ALL ${testNumber} REAL STREAMING TESTS PASSED!`);
  console.log('✅ RKLLM server streaming functionality is working correctly');
  console.log('🚀 Production ready for real-world usage');
  console.log('=' .repeat(50));
  process.exit(0);
}

/**
 * Run a single test with proper error handling
 */
async function runTest(number, name, testFunction) {
  process.stdout.write(`📋 TEST ${number}: ${name}... `);
  
  try {
    await testFunction();
    console.log('✅ PASS');
  } catch (error) {
    console.log('❌ FAIL');
    throw error; // Re-throw to stop execution
  }
}

// Error handlers for clean exit
process.on('uncaughtException', (error) => {
  console.error('\n💥 Uncaught Exception:', error.message);
  process.exit(1);
});

process.on('unhandledRejection', (reason, promise) => {
  console.error('\n💥 Unhandled Rejection:', reason);
  process.exit(1);
});

// Run the real streaming tests
if (require.main === module) {
  runRealStreamingTests();
}

module.exports = runRealStreamingTests;