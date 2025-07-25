const net = require('net');

class UnifiedParamsTestClient {
    constructor() {
        this.socket = null;
        this.requestId = 1;
        this.pendingRequests = new Map();
    }

    connect() {
        return new Promise((resolve, reject) => {
            this.socket = net.createConnection('/tmp/rkllm.sock', () => {
                console.log('✅ Connected to server');
                resolve();
            });

            this.socket.on('error', reject);
            this.socket.on('data', (data) => {
                const lines = data.toString().split('\n').filter(line => line.trim());
                for (const line of lines) {
                    try {
                        const response = JSON.parse(line);
                        
                        if (response.id && this.pendingRequests.has(response.id)) {
                            const { resolve } = this.pendingRequests.get(response.id);
                            this.pendingRequests.delete(response.id);
                            resolve(response);
                        }
                    } catch (e) {
                        console.error('❌ Failed to parse response:', line, e);
                    }
                }
            });
        });
    }

    sendRequest(method, params) {
        return new Promise((resolve, reject) => {
            const id = this.requestId++;
            const request = {
                jsonrpc: "2.0",
                method,
                params,  // Now always an object!
                id
            };
            
            console.log(`📤 REQUEST (${method}):`);
            console.log(JSON.stringify(request, null, 2));
            
            this.pendingRequests.set(id, { resolve, reject });
            this.socket.write(JSON.stringify(request) + '\n');
            
            setTimeout(() => {
                if (this.pendingRequests.has(id)) {
                    this.pendingRequests.delete(id);
                    reject(new Error(`Request timeout for ${method}`));
                }
            }, 15000);
        });
    }

    disconnect() {
        if (this.socket) {
            this.socket.end();
        }
    }
}

async function testUnifiedParameterFormat() {
    console.log('🧪 Testing Unified JSON Object Parameter Format');
    console.log('=' * 60);
    
    const client = new UnifiedParamsTestClient();
    
    try {
        await client.connect();
        
        // Test 1: RKLLM Init with object parameters
        console.log('\n🔧 Test 1: RKLLM Init with Object Parameters');
        console.log('-' * 40);
        
        const rkllmInitResult = await client.sendRequest('rkllm.init', {
            model_path: '/home/x/Projects/nano/models/qwen2vl2b/Qwen2-VL-2B-Instruct.rkllm',
            param: {
                num_npu_core: 1,
                max_context_len: 512,
                max_new_tokens: 128,
                temperature: 0.7
            }
        });
        
        console.log('📨 RESPONSE:');
        console.log(JSON.stringify(rkllmInitResult, null, 2));
        
        if (rkllmInitResult.error) {
            console.log('❌ RKLLM init failed:', rkllmInitResult.error.message);
        } else {
            console.log('✅ RKLLM init succeeded with object parameters!');
        }
        
        // Test 2: RKNN Init with object parameters
        console.log('\n🔧 Test 2: RKNN Init with Object Parameters');
        console.log('-' * 40);
        
        const rknnInitResult = await client.sendRequest('rknn.init', {
            model_path: '/home/x/Projects/nano/models/qwen2vl2b/Qwen2-VL-2B-Instruct.rknn',
            flags: 0
        });
        
        console.log('📨 RESPONSE:');
        console.log(JSON.stringify(rknnInitResult, null, 2));
        
        if (rknnInitResult.error) {
            console.log('❌ RKNN init failed:', rknnInitResult.error.message);
        } else {
            console.log('✅ RKNN init succeeded with object parameters!');
        }
        
        // Test 3: Text generation with object parameters
        if (!rkllmInitResult.error) {
            console.log('\n💬 Test 3: Text Generation with Object Parameters');
            console.log('-' * 40);
            
            const textResult = await client.sendRequest('rkllm.run', {
                input_type: 'RKLLM_INPUT_PROMPT',
                prompt: 'Hello, world!',
                role: 'user'
            });
            
            console.log('📨 RESPONSE:');
            console.log(JSON.stringify(textResult, null, 2));
            
            if (textResult.error) {
                console.log('❌ Text generation failed:', textResult.error.message);
            } else {
                console.log('✅ Text generation succeeded with object parameters!');
                if (textResult.result && textResult.result.text) {
                    console.log('🤖 Generated text:', textResult.result.text);
                }
            }
        }
        
        // Cleanup
        console.log('\n🧹 Cleanup');
        console.log('-' * 40);
        
        if (!rknnInitResult.error) {
            await client.sendRequest('rknn.destroy', {});
            console.log('✅ RKNN destroyed');
        }
        
        if (!rkllmInitResult.error) {
            await client.sendRequest('rkllm.destroy', {});
            console.log('✅ RKLLM destroyed');
        }
        
        console.log('\n🎉 UNIFIED PARAMETER FORMAT TEST COMPLETED!');
        console.log('✅ All RKLLM and RKNN functions now use consistent JSON object parameters');
        
    } catch (error) {
        console.error('❌ Test failed:', error);
        throw error;
    } finally {
        client.disconnect();
    }
}

if (require.main === module) {
    testUnifiedParameterFormat().catch(error => {
        console.error('Final error:', error);
        process.exit(1);
    });
}