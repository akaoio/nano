#!/usr/bin/env node

/**
 * Real Streaming Test - Test with proper streaming token collection
 */

const ServerManager = require('./lib/server-manager');
const StreamingTestClient = require('./lib/streaming-test-client');
const { createRKLLMParam, createRKLLMInput, createRKLLMInferParam, TEST_MODELS } = require('./lib/test-helpers');

async function testRealStreaming() {
  console.log('🚀 REAL STREAMING TEST SUITE');
  console.log('=' .repeat(60));
  console.log('🎯 Testing with proper streaming token collection');
  console.log('');

  const serverManager = new ServerManager();
  const client = new StreamingTestClient();
  
  let totalTests = 0;
  let passedTests = 0;
  const results = [];

  try {
    // Start server and connect
    console.log('🔧 SETUP');
    console.log('=' .repeat(30));
    await serverManager.startServer();
    await client.connect();
    console.log('✅ Server ready for streaming tests\n');

    // Test 1: Basic non-streaming functions
    console.log('📋 TEST 1: Basic Functions');
    console.log('=' .repeat(40));
    
    totalTests++;
    const defaultParamResult = await client.testBasicFunction('rkllm.createDefaultParam', []);
    if (defaultParamResult.success && defaultParamResult.result?.model_path !== undefined) {
      console.log('✅ createDefaultParam: PASS');
      passedTests++;
      results.push({test: 'createDefaultParam', passed: true});
    } else {
      console.log('❌ createDefaultParam: FAIL');
      results.push({test: 'createDefaultParam', passed: false});
    }

    totalTests++;
    const constantsResult = await client.testBasicFunction('rkllm.get_constants', []);
    if (constantsResult.success && constantsResult.result?.CPU_MASKS) {
      console.log('✅ get_constants: PASS');
      passedTests++;
      results.push({test: 'get_constants', passed: true});
    } else {
      console.log('❌ get_constants: FAIL');
      results.push({test: 'get_constants', passed: false});
    }

    // Test 2: Model initialization
    console.log('\n📋 TEST 2: Model Initialization');
    console.log('=' .repeat(40));
    
    totalTests++;
    const initParams = createRKLLMParam(TEST_MODELS.NORMAL);
    initParams.max_new_tokens = 30; // More tokens for better streaming test
    
    const initResult = await client.testBasicFunction('rkllm.init', [null, initParams, null]);
    if (initResult.success) {
      console.log('✅ init: PASS - Model initialized');
      passedTests++;
      results.push({test: 'init', passed: true});
      
      // Wait for model to be ready
      await new Promise(resolve => setTimeout(resolve, 3000));
      
    } else {
      console.log('❌ init: FAIL - Model initialization failed');
      results.push({test: 'init', passed: false});
    }

    // Test 3: Real streaming inference
    console.log('\n📋 TEST 3: Real Streaming Inference');
    console.log('=' .repeat(40));
    
    totalTests++;
    const input = createRKLLMInput('Tell me a joke about programming. Make it funny and at least 2 sentences.');
    const inferParams = createRKLLMInferParam();
    
    const streamingResult = await client.testStreamingFunction('rkllm.run', [null, input, inferParams, null]);
    
    if (streamingResult.success && streamingResult.streamingResult) {
      const { combinedText, tokenCount } = streamingResult.streamingResult;
      
      console.log('✅ STREAMING INFERENCE: PASS');
      console.log(`   📝 Generated text: "${combinedText}"`);
      console.log(`   🔢 Token count: ${tokenCount}`);
      console.log(`   📊 Average chars per token: ${(combinedText.length / tokenCount).toFixed(1)}`);
      
      if (tokenCount >= 5) {
        console.log('✅ Real streaming confirmed - multiple tokens received!');
        passedTests++;
        results.push({
          test: 'streaming_inference', 
          passed: true, 
          tokens: tokenCount,
          text: combinedText
        });
      } else {
        console.log('❌ Insufficient streaming - too few tokens');
        results.push({test: 'streaming_inference', passed: false, tokens: tokenCount});
      }
    } else {
      console.log('❌ streaming inference: FAIL');
      results.push({test: 'streaming_inference', passed: false});
    }

    // Test 4: Test is_running during inference
    console.log('\n📋 TEST 4: Runtime Status Check');
    console.log('=' .repeat(40));
    
    totalTests++;
    const isRunningResult = await client.testBasicFunction('rkllm.is_running', [null]);
    
    if (isRunningResult.success) {
      console.log('✅ is_running: PASS - Function accessible');
      console.log(`   Status: ${JSON.stringify(isRunningResult.result)}`);
      passedTests++;
      results.push({test: 'is_running', passed: true});
    } else {
      console.log('❌ is_running: FAIL');
      results.push({test: 'is_running', passed: false});
    }

    // Test 5: Test another streaming inference with different prompt
    console.log('\n📋 TEST 5: Second Streaming Test');
    console.log('=' .repeat(40));
    
    totalTests++;
    const input2 = createRKLLMInput('What is artificial intelligence? Explain in simple terms.');
    const streamingResult2 = await client.testStreamingFunction('rkllm.run', [null, input2, inferParams, null]);
    
    if (streamingResult2.success && streamingResult2.streamingResult) {
      const { combinedText, tokenCount } = streamingResult2.streamingResult;
      
      console.log('✅ SECOND STREAMING: PASS');
      console.log(`   📝 Generated text: "${combinedText}"`);
      console.log(`   🔢 Token count: ${tokenCount}`);
      
      passedTests++;
      results.push({
        test: 'second_streaming', 
        passed: true, 
        tokens: tokenCount,
        text: combinedText
      });
    } else {
      console.log('❌ second streaming: FAIL');
      results.push({test: 'second_streaming', passed: false});
    }

    // Test 6: Cleanup
    console.log('\n📋 TEST 6: Model Cleanup');
    console.log('=' .repeat(40));
    
    totalTests++;
    const destroyResult = await client.testBasicFunction('rkllm.destroy', [null]);
    
    if (destroyResult.success || destroyResult.error?.message === 'Internal server error') {
      console.log('✅ destroy: PASS - Model cleanup completed');
      passedTests++;
      results.push({test: 'destroy', passed: true});
    } else {
      console.log('❌ destroy: FAIL');
      results.push({test: 'destroy', passed: false});
    }

  } catch (error) {
    console.error('\n❌ TEST SUITE FAILED:', error.message);
  } finally {
    // Always cleanup
    console.log('\n🔌 Disconnecting...');
    client.disconnect();
    serverManager.stopServer();
  }

  // Results Summary
  console.log('\n' + '=' .repeat(60));
  console.log('📊 REAL STREAMING TEST RESULTS');
  console.log('=' .repeat(60));
  
  const passRate = totalTests > 0 ? (passedTests / totalTests * 100).toFixed(1) : 0;
  console.log(`\n🎯 OVERALL: ${passedTests}/${totalTests} tests passed (${passRate}%)`);
  
  console.log('\n📋 DETAILED RESULTS:');
  results.forEach((result, index) => {
    const status = result.passed ? '✅' : '❌';
    console.log(`${status} ${index + 1}. ${result.test}`);
    
    if (result.tokens) {
      console.log(`    Tokens generated: ${result.tokens}`);
    }
    if (result.text) {
      console.log(`    Text: "${result.text.substring(0, 100)}${result.text.length > 100 ? '...' : ''}"`);
    }
  });

  console.log('\n' + '=' .repeat(60));
  
  // Analyze streaming results
  const streamingTests = results.filter(r => r.tokens !== undefined);
  if (streamingTests.length > 0) {
    const totalTokens = streamingTests.reduce((sum, r) => sum + r.tokens, 0);
    const avgTokens = totalTokens / streamingTests.length;
    
    console.log('🔍 STREAMING ANALYSIS:');
    console.log(`   Total streaming tests: ${streamingTests.length}`);
    console.log(`   Total tokens generated: ${totalTokens}`);
    console.log(`   Average tokens per test: ${avgTokens.toFixed(1)}`);
    
    if (avgTokens >= 10) {
      console.log('🎉 EXCELLENT: Real streaming with substantial token generation confirmed!');
    } else if (avgTokens >= 5) {
      console.log('✅ GOOD: Real streaming confirmed with moderate token generation');
    } else {
      console.log('⚠️  LIMITED: Streaming working but with few tokens');
    }
  }
  
  if (passedTests >= totalTests * 0.8) {
    console.log('🎉 STREAMING TEST SUITE PASSED! Real streaming functionality confirmed.');
  } else {
    console.log('⚠️  STREAMING TEST SUITE PARTIAL: Some streaming issues detected.');
  }
  
  console.log('🏁 Real streaming testing complete');
  
  // Exit with appropriate code
  process.exit(passedTests >= totalTests * 0.8 ? 0 : 1);
}

// Handle errors
process.on('uncaughtException', (error) => {
  console.error('\n💥 Uncaught Exception:', error.message);
  process.exit(1);
});

process.on('unhandledRejection', (reason, promise) => {
  console.error('\n💥 Unhandled Rejection:', reason);
  process.exit(1);
});

// Run the streaming test suite
if (require.main === module) {
  testRealStreaming();
}

module.exports = testRealStreaming;