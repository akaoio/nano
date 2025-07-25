const concurrently = require('concurrently');
const path = require('path');
const fs = require('fs');

async function main() {
    console.log('🚀 Starting Concurrent RKLLM+RKNN Server and Comprehensive Multimodal Testing...');
    console.log('This test follows the proper Qwen2-VL workflow from external examples');
    
    // Ensure socket doesn't exist from previous runs
    const socketPath = '/tmp/rkllm.sock';
    try {
        fs.unlinkSync(socketPath);
        console.log(`🧹 Cleaned up existing socket: ${socketPath}`);
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
            command: 'sleep 8 && cd sandbox && node correct_multimodal_test.js',
            delay: 8000
        }
    ];
    
    try {
        console.log('\\nStarting processes:');
        console.log('📘 SERVER: RKLLM+RKNN Unix Domain Socket Server');
        console.log('📗 TESTS: Comprehensive Multimodal Test Suite');
        console.log('\\n' + '=' * 70);
        
        const { result } = concurrently(commands, {
            prefix: 'name',
            killOthers: ['failure', 'success'],
            restartTries: 0,
            handleInput: false,
            timestampFormat: 'HH:mm:ss.SSS'
        });
        
        await result;
        console.log('\\n✅ All processes completed successfully');
        console.log('🎉 COMPREHENSIVE MULTIMODAL TESTING COMPLETED!');
        
        // Save success log
        const successLogPath = './sandbox/test_results/success_log.txt';
        fs.mkdirSync(path.dirname(successLogPath), { recursive: true });
        fs.writeFileSync(successLogPath, `
Comprehensive Multimodal Test SUCCESS
=====================================
Completed at: ${new Date().toISOString()}
Models tested: 
- RKNN Vision Encoder: ./models/qwen2vl2b/Qwen2-VL-2B-Instruct.rknn
- RKLLM Language Model: ./models/qwen2vl2b/Qwen2-VL-2B-Instruct.rkllm
Test images:
- ./tests/images/image1.jpg  
- ./tests/images/image2.png
Features tested:
- RKNN+RKLLM initialization
- Image processing with vision encoder
- Multimodal inference (image + text)
- Pure text generation
- Resource cleanup
- JSON-RPC 2.0 protocol compliance
- Socket communication
        `);
        
        console.log(`\\n📄 Success details saved to: ${successLogPath}`);
        
    } catch (error) {
        console.error('\\n❌ Process failed:', error);
        
        // Save detailed error logs for investigation
        const errorLogPath = './sandbox/test_results/error_log.txt';
        fs.mkdirSync(path.dirname(errorLogPath), { recursive: true });
        fs.writeFileSync(errorLogPath, `
Comprehensive Multimodal Test FAILURE
=====================================
Failed at: ${new Date().toISOString()}
Error: ${error.message}
Stack: ${error.stack}

Commands that were running:
${JSON.stringify(commands, null, 2)}

Possible issues to investigate:
1. Check if models exist at specified paths
2. Verify server can bind to socket path: ${socketPath}
3. Check image files exist in tests/images/
4. Validate NPU cores are available
5. Ensure sufficient memory for model loading
6. Check for library dependencies (librkllmrt.so, librknnrt.so)
        `);
        
        console.log(`\\n📄 Error details saved to: ${errorLogPath}`);
        
        // Also log to console for immediate visibility
        console.error('\\n🔍 DEBUGGING INFORMATION:');
        console.error('- Server socket path:', socketPath);
        console.error('- Test images required:');
        console.error('  * ./tests/images/image1.jpg');
        console.error('  * ./tests/images/image2.png');
        console.error('- Models required:');
        console.error('  * ./models/qwen2vl2b/Qwen2-VL-2B-Instruct.rknn');
        console.error('  * ./models/qwen2vl2b/Qwen2-VL-2B-Instruct.rkllm');
        
        process.exit(1);
    }
}

if (require.main === module) {
    main().catch(console.error);
}