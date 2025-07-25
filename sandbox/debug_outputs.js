const net = require('net');
const fs = require('fs');

// Simple client that works with the actual implementation
class RKLLMClient {
    constructor() {
        this.socket = null;
        this.requestId = 1;
    }

    connect() {
        return new Promise((resolve, reject) => {
            this.socket = net.createConnection('/tmp/rkllm.sock', () => {
                console.log('✅ Connected');
                resolve();
            });
            this.socket.on('error', reject);
        });
    }

    async request(method, params) {
        return new Promise((resolve) => {
            const id = this.requestId++;
            const req = { jsonrpc: '2.0', method, params, id };
            
            console.log(`\n📤 ${method}`);
            this.socket.write(JSON.stringify(req) + '\n');
            
            const responses = [];
            let done = false;
            
            const handler = (data) => {
                const lines = data.toString().split('\n').filter(l => l);
                for (const line of lines) {
                    try {
                        const resp = JSON.parse(line);
                        if (resp.id === id) {
                            responses.push(resp);
                            
                            // Check if streaming is complete
                            if (resp.result && resp.result._callback_state === 2) {
                                done = true;
                                this.socket.off('data', handler);
                                resolve(responses);
                            }
                            // Or if it's a non-streaming response
                            else if (resp.result && !resp.result.hasOwnProperty('_callback_state')) {
                                done = true;
                                this.socket.off('data', handler);
                                resolve(responses);
                            }
                        }
                    } catch (e) {
                        // Ignore parsing errors
                    }
                }
            };
            
            this.socket.on('data', handler);
        });
    }
    
    close() {
        this.socket.end();
    }
}

async function main() {
    const client = new RKLLMClient();
    
    try {
        await client.connect();
        
        // 1. Initialize vision encoder
        console.log('\n📤 rknn.init');
        await client.request('rknn.init', { 
            model_path: './models/vision_encoder/vision_encoder.rknn' 
        });
        console.log('✅ RKNN initialized');
        
        // 2. Initialize LLM
        console.log('\n📤 rkllm.init');
        await client.request('rkllm.init', { 
            model_path: './models/qwen2vl2b/Qwen2-VL-2B-Instruct.rkllm' 
        });
        console.log('✅ RKLLM initialized');
        
        // 3. Load and process image
        const imageData = fs.readFileSync('./tests/images/demo.jpg');
        console.log('\n📤 rknn.inputs_set');
        await client.request('rknn.inputs_set', {
            n_inputs: 1,
            inputs: [{
                index: 0,
                data_format: 'RKNN_TENSOR_UINT8',
                data_base64: imageData.toString('base64'),
                size: imageData.length,
                n_dims: 4,
                dims: [1, 392, 392, 3]
            }]
        });
        console.log('✅ Image loaded');
        
        // Run vision encoder
        console.log('\n📤 rknn.run');
        await client.request('rknn.run', {});
        console.log('✅ Vision encoder complete');
        
        // 4. Get embeddings
        console.log('\n📤 rknn.outputs_get');
        const outputResp = await client.request('rknn.outputs_get', {
            n_outputs: 1,
            outputs: [{ want_float: true }],
            extend: { return_full_data: true }
        });
        
        console.log('DEBUG: Full output response:');
        console.log(JSON.stringify(outputResp, null, 2));
        
        // Cleanup
        await client.request('rknn.destroy', {});
        await client.request('rkllm.destroy', {});
        
    } catch (error) {
        console.error('❌ Error:', error);
    } finally {
        client.close();
    }
}

main();