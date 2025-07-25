const net = require('net');
const fs = require('fs');

class SimpleImageTester {
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
                        console.error('Parse error:', e);
                    }
                }
            });
        });
    }

    sendRequest(method, params, showParams = true) {
        return new Promise((resolve, reject) => {
            const id = this.requestId++;
            const request = { jsonrpc: "2.0", method, params, id };
            
            if (showParams) {
                console.log(`\n📤 REQUEST: ${method}`);
                if (method === 'rknn.inputs_set' && params.inputs && params.inputs[0].data) {
                    console.log('Parameters (image data truncated):');
                    const p = { ...params };
                    p.inputs[0] = { ...params.inputs[0], data: '[IMAGE_DATA]' };
                    console.log(JSON.stringify(p, null, 2));
                } else {
                    console.log('Parameters:');
                    console.log(JSON.stringify(params, null, 2));
                }
            }
            
            this.pendingRequests.set(id, { resolve, reject });
            this.socket.write(JSON.stringify(request) + '\n');
            
            setTimeout(() => {
                if (this.pendingRequests.has(id)) {
                    this.pendingRequests.delete(id);
                    reject(new Error(`Timeout: ${method}`));
                }
            }, 30000);
        });
    }

    disconnect() { if (this.socket) this.socket.end(); }
}

async function testImageDescription() {
    console.log('🎯 SIMPLE TEST: Qwen2-VL-2B describes image2.png');
    console.log('='*60);
    
    const client = new SimpleImageTester();
    
    try {
        await client.connect();
        
        // Step 1: Initialize RKLLM only (skip RKNN for now to debug params)
        console.log('\n🔧 Step 1: Initialize RKLLM Language Model');
        const rkllmResult = await client.sendRequest('rkllm.init', {
            model_path: './models/qwen2vl2b/Qwen2-VL-2B-Instruct.rkllm',
            param: {
                max_context_len: 1024,
                max_new_tokens: 256,
                temperature: 0.8,
                top_k: 1
            }
        });
        
        console.log('📨 RKLLM Response:');
        console.log(JSON.stringify(rkllmResult, null, 2));
        
        if (rkllmResult.error) {
            throw new Error('RKLLM init failed: ' + rkllmResult.error.message);
        }
        
        console.log('✅ RKLLM initialized successfully');
        
        // Step 2: Test simple text generation (no images yet)
        console.log('\n💬 Step 2: Test Simple Text Generation');
        const textResult = await client.sendRequest('rkllm.run', {
            input_type: 'RKLLM_INPUT_PROMPT',
            prompt: 'Describe what a cute kitten looks like.',
            role: 'user'
        });
        
        console.log('📨 Text Generation Response:');
        console.log(JSON.stringify(textResult, null, 2));
        
        if (textResult.result && textResult.result.text) {
            console.log('\n🤖 Generated Text:');
            console.log(textResult.result.text);
        }
        
        // Step 3: Cleanup
        console.log('\n🧹 Step 3: Cleanup');
        await client.sendRequest('rkllm.destroy', {});
        console.log('✅ RKLLM destroyed');
        
        console.log('\n🎉 Basic test completed - parameters are being sent correctly!');
        console.log('Next step: Debug RKNN vision encoder separately');
        
    } catch (error) {
        console.error('\n❌ Test failed:', error);
    } finally {
        client.disconnect();
    }
}

if (require.main === module) {
    testImageDescription().catch(console.error);
}