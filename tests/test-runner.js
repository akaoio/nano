#!/usr/bin/env node

/**
 * RKLLM Server Test Runner
 * Comprehensive test suite for all RKLLM functions
 * 
 * Usage: npm test (from project root)
 */

const ServerManager = require('./lib/server-manager');
const TestClient = require('./lib/test-client');

// Import all test modules
const testCreateDefaultParam = require('./core/test-createDefaultParam');
const testInit = require('./core/test-init');
const testGetConstants = require('./utilities/test-get-constants');
const testAllCore = require('./core/test-all-core');
const testAllAdvanced = require('./advanced/test-all-advanced');
const testCompleteWorkflow = require('./integration/test-complete-workflow');

// Import comprehensive test modules
const testRealModelComplete = require('./integration/test-real-model-complete');
const testErrorScenarios = require('./edge-cases/test-error-scenarios');
const testAsyncOperations = require('./advanced/test-async-operations');

/**
 * Main test runner
 */
async function runAllTests() {
  console.log('🚀 RKLLM UNIX DOMAIN SOCKET SERVER - TEST SUITE');
  console.log('=' .repeat(70));
  console.log('📋 Testing complete 1:1 RKLLM API mapping per DESIGN.md');
  console.log('🎯 Goal: Verify all 15 RKLLM functions are properly implemented');
  console.log('');

  const serverManager = new ServerManager();
  const client = new TestClient();
  
  let totalPassed = 0;
  let totalTests = 0;

  try {
    // Start server
    console.log('🔧 SETUP PHASE');
    console.log('=' .repeat(50));
    
    await serverManager.startServer();
    await client.connect();

    console.log('✅ Test environment ready\\n');

    // Define comprehensive test suite
    const testSuite = [
      {
        name: 'Basic Functions',
        tests: [
          { name: 'rkllm.createDefaultParam', fn: testCreateDefaultParam },
          { name: 'rkllm.get_constants', fn: testGetConstants }
        ]
      },
      {
        name: 'Core Functions Mapping',
        tests: [
          { name: 'All Core Functions', fn: testAllCore }
        ]
      },
      {
        name: 'Advanced Functions Mapping', 
        tests: [
          { name: 'All Advanced Functions', fn: testAllAdvanced }
        ]
      },
      {
        name: 'Real Model Testing',
        tests: [
          { name: 'Complete Real Model Tests', fn: testRealModelComplete }
        ]
      },
      {
        name: 'Async & Streaming',
        tests: [
          { name: 'Async Operations & Streaming', fn: testAsyncOperations }
        ]
      },
      {
        name: 'Error Handling',
        tests: [
          { name: 'Error Scenarios & Edge Cases', fn: testErrorScenarios }
        ]
      },
      {
        name: 'Integration Tests',
        tests: [
          { name: 'Complete Workflow', fn: testCompleteWorkflow },
          { name: 'Model Initialization', fn: testInit }
        ]
      }
    ];

    // Run all test categories
    const results = [];
    
    for (const category of testSuite) {
      console.log(`\\n🧪 ${category.name.toUpperCase()}`);
      console.log('=' .repeat(category.name.length + 4));
      
      for (const test of category.tests) {
        totalTests++;
        console.log(`\\n[${totalTests}] Running: ${test.name}`);
        console.log('-' .repeat(40));
        
        try {
          const success = await test.fn(client);
          if (success) totalPassed++;
          
          results.push({
            category: category.name,
            name: test.name,
            success: success
          });
          
        } catch (error) {
          console.error(`❌ Test failed with error: ${error.message}`);
          results.push({
            category: category.name,
            name: test.name,
            success: false,
            error: error.message
          });
        }
      }
    }

    // Print final results
    console.log('\\n' + '=' .repeat(70));
    console.log('📊 FINAL TEST RESULTS');
    console.log('=' .repeat(70));

    // Group results by category
    const categories = {};
    results.forEach(result => {
      if (!categories[result.category]) {
        categories[result.category] = [];
      }
      categories[result.category].push(result);
    });

    // Print category summaries
    Object.entries(categories).forEach(([categoryName, categoryResults]) => {
      const passed = categoryResults.filter(r => r.success).length;
      const total = categoryResults.length;
      const status = passed === total ? '✅' : '⚠️';
      
      console.log(`\\n${status} ${categoryName.toUpperCase()} (${passed}/${total})`);
      
      categoryResults.forEach(result => {
        const status = result.success ? '✅' : '❌';
        console.log(`  ${status} ${result.name}`);
        if (result.error) {
          console.log(`      Error: ${result.error}`);
        }
      });
    });

    // Overall summary
    console.log('\\n' + '=' .repeat(70));
    const passRate = totalPassed / totalTests * 100;
    const overallStatus = totalPassed === totalTests ? '🎉' : '⚠️';
    
    console.log(`${overallStatus} OVERALL RESULTS: ${totalPassed}/${totalTests} tests passed (${passRate.toFixed(1)}%)`);

    if (totalPassed === totalTests) {
      console.log('\\n🎉 ALL TESTS PASSED!');
      console.log('✅ RKLLM Server fully functional');
      console.log('✅ Complete 1:1 API mapping verified');
      console.log('✅ All 15 RKLLM functions properly implemented');
      console.log('✅ JSON-RPC 2.0 compliance confirmed');
      console.log('✅ Ultra-modular architecture working correctly');
    } else {
      console.log('\\n⚠️  Some tests failed or models unavailable');
      console.log('📋 This may be expected if:');
      console.log('   - Model files are not present at expected paths');
      console.log('   - NPU hardware is not available');
      console.log('   - RKLLM library dependencies are missing');
      console.log('');
      console.log('🎯 Key success criteria:');
      console.log('   ✅ All functions should be properly mapped (not "Method not found")');
      console.log('   ✅ JSON-RPC requests should be accepted and processed');
      console.log('   ✅ Parameter structures should match DESIGN.md specification');
    }

    console.log('\\n' + '=' .repeat(70));
    console.log('🏁 TEST SUITE COMPLETE');
    console.log('=' .repeat(70));

  } catch (error) {
    console.error('\\n❌ TEST SUITE FAILED:', error.message);
    console.error('   Check server build and dependencies');
  } finally {
    // Cleanup
    console.log('\\n🧹 Cleaning up...');
    client.disconnect();
    serverManager.stopServer();
  }

  // Exit with appropriate code
  process.exit(totalPassed === totalTests ? 0 : 1);
}

// Handle uncaught errors
process.on('uncaughtException', (error) => {
  console.error('\\n💥 Uncaught Exception:', error.message);
  process.exit(1);
});

process.on('unhandledRejection', (reason, promise) => {
  console.error('\\n💥 Unhandled Rejection:', reason);
  process.exit(1);
});

// Run the test suite
if (require.main === module) {
  runAllTests();
}

module.exports = runAllTests;