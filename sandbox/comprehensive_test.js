const net = require('net');
const fs = require('fs');
const path = require('path');
const { spawn } = require('child_process');

class TestClient {
    constructor() {
        this.socket = null;
        this.requestId = 1;
        this.pendingRequests = new Map();
    }

    connect() {
        return new Promise((resolve, reject) => {
            this.socket = net.createConnection('/tmp/rkllm.sock', () => {
                console.log('Connected to server');
                resolve();
            });

            this.socket.on('error', reject);
            this.socket.on('data', (data) => {
                const lines = data.toString().split('\n').filter(line => line.trim());
                for (const line of lines) {
                    try {
                        const response = JSON.parse(line);
                        console.log('RAW RESPONSE JSON:', JSON.stringify(response, null, 2));
                        
                        if (response.id && this.pendingRequests.has(response.id)) {
                            const { resolve } = this.pendingRequests.get(response.id);
                            this.pendingRequests.delete(response.id);
                            resolve(response);
                        }
                    } catch (e) {
                        console.error('Failed to parse response:', line, e);
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
                params,
                id
            };
            
            console.log('RAW REQUEST JSON:', JSON.stringify(request, null, 2));
            
            this.pendingRequests.set(id, { resolve, reject });
            this.socket.write(JSON.stringify(request) + '\n');
            
            setTimeout(() => {
                if (this.pendingRequests.has(id)) {
                    this.pendingRequests.delete(id);
                    reject(new Error('Request timeout'));
                }
            }, 30000);
        });
    }

    disconnect() {
        if (this.socket) {
            this.socket.end();
        }
    }
}

async function testRKLLM() {
    console.log('\n=== TESTING RKLLM ===');
    const client = new TestClient();
    
    try {
        await client.connect();
        
        // Test rkllm.init
        console.log('\n--- Testing rkllm.init ---');
        const initResult = await client.sendRequest('rkllm.init', {
            model_path: './models/qwen2vl2b/Qwen2-VL-2B-Instruct.rkllm',
            param: {
                num_npu_core: 1,
                max_context_len: 512,
                max_new_tokens: 256,
                temperature: 0.7,
                top_p: 0.9,
                top_k: 40,
                frequency_penalty: 0.0,
                presence_penalty: 0.0,
                mirostat: 0,
                mirostat_tau: 5.0,
                mirostat_eta: 0.1
            }
        });
        console.log('RKLLM Init Result:', JSON.stringify(initResult, null, 2));
        
        // Test text generation
        console.log('\n--- Testing rkllm.run ---');
        const runResult = await client.sendRequest('rkllm.run', {
            prompt: 'What is artificial intelligence?',
            input_type: 'RKLLM_INPUT_PROMPT'
        });
        console.log('RKLLM Run Result:', JSON.stringify(runResult, null, 2));
        
        // Test with multimodal input
        console.log('\n--- Testing rkllm.run with image ---');
        const imageBuffer = fs.readFileSync('./tests/images/image1.jpg');
        const imageBase64 = imageBuffer.toString('base64');
        
        const multimodalResult = await client.sendRequest('rkllm.run', {
            prompt: 'Describe this image',
            input_type: 'RKLLM_INPUT_MULTIMODAL',
            multimodal: [{
                type: 'image',
                data: imageBase64
            }]
        });
        console.log('RKLLM Multimodal Result:', JSON.stringify(multimodalResult, null, 2));
        
        // Test cleanup
        console.log('\n--- Testing rkllm.destroy ---');
        const destroyResult = await client.sendRequest('rkllm.destroy', {});
        console.log('RKLLM Destroy Result:', JSON.stringify(destroyResult, null, 2));
        
    } catch (error) {
        console.error('RKLLM Test Error:', error);
        throw error;
    } finally {
        client.disconnect();
    }
}

async function testRKNN() {
    console.log('\n=== TESTING RKNN ===');
    const client = new TestClient();
    
    try {
        await client.connect();
        
        // Test rknn.init
        console.log('\n--- Testing rknn.init ---');
        const initResult = await client.sendRequest('rknn.init', {
            model_path: './models/qwen2vl2b/Qwen2-VL-2B-Instruct.rknn',
            flags: 0
        });
        console.log('RKNN Init Result:', JSON.stringify(initResult, null, 2));
        
        // Test query
        console.log('\n--- Testing rknn.query ---');
        const queryResult = await client.sendRequest('rknn.query', {
            cmd: 'RKNN_QUERY_SDK_VERSION'
        });
        console.log('RKNN Query Result:', JSON.stringify(queryResult, null, 2));
        
        // Test with image input
        console.log('\n--- Testing rknn.inputs_set with image ---');
        const imageBuffer = fs.readFileSync('./tests/images/image2.png');
        const imageBase64 = imageBuffer.toString('base64');
        
        const inputsSetResult = await client.sendRequest('rknn.inputs_set', {
            inputs: [{
                index: 0,
                type: 'RKNN_TENSOR_UINT8',
                size: imageBuffer.length,
                fmt: 'RKNN_TENSOR_NHWC',
                data: imageBase64
            }]
        });
        console.log('RKNN Inputs Set Result:', JSON.stringify(inputsSetResult, null, 2));
        
        // Test run
        console.log('\n--- Testing rknn.run ---');
        const runResult = await client.sendRequest('rknn.run', {
            context: 0
        });
        console.log('RKNN Run Result:', JSON.stringify(runResult, null, 2));
        
        // Test get outputs
        console.log('\n--- Testing rknn.outputs_get ---');
        const outputsResult = await client.sendRequest('rknn.outputs_get', {
            num_outputs: 1,
            outputs: [{
                want_float: true
            }]
        });
        console.log('RKNN Outputs Get Result:', JSON.stringify(outputsResult, null, 2));
        
        // Test cleanup
        console.log('\n--- Testing rknn.destroy ---');
        const destroyResult = await client.sendRequest('rknn.destroy', {});
        console.log('RKNN Destroy Result:', JSON.stringify(destroyResult, null, 2));
        
    } catch (error) {
        console.error('RKNN Test Error:', error);
        throw error;
    } finally {
        client.disconnect();
    }
}

async function runTests() {
    console.log('Starting comprehensive RKLLM+RKNN server tests...');
    
    try {
        console.log('\n🔄 Testing RKLLM functionality...');
        await testRKLLM();
        console.log('✅ RKLLM tests completed successfully');
        
        // Small delay between tests
        await new Promise(resolve => setTimeout(resolve, 2000));
        
        console.log('\n🔄 Testing RKNN functionality...');
        await testRKNN();
        console.log('✅ RKNN tests completed successfully');
        
        console.log('\n🎉 ALL TESTS PASSED!');
        
    } catch (error) {
        console.error('\n❌ TEST FAILED:', error);
        process.exit(1);
    }
}

if (require.main === module) {
    runTests();
}

module.exports = { TestClient, testRKLLM, testRKNN };