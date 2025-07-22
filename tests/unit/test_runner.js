import { runTests as runStreamingCoreTests } from './test_streaming_core.js';

async function runAllUnitTests() {
    console.log('🧪 Running Unit Test Suite');
    console.log('===========================');
    
    try {
        // Run streaming core tests
        await runStreamingCoreTests();
        
        console.log('\n✅ All unit tests passed!');
        process.exit(0);
    } catch (error) {
        console.error('\n❌ Unit tests failed:', error);
        process.exit(1);
    }
}

runAllUnitTests();