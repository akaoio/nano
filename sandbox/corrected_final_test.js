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
        
        // 1. Initialize vision encoder (separate RKNN model)
        console.log('\n📤 rknn.init');
        await client.request('rknn.init', { 
            model_path: '/home/x/Projects/nano/models/vision_encoder/vision_encoder.rknn' 
        });
        console.log('✅ RKNN initialized');
        
        // 2. Initialize LLM
        console.log('\n📤 rkllm.init');
        await client.request('rkllm.init', { 
            model_path: '/home/x/Projects/nano/models/qwen2vl2b/Qwen2-VL-2B-Instruct.rkllm' 
        });
        console.log('✅ RKLLM initialized');
        
        // 3. Load and process image
        const imageData = fs.readFileSync('/home/x/Projects/nano/tests/images/demo.jpg');
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
        
        if (!outputResp || outputResp.length === 0 || !outputResp[0].result) {
            console.error('❌ Invalid response from rknn.outputs_get:', outputResp);
            return;
        }
        
        const embedData = outputResp[0].result.outputs[0];
        console.log(`✅ Got ${embedData.num_floats} embeddings`);
        
        // 5. Decode embeddings from base64
        let embeddings;
        if (embedData.data_base64) {
            const buf = Buffer.from(embedData.data_base64, 'base64');
            embeddings = buf.toString('base64'); // Keep as base64 for RKLLM
        }
        
        // 6. Run multimodal inference
        console.log('\n💬 Asking about the image...\n');
        console.log('🤖 Response: ');
        
        const responses = await client.request('rkllm.run', {
            input_type: 'RKLLM_INPUT_MULTIMODAL',
            role: 'user', 
            prompt: '<image>What do you see in this image?',
            multimodal: {
                image_embed_base64: embeddings,
                n_image_tokens: 196,
                n_image: 1,
                image_height: 392,
                image_width: 392
            }
        });
        
        // Print streamed text
        const fullText = responses
            .filter(r => r.result && r.result.text)
            .map(r => r.result.text)
            .join('');
            
        console.log('\n\n📝 Full response:', fullText || '(no response)');
        
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