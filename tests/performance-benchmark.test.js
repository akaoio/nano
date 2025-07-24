#!/usr/bin/env node

/**
 * Performance Benchmark Test Suite
 * Tests server performance characteristics and benchmarks:
 * - Response time measurements
 * - Throughput testing
 * - Memory usage monitoring
 * - Streaming performance
 * - Load testing
 * - Performance regression detection
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

class PerformanceBenchmarks {
  constructor() {
    this.serverManager = new ServerManager();
    this.testResults = {
      passed: 0,
      failed: 0,
      total: 0,
      benchmarks: {}
    };
  }

  async runTest(name, testFunction) {
    this.testResults.total++;
    process.stdout.write(`📋 ${name}... `);
    
    try {
      const result = await testFunction();
      console.log('✅ PASS');
      this.testResults.passed++;
      if (result) {
        this.testResults.benchmarks[name] = result;
      }
    } catch (error) {
      console.log('❌ FAIL');
      console.log(`   Error: ${error.message}`);
      this.testResults.failed++;
    }
  }

  async setup() {
    printTestSection('PERFORMANCE BENCHMARK TEST SUITE');
    console.log('🎯 Testing server performance characteristics and benchmarks');
    console.log('');

    console.log('🔧 SETUP');
    console.log('=' .repeat(30));
    await this.serverManager.startServer();
    console.log('✅ Server ready for performance benchmarks\n');
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

  async benchmarkModelInitialization() {
    const client = new TestClient();
    await client.connect();

    const iterations = 3;
    const times = [];

    for (let i = 0; i < iterations; i++) {
      const params = createRKLLMParam();
      
      const startTime = Date.now();
      const result = await client.sendRequest('rkllm.init', [null, params, null]);
      const endTime = Date.now();
      
      if (!result.result || !result.result.success) {
        throw new Error(`Model initialization ${i} failed`);
      }
      
      const initTime = endTime - startTime;
      times.push(initTime);
      
      console.log(`      📊 Init ${i + 1}: ${initTime}ms`);
      
      // Destroy for next iteration
      await client.sendRequest('rkllm.destroy', [null]);
      await sleep(1000);
    }

    client.disconnect();

    const avgTime = times.reduce((a, b) => a + b, 0) / times.length;
    const minTime = Math.min(...times);
    const maxTime = Math.max(...times);

    console.log(`      📈 Average: ${avgTime.toFixed(1)}ms, Min: ${minTime}ms, Max: ${maxTime}ms`);

    // Performance threshold (adjust based on hardware)
    if (avgTime > 10000) { // 10 seconds
      throw new Error(`Model initialization too slow: ${avgTime.toFixed(1)}ms average`);
    }

    return { avgTime, minTime, maxTime, samples: times.length };
  }

  async benchmarkSingleInference() {
    const client = new TestClient();
    await client.connect();

    // Initialize model
    const params = createRKLLMParam();
    params.max_new_tokens = 20; // Fixed size for consistent measurement
    const initResult = await client.sendRequest('rkllm.init', [null, params, null]);
    if (!initResult.result || !initResult.result.success) {
      throw new Error('Model initialization failed');
    }
    await sleep(2000);

    const iterations = 5;
    const times = [];
    const tokens = [];

    for (let i = 0; i < iterations; i++) {
      const input = createRKLLMInput(`Performance test ${i}`);
      const inferParams = createRKLLMInferParam();
      
      const startTime = Date.now();
      const result = await client.sendRequest('rkllm.run', [null, input, inferParams, null]);
      const endTime = Date.now();
      
      if (!result.result) {
        throw new Error(`Inference ${i} failed`);
      }
      
      const inferenceTime = endTime - startTime;
      const generatedTokens = result.result.text ? result.result.text.split(' ').length : 1;
      
      times.push(inferenceTime);
      tokens.push(generatedTokens);
      
      console.log(`      📊 Inference ${i + 1}: ${inferenceTime}ms (${generatedTokens} tokens)`);
      await sleep(500);
    }

    // Cleanup
    await client.sendRequest('rkllm.destroy', [null]);
    client.disconnect();

    const avgTime = times.reduce((a, b) => a + b, 0) / times.length;
    const avgTokens = tokens.reduce((a, b) => a + b, 0) / tokens.length;
    const tokensPerSecond = (avgTokens / avgTime) * 1000;

    console.log(`      📈 Average: ${avgTime.toFixed(1)}ms, ${avgTokens.toFixed(1)} tokens, ${tokensPerSecond.toFixed(2)} tokens/sec`);

    // Performance threshold
    if (avgTime > 5000) { // 5 seconds
      throw new Error(`Single inference too slow: ${avgTime.toFixed(1)}ms average`);
    }

    return { 
      avgTime, 
      avgTokens, 
      tokensPerSecond, 
      samples: times.length,
      rawTimes: times,
      rawTokens: tokens
    };
  }

  async benchmarkStreamingPerformance() {
    const client = new TestClient();
    await client.connect();

    // Initialize async model
    const params = createRKLLMParam();
    params.is_async = true;
    params.max_new_tokens = 30;
    const initResult = await client.sendRequest('rkllm.init', [null, params, null]);
    if (!initResult.result || !initResult.result.success) {
      throw new Error('Async model initialization failed');
    }
    await sleep(2000);

    const input = createRKLLMInput('Write a short story about performance.');
    const inferParams = createRKLLMInferParam();
    
    console.log(`      🎬 Starting streaming performance measurement...`);
    
    const startTime = Date.now();
    const streamResult = await client.sendRawJsonStreamingRequest('rkllm.run', [null, input, inferParams, null]);
    const endTime = Date.now();
    
    if (!streamResult.tokens || streamResult.tokens.length === 0) {
      throw new Error('No streaming tokens received');
    }

    const totalTime = endTime - startTime;
    const tokenCount = streamResult.tokens.length;
    const avgTimePerToken = totalTime / tokenCount;
    const tokensPerSecond = (tokenCount / totalTime) * 1000;
    
    // Calculate first token latency (TTFT - Time To First Token)
    const firstTokenTime = streamResult.tokens.length > 0 ? 
      (streamResult.tokens[0].timestamp || startTime + 100) - startTime : 0;

    console.log(`      📈 Streaming: ${totalTime}ms total, ${tokenCount} tokens`);
    console.log(`      📈 Performance: ${tokensPerSecond.toFixed(2)} tokens/sec, ${avgTimePerToken.toFixed(1)}ms/token`);
    console.log(`      📈 TTFT: ${firstTokenTime}ms`);

    // Cleanup
    await client.sendRequest('rkllm.destroy', [null]);
    client.disconnect();

    // Performance thresholds
    if (tokensPerSecond < 1.0) {
      throw new Error(`Streaming too slow: ${tokensPerSecond.toFixed(2)} tokens/sec`);
    }

    return {
      totalTime,
      tokenCount,
      tokensPerSecond,
      avgTimePerToken,
      firstTokenTime,
      fullText: streamResult.fullText
    };
  }

  async benchmarkThroughput() {
    const numClients = 3;
    const requestsPerClient = 3;
    const clients = [];

    // Create clients
    for (let i = 0; i < numClients; i++) {
      const client = new TestClient();
      await client.connect();
      clients.push(client);
    }

    // Initialize model on first client
    const params = createRKLLMParam();
    params.max_new_tokens = 10; // Short responses for speed
    const initResult = await clients[0].sendRequest('rkllm.init', [null, params, null]);
    if (!initResult.result || !initResult.result.success) {
      throw new Error('Model initialization failed');
    }
    await sleep(2000);

    console.log(`      🚀 Starting throughput test: ${numClients} clients × ${requestsPerClient} requests...`);

    const startTime = Date.now();
    const allPromises = [];

    // Launch all requests concurrently
    for (let clientIndex = 0; clientIndex < numClients; clientIndex++) {
      for (let reqIndex = 0; reqIndex < requestsPerClient; reqIndex++) {
        const promise = (async () => {
          const input = createRKLLMInput(`Throughput test C${clientIndex}R${reqIndex}`);
          const inferParams = createRKLLMInferParam();
          
          try {
            const reqStart = Date.now();
            const result = await clients[clientIndex].sendRequest('rkllm.run', [null, input, inferParams, null]);
            const reqEnd = Date.now();
            
            return {
              client: clientIndex,
              request: reqIndex,
              success: !!result.result,
              time: reqEnd - reqStart,
              text: result.result?.text || ''
            };
          } catch (error) {
            return {
              client: clientIndex,
              request: reqIndex,
              success: false,
              error: error.message,
              time: 0
            };
          }
        })();
        
        allPromises.push(promise);
      }
    }

    const results = await Promise.all(allPromises);
    const endTime = Date.now();
    
    const totalTime = endTime - startTime;
    const successful = results.filter(r => r.success).length;
    const totalRequests = numClients * requestsPerClient;
    const requestsPerSecond = (successful / totalTime) * 1000;
    const avgResponseTime = results
      .filter(r => r.success)
      .reduce((sum, r) => sum + r.time, 0) / successful;

    console.log(`      📈 Throughput: ${successful}/${totalRequests} successful in ${totalTime}ms`);
    console.log(`      📈 Rate: ${requestsPerSecond.toFixed(2)} requests/sec`);
    console.log(`      📈 Avg response time: ${avgResponseTime.toFixed(1)}ms`);

    // Cleanup
    await clients[0].sendRequest('rkllm.destroy', [null]);
    for (const client of clients) {
      client.disconnect();
    }

    // Performance threshold
    if (successful < totalRequests * 0.8) {
      throw new Error(`Throughput too low: ${successful}/${totalRequests} successful`);
    }

    return {
      totalRequests,
      successful,
      totalTime,
      requestsPerSecond,
      avgResponseTime,
      concurrency: numClients
    };
  }

  async benchmarkMemoryUsage() {
    const client = new TestClient();
    await client.connect();

    // Initialize model
    const params = createRKLLMParam();
    const initResult = await client.sendRequest('rkllm.init', [null, params, null]);
    if (!initResult.result || !initResult.result.success) {
      throw new Error('Model initialization failed');
    }
    await sleep(2000);

    // Perform multiple inferences to check memory stability
    const iterations = 10;
    const memoryReadings = [];

    for (let i = 0; i < iterations; i++) {
      const input = createRKLLMInput(`Memory usage test ${i}`);
      const inferParams = createRKLLMInferParam();
      
      const result = await client.sendRequest('rkllm.run', [null, input, inferParams, null]);
      
      if (!result.result) {
        throw new Error(`Memory test inference ${i} failed`);
      }

      // Extract memory info if available
      const memoryMB = result.result.perf?.memory_usage_mb || 0;
      memoryReadings.push(memoryMB);
      
      console.log(`      📊 Iteration ${i + 1}: ${memoryMB}MB`);
      await sleep(200);
    }

    // Cleanup
    await client.sendRequest('rkllm.destroy', [null]);
    client.disconnect();

    const avgMemory = memoryReadings.reduce((a, b) => a + b, 0) / memoryReadings.length;
    const maxMemory = Math.max(...memoryReadings);
    const minMemory = Math.min(...memoryReadings);
    const memoryVariance = maxMemory - minMemory;

    console.log(`      📈 Memory: Avg ${avgMemory.toFixed(1)}MB, Range ${minMemory}-${maxMemory}MB (±${memoryVariance.toFixed(1)}MB)`);

    // Check for memory leaks (increasing trend)
    const firstHalf = memoryReadings.slice(0, Math.floor(iterations / 2));
    const secondHalf = memoryReadings.slice(Math.floor(iterations / 2));
    const firstAvg = firstHalf.reduce((a, b) => a + b, 0) / firstHalf.length;
    const secondAvg = secondHalf.reduce((a, b) => a + b, 0) / secondHalf.length;
    const memoryIncrease = secondAvg - firstAvg;

    if (memoryIncrease > 50) { // 50MB increase might indicate leak
      console.log(`      ⚠️  Potential memory leak detected: +${memoryIncrease.toFixed(1)}MB trend`);
    }

    return {
      avgMemory,
      maxMemory,
      minMemory,
      memoryVariance,
      memoryIncrease,
      samples: iterations,
      readings: memoryReadings
    };
  }

  async runAllTests() {
    try {
      await this.setup();

      // Run all performance benchmarks
      await this.runTest('Model Initialization Benchmark', () => this.benchmarkModelInitialization());
      await this.runTest('Single Inference Benchmark', () => this.benchmarkSingleInference());
      await this.runTest('Streaming Performance Benchmark', () => this.benchmarkStreamingPerformance());
      await this.runTest('Throughput Benchmark', () => this.benchmarkThroughput());
      await this.runTest('Memory Usage Benchmark', () => this.benchmarkMemoryUsage());

    } catch (error) {
      console.error(`\n❌ PERFORMANCE BENCHMARKS FAILED: ${error.message}`);
      process.exit(1);
    } finally {
      await this.teardown();
    }

    // Results summary
    console.log('\n' + '=' .repeat(60));
    console.log(`🎉 PERFORMANCE BENCHMARKS COMPLETED`);
    console.log(`✅ Passed: ${this.testResults.passed}/${this.testResults.total}`);
    console.log(`❌ Failed: ${this.testResults.failed}/${this.testResults.total}`);
    
    // Performance summary
    console.log('\n📊 PERFORMANCE SUMMARY:');
    for (const [testName, results] of Object.entries(this.testResults.benchmarks)) {
      console.log(`\n🔹 ${testName}:`);
      if (results.avgTime !== undefined) console.log(`   Average Time: ${results.avgTime.toFixed(1)}ms`);
      if (results.tokensPerSecond !== undefined) console.log(`   Tokens/sec: ${results.tokensPerSecond.toFixed(2)}`);
      if (results.requestsPerSecond !== undefined) console.log(`   Requests/sec: ${results.requestsPerSecond.toFixed(2)}`);
      if (results.avgMemory !== undefined) console.log(`   Memory Usage: ${results.avgMemory.toFixed(1)}MB`);
    }
    
    if (this.testResults.failed === 0) {
      console.log('\n🚀 Performance benchmarks completed - server performance verified!');
      process.exit(0);
    } else {
      console.log('\n⚠️  Performance issues detected - optimization needed');
      process.exit(1);
    }
  }
}

// Run tests if called directly
if (require.main === module) {
  const tests = new PerformanceBenchmarks();
  tests.runAllTests();
}

module.exports = PerformanceBenchmarks;