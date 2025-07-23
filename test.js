#!/usr/bin/env node

/**
 * Sequential Test Runner - Exit on First Failure
 * Shows each test completion and exits immediately if any test fails
 */

const ServerManager = require('./tests/lib/server-manager');
const TestClient = require('./tests/lib/test-client');
const { createRKLLMParam, createRKLLMInput, createRKLLMInferParam, TEST_MODELS } = require('./tests/lib/test-helpers');

/**
 * Run all tests sequentially - exit immediately on failure
 */
async function runAllTests() {
  console.log('🚀 RKLLM TEST SUITE - SEQUENTIAL EXECUTION');
  console.log('=' .repeat(60));
  console.log('🎯 Exit on first failure, show each test completion');
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
    console.log('✅ Server ready for testing\n');

    // Test 1: Basic Functions
    await runTest(++testNumber, 'Basic Functions - createDefaultParam', async () => {
      const result = await client.testFunction('rkllm.createDefaultParam', []);
      if (!result.success || !result.result || result.result.model_path === undefined) {
        throw new Error('createDefaultParam failed');
      }
    });

    await runTest(++testNumber, 'Basic Functions - get_constants', async () => {
      const result = await client.testFunction('rkllm.get_constants', []);
      if (!result.success || !result.result || !result.result.CPU_MASKS) {
        throw new Error('get_constants failed');
      }
    });

    // Test 2: Model Initialization
    await runTest(++testNumber, 'Model Initialization', async () => {
      const initParams = createRKLLMParam(TEST_MODELS.NORMAL);
      const result = await client.testFunction('rkllm.init', [null, initParams, null]);
      if (!result.success || !result.result) {
        throw new Error('Model initialization failed');
      }
      // Wait for model to be ready
      await new Promise(resolve => setTimeout(resolve, 2000));
    });

    // Test 3: Text Generation
    await runTest(++testNumber, 'Text Generation', async () => {
      const input = createRKLLMInput('Hello, what is your name?');
      const inferParams = createRKLLMInferParam();
      const result = await client.testFunction('rkllm.run', [null, input, inferParams, null]);
      if (!result.success || !result.result || result.result.text === undefined) {
        throw new Error('Text generation failed');
      }
      console.log(`      Generated: "${result.result.text}"`);
    });

    // Test 4: Hidden States
    await runTest(++testNumber, 'Hidden States Mode', async () => {
      const input = createRKLLMInput('Test hidden states');
      const inferParams = createRKLLMInferParam();
      inferParams.mode = 1; // RKLLM_INFER_GET_LAST_HIDDEN_LAYER
      const result = await client.testFunction('rkllm.run', [null, input, inferParams, null]);
      if (!result.success || !result.result) {
        throw new Error('Hidden states mode failed');
      }
      if (result.result.last_hidden_layer && result.result.last_hidden_layer.hidden_states) {
        console.log(`      Hidden states: ${result.result.last_hidden_layer.hidden_states.length} elements`);
      }
    });

    // Test 5: Core Function Mapping
    const coreFunctions = ['is_running', 'abort', 'destroy', 'get_kv_cache_size', 'clear_kv_cache'];
    for (const func of coreFunctions) {
      await runTest(++testNumber, `Function Mapping - ${func}`, async () => {
        const result = await client.testFunction(`rkllm.${func}`, [null]);
        // For mapping tests, we just check if the function is recognized
        if (result.error && result.error.message === 'Method not found') {
          throw new Error(`Function ${func} not mapped`);
        }
        // Success or any other error means the function is mapped
      });
    }

  } catch (error) {
    console.error(`\n❌ TEST FAILED: ${error.message}`);
    console.error('🛑 Stopping test execution immediately');
    process.exit(1);
  } finally {
    // Always cleanup
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

  // All tests passed
  console.log('\n' + '=' .repeat(60));
  console.log(`🎉 ALL ${testNumber} TESTS PASSED!`);
  console.log('✅ RKLLM server is working correctly');
  console.log('=' .repeat(60));
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

// Error handlers
process.on('uncaughtException', (error) => {
  console.error('\n💥 Uncaught Exception:', error.message);
  process.exit(1);
});

process.on('unhandledRejection', (reason, promise) => {
  console.error('\n💥 Unhandled Rejection:', reason);
  process.exit(1);
});

// Run tests
if (require.main === module) {
  runAllTests();
}

module.exports = runAllTests;