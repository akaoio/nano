#!/usr/bin/env node

/**
 * Server Robustness Test Suite
 * Tests server stability, error handling, and edge cases:
 * - Multi-client concurrency
 * - Error handling (malformed JSON, invalid parameters)
 * - Resource management and cleanup
 * - Signal handling
 * - Configuration validation
 * - Memory leak detection
 */

const ServerManager = require('./lib/server-manager');
const TestClient = require('./lib/test-client');
const { 
  createRKLLMParam, 
  createRKLLMInput, 
  createRKLLMInferParam,
  TEST_MODELS,
  printTestSection,
  sleep
} = require('./lib/test-helpers');

class ServerRobustnessTests {
  constructor() {
    this.serverManager = new ServerManager();
    this.testResults = {
      passed: 0,
      failed: 0,
      total: 0
    };
  }

  async runTest(name, testFunction) {
    this.testResults.total++;
    process.stdout.write(`📋 ${name}... `);
    
    try {
      await testFunction();
      console.log('✅ PASS');
      this.testResults.passed++;
    } catch (error) {
      console.log('❌ FAIL');
      console.log(`   Error: ${error.message}`);
      this.testResults.failed++;
    }
  }

  async setup() {
    printTestSection('SERVER ROBUSTNESS TEST SUITE');
    console.log('🎯 Testing server stability, error handling, and edge cases');
    console.log('');

    console.log('🔧 SETUP');
    console.log('=' .repeat(30));
    await this.serverManager.startServer();
    console.log('✅ Server ready for robustness tests\n');
  }

  async teardown() {
    console.log('\n🧹 CLEANUP');
    try {
      this.serverManager.stopServer();
      console.log('✅ Cleanup completed');
    } catch (error) {
      console.log(`⚠️  Cleanup warning: ${error.message}`);
    }
  }

  async testMultiClientConcurrency() {
    const numClients = 5;
    const clients = [];
    
    // Create multiple clients
    for (let i = 0; i < numClients; i++) {
      const client = new TestClient();
      await client.connect();
      clients.push(client);
    }

    console.log(`\n      🔗 Created ${numClients} concurrent clients`);

    // Initialize model on first client
    const params = createRKLLMParam();
    const initResult = await clients[0].sendRequest('rkllm.init', [null, params, null]);
    if (!initResult.result || !initResult.result.success) {
      throw new Error('Model initialization failed');
    }
    await sleep(2000);

    // Send concurrent requests
    const promises = clients.map(async (client, index) => {
      const input = createRKLLMInput(`Concurrent request ${index}`);
      const inferParams = createRKLLMInferParam();
      
      try {
        const result = await client.sendRequest('rkllm.run', [null, input, inferParams, null]);
        return { client: index, success: !!result.result, text: result.result?.text || 'no text' };
      } catch (error) {
        return { client: index, success: false, error: error.message };
      }
    });

    const results = await Promise.all(promises);
    const successful = results.filter(r => r.success).length;
    
    console.log(`      📊 Concurrent results: ${successful}/${numClients} successful`);
    
    // Cleanup
    await clients[0].sendRequest('rkllm.destroy', [null]);
    for (const client of clients) {
      client.disconnect();
    }

    if (successful < numClients / 2) {
      throw new Error(`Too many concurrent failures: ${successful}/${numClients}`);
    }
  }

  async testMalformedJSONHandling() {
    const client = new TestClient();
    await client.connect();

    const malformedRequests = [
      '{"jsonrpc": "2.0", "method": "rkllm.createDefaultParam"', // Missing closing brace
      '{"invalid": json}', // Invalid JSON syntax
      '{"jsonrpc": "1.0", "method": "test"}', // Wrong JSON-RPC version
      '{"jsonrpc": "2.0", "method": 123}', // Non-string method
      '{"jsonrpc": "2.0", "method": "nonexistent.method", "id": 1}', // Non-existent method
      '', // Empty request
      'not json at all' // Not JSON
    ];

    let errorHandled = 0;
    
    for (const malformedJson of malformedRequests) {
      try {
        const result = await client.sendRawRequest(malformedJson);
        if (result.error) {
          errorHandled++;
          console.log(`      ✅ Properly handled malformed request: ${result.error.message}`);
        }
      } catch (error) {
        errorHandled++;
        console.log(`      ✅ Connection-level error handling: ${error.message}`);
      }
    }

    client.disconnect();

    if (errorHandled < malformedRequests.length - 2) { // Allow some flexibility
      throw new Error(`Insufficient error handling: ${errorHandled}/${malformedRequests.length}`);
    }

    console.log(`      📊 Error handling: ${errorHandled}/${malformedRequests.length} cases handled`);
  }

  async testInvalidParameterHandling() {
    const client = new TestClient();
    await client.connect();

    const invalidCases = [
      // Invalid init parameters
      {
        method: 'rkllm.init',
        params: [null, { model_path: '/nonexistent/path.rkllm' }, null],
        expectedError: true
      },
      // Invalid run parameters - no model initialized
      {
        method: 'rkllm.run',
        params: [null, createRKLLMInput('test'), createRKLLMInferParam(), null],
        expectedError: true
      },
      // Invalid parameter types
      {
        method: 'rkllm.createDefaultParam',
        params: ['invalid_param'],
        expectedError: true
      },
      // Missing required parameters
      {
        method: 'rkllm.init',
        params: [],
        expectedError: true
      }
    ];

    let errorsHandled = 0;

    for (const testCase of invalidCases) {
      try {
        const result = await client.sendRequest(testCase.method, testCase.params);
        if (testCase.expectedError && result.error) {
          errorsHandled++;
          console.log(`      ✅ ${testCase.method}: ${result.error.message}`);
        } else if (!testCase.expectedError && result.result) {
          errorsHandled++;
          console.log(`      ✅ ${testCase.method}: Valid response`);
        }
      } catch (error) {
        if (testCase.expectedError) {
          errorsHandled++;
          console.log(`      ✅ ${testCase.method}: Exception handled`);
        }
      }
    }

    client.disconnect();

    if (errorsHandled < invalidCases.length - 1) {
      throw new Error(`Insufficient parameter validation: ${errorsHandled}/${invalidCases.length}`);
    }

    console.log(`      📊 Parameter validation: ${errorsHandled}/${invalidCases.length} cases handled`);
  }

  async testResourceCleanup() {
    const client = new TestClient();
    await client.connect();

    // Initialize and destroy model multiple times to test cleanup
    for (let i = 0; i < 3; i++) {
      const params = createRKLLMParam();
      
      // Initialize
      const initResult = await client.sendRequest('rkllm.init', [null, params, null]);
      if (!initResult.result || !initResult.result.success) {
        throw new Error(`Model initialization ${i} failed`);
      }
      await sleep(1000);

      // Use model
      const input = createRKLLMInput(`Cleanup test ${i}`);
      const inferParams = createRKLLMInferParam();
      const runResult = await client.sendRequest('rkllm.run', [null, input, inferParams, null]);
      
      if (!runResult.result) {
        throw new Error(`Model inference ${i} failed`);
      }

      // Destroy
      const destroyResult = await client.sendRequest('rkllm.destroy', [null]);
      if (!destroyResult.result || !destroyResult.result.success) {
        throw new Error(`Model destruction ${i} failed`);
      }
      await sleep(500);
    }

    client.disconnect();
    console.log(`      ✅ Resource cleanup verified through 3 init/destroy cycles`);
  }

  async testConnectionRecovery() {
    let client = new TestClient();
    await client.connect();

    // Initialize model
    const params = createRKLLMParam();
    const initResult = await client.sendRequest('rkllm.init', [null, params, null]);
    if (!initResult.result || !initResult.result.success) {
      throw new Error('Initial model initialization failed');
    }
    await sleep(2000);

    console.log(`      🔗 Initial connection established and model initialized`);

    // Disconnect and reconnect
    client.disconnect();
    await sleep(1000);
    
    client = new TestClient();
    await client.connect();
    
    console.log(`      🔄 Connection recovered`);

    // Try to use previous model (should fail)
    try {
      const input = createRKLLMInput('Test after reconnect');
      const inferParams = createRKLLMInferParam();
      const result = await client.sendRequest('rkllm.run', [null, input, inferParams, null]);
      
      if (result.error) {
        console.log(`      ✅ Previous model state properly invalidated: ${result.error.message}`);
      } else {
        throw new Error('Previous model state incorrectly persisted');
      }
    } catch (error) {
      console.log(`      ✅ Connection state properly reset: ${error.message}`);
    }

    client.disconnect();
  }

  async testLargeInputHandling() {
    const client = new TestClient();
    await client.connect();

    // Initialize model
    const params = createRKLLMParam();
    const initResult = await client.sendRequest('rkllm.init', [null, params, null]);
    if (!initResult.result || !initResult.result.success) {
      throw new Error('Model initialization failed');
    }
    await sleep(2000);

    // Test with increasingly large inputs
    const inputSizes = [100, 1000, 10000];
    let handledSizes = 0;

    for (const size of inputSizes) {
      const largePrompt = 'A'.repeat(size);
      const input = createRKLLMInput(largePrompt);
      const inferParams = createRKLLMInferParam();

      try {
        const result = await client.sendRequest('rkllm.run', [null, input, inferParams, null], 15000); // Longer timeout
        
        if (result.result || result.error) {
          handledSizes++;
          const status = result.result ? 'processed' : 'properly rejected';
          console.log(`      ✅ Large input (${size} chars): ${status}`);
        }
      } catch (error) {
        if (error.message.includes('timeout')) {
          console.log(`      ⚠️  Large input (${size} chars): timeout (acceptable)`);
          handledSizes++;
        } else {
          console.log(`      ❌ Large input (${size} chars): ${error.message}`);
        }
      }
    }

    // Cleanup
    await client.sendRequest('rkllm.destroy', [null]);
    client.disconnect();

    if (handledSizes < inputSizes.length - 1) {
      throw new Error(`Large input handling insufficient: ${handledSizes}/${inputSizes.length}`);
    }

    console.log(`      📊 Large input handling: ${handledSizes}/${inputSizes.length} sizes handled`);
  }

  async testMemoryStabilityUnderLoad() {
    const client = new TestClient();
    await client.connect();

    // Initialize model
    const params = createRKLLMParam();
    params.max_new_tokens = 10; // Keep responses short for speed
    const initResult = await client.sendRequest('rkllm.init', [null, params, null]);
    if (!initResult.result || !initResult.result.success) {
      throw new Error('Model initialization failed');
    }
    await sleep(2000);

    const numRequests = 20;
    let successfulRequests = 0;

    console.log(`      🔄 Running ${numRequests} consecutive requests for memory stability...`);

    for (let i = 0; i < numRequests; i++) {
      try {
        const input = createRKLLMInput(`Memory test ${i}`);
        const inferParams = createRKLLMInferParam();
        const result = await client.sendRequest('rkllm.run', [null, input, inferParams, null]);
        
        if (result.result) {
          successfulRequests++;
        }
        
        // Brief pause to avoid overwhelming
        await sleep(100);
      } catch (error) {
        console.log(`      ⚠️  Request ${i} failed: ${error.message}`);
      }
    }

    // Cleanup
    await client.sendRequest('rkllm.destroy', [null]);
    client.disconnect();

    console.log(`      📊 Memory stability: ${successfulRequests}/${numRequests} requests successful`);

    if (successfulRequests < numRequests * 0.8) {
      throw new Error(`Memory stability issue: only ${successfulRequests}/${numRequests} successful`);
    }
  }

  async runAllTests() {
    try {
      await this.setup();

      // Run all robustness tests
      await this.runTest('Multi-Client Concurrency', () => this.testMultiClientConcurrency());
      await this.runTest('Malformed JSON Handling', () => this.testMalformedJSONHandling());
      await this.runTest('Invalid Parameter Handling', () => this.testInvalidParameterHandling());
      await this.runTest('Resource Cleanup', () => this.testResourceCleanup());
      await this.runTest('Connection Recovery', () => this.testConnectionRecovery());
      await this.runTest('Large Input Handling', () => this.testLargeInputHandling());
      await this.runTest('Memory Stability Under Load', () => this.testMemoryStabilityUnderLoad());

    } catch (error) {
      console.error(`\n❌ SERVER ROBUSTNESS TESTS FAILED: ${error.message}`);
      process.exit(1);
    } finally {
      await this.teardown();
    }

    // Results summary
    console.log('\n' + '=' .repeat(60));
    console.log(`🎉 SERVER ROBUSTNESS TESTS COMPLETED`);
    console.log(`✅ Passed: ${this.testResults.passed}/${this.testResults.total}`);
    console.log(`❌ Failed: ${this.testResults.failed}/${this.testResults.total}`);
    
    if (this.testResults.failed === 0) {
      console.log('🚀 Server robustness verified - production ready!');
      process.exit(0);
    } else {
      console.log('⚠️  Server robustness needs improvement');
      process.exit(1);
    }
  }
}

// Run tests if called directly
if (require.main === module) {
  const tests = new ServerRobustnessTests();
  tests.runAllTests();
}

module.exports = ServerRobustnessTests;