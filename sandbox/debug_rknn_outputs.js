const net = require('net');
const fs = require('fs');
const path = require('path');

class RKNNOutputsDebugger {
    constructor() {
        this.socket = null;
        this.requestId = 1;
        this.pendingRequests = new Map();
    }

    connect() {
        return new Promise((resolve, reject) => {
            this.socket = net.createConnection('/tmp/rkllm.sock', resolve);
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
                        console.error('❌ Parse error:', e);
                    }
                }
            });
        });
    }

    sendRequest(method, params, timeout = 15000) {
        return new Promise((resolve, reject) => {
            const id = this.requestId++;
            const request = { jsonrpc: "2.0", method, params, id };
            
            this.pendingRequests.set(id, { resolve, reject });
            this.socket.write(JSON.stringify(request) + '\n');
            
            setTimeout(() => {
                if (this.pendingRequests.has(id)) {
                    this.pendingRequests.delete(id);
                    reject(new Error(`Timeout: ${method}`));
                }
            }, timeout);
        });
    }

    disconnect() { if (this.socket) this.socket.end(); }
}

async function debugRKNNOutputs() {
    console.log('🔍 DEBUGGING RKNN OUTPUTS_GET RESPONSE');
    
    const client = new RKNNOutputsDebugger();
    
    try {
        await client.connect();
        console.log('✅ Connected');
        
        // Initialize RKNN
        console.log('\n1. Initializing RKNN...');
        const initResult = await client.sendRequest('rknn.init', {
            model_path: './models/qwen2vl2b/Qwen2-VL-2B-Instruct.rknn',
            flags: 0
        });
        console.log('RKNN Init Result:', JSON.stringify(initResult, null, 2));
        
        if (initResult.error) {
            throw new Error('RKNN init failed');
        }
        
        // Set inputs with a small test image
        console.log('\n2. Setting inputs...');
        const imageBuffer = fs.readFileSync('./tests/images/image1.jpg');
        const imageBase64 = imageBuffer.toString('base64');
        
        const inputsResult = await client.sendRequest('rknn.inputs_set', {
            inputs: [{
                index: 0,
                type: 'RKNN_TENSOR_UINT8',
                size: imageBuffer.length,
                fmt: 'RKNN_TENSOR_NHWC',
                data: imageBase64
            }]
        });
        console.log('Inputs Set Result:', JSON.stringify(inputsResult, null, 2));
        
        if (inputsResult.error) {
            throw new Error('RKNN inputs_set failed');
        }
        
        // Run RKNN
        console.log('\n3. Running RKNN...');
        const runResult = await client.sendRequest('rknn.run', {});
        console.log('Run Result:', JSON.stringify(runResult, null, 2));
        
        if (runResult.error) {
            throw new Error('RKNN run failed');
        }
        
        // Get outputs - this is where the issue is
        console.log('\n4. Getting outputs (DEBUG FOCUS)...');
        const outputsResult = await client.sendRequest('rknn.outputs_get', {
            num_outputs: 1,
            outputs: [{
                want_float: true
            }]
        });
        
        console.log('\n📊 DETAILED OUTPUTS ANALYSIS:');
        console.log('Full response:', JSON.stringify(outputsResult, null, 2));
        console.log('Type of result:', typeof outputsResult.result);
        console.log('Result keys:', outputsResult.result ? Object.keys(outputsResult.result) : 'none');
        
        if (outputsResult.result && outputsResult.result.outputs) {
            console.log('Outputs array length:', outputsResult.result.outputs.length);
            console.log('First output keys:', Object.keys(outputsResult.result.outputs[0] || {}));
            if (outputsResult.result.outputs[0] && outputsResult.result.outputs[0].data) {
                console.log('Data type:', typeof outputsResult.result.outputs[0].data);
                console.log('Data length:', Array.isArray(outputsResult.result.outputs[0].data) ? outputsResult.result.outputs[0].data.length : 'not array');
            }
        }
        
        // Cleanup
        console.log('\n5. Cleanup...');
        await client.sendRequest('rknn.destroy', {});
        console.log('✅ Cleanup complete');
        
    } catch (error) {
        console.error('❌ Debug error:', error);
    } finally {
        client.disconnect();
    }
}

if (require.main === module) {
    debugRKNNOutputs().catch(console.error);
}