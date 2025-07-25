const concurrently = require('concurrently');
const path = require('path');
const fs = require('fs');

async function main() {
    console.log('Starting concurrent server and test execution...');
    
    // Ensure socket doesn't exist from previous runs
    try {
        fs.unlinkSync('/tmp/rkllm.sock');
    } catch (e) {
        // Socket doesn't exist, that's fine
    }
    
    const commands = [
        {
            name: 'Server',
            command: 'cd build && ./server',
            delay: 0
        },
        {
            name: 'Client Test',
            command: 'sleep 5 && cd sandbox && node comprehensive_test.js',
            delay: 5000
        }
    ];
    
    try {
        const { result } = concurrently(commands, {
            prefix: 'name',
            killOthers: ['failure', 'success'],
            restartTries: 0,
            handleInput: false
        });
        
        await result;
        console.log('\n✅ All processes completed successfully');
        
    } catch (error) {
        console.error('\n❌ Process failed:', error);
        
        // Save detailed logs for investigation
        const logPath = './sandbox/test_results/error_log.txt';
        fs.writeFileSync(logPath, `
Test run failed at: ${new Date().toISOString()}
Error: ${error.message}
Stack: ${error.stack}
        `);
        
        console.log(`Error details saved to: ${logPath}`);
        process.exit(1);
    }
}

if (require.main === module) {
    main().catch(console.error);
}