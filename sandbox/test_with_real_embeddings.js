const net = require('net');
const fs = require('fs');
const path = require('path');

class Qwen2VLTester {
    constructor() {
        this.socket = null;
        this.requestId = 1;
        this.responses = new Map();
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
                        if (response.id && this.responses.has(response.id)) {
                            this.responses.get(response.id).resolve(response);
                            this.responses.delete(response.id);
                        }
                    } catch (e) {
                        // Ignore parse errors
                    }
                }
            });
        });
    }

    async sendRequest(method, params) {
        return new Promise((resolve, reject) => {
            const id = this.requestId++;
            const request = { jsonrpc: "2.0", method, params, id };
            
            console.log(`\n📤 ${method}:`);
            // Don't log huge data
            if (method === 'rknn.outputs_get' && params.extend?.return_full_data) {
                console.log('  [Requesting full embedding data...]');
            } else if (method === 'rkllm.run' && params.multimodal?.image_embed_base64) {
                const logParams = { ...params };
                logParams.multimodal = { ...params.multimodal };
                logParams.multimodal.image_embed_base64 = `[${params.multimodal.image_embed_base64.length} chars]`;
                console.log(JSON.stringify(logParams, null, 2));
            } else {
                console.log(JSON.stringify(params, null, 2));
            }
            
            this.responses.set(id, { resolve, reject });
            this.socket.write(JSON.stringify(request) + '\n');
            
            // Set timeout
            setTimeout(() => {
                if (this.responses.has(id)) {
                    this.responses.get(id).reject(new Error('Request timeout'));
                    this.responses.delete(id);
                }
            }, 30000);
        });
    }

    async test() {
        console.log('🎯 Testing Qwen2-VL-2B Multimodal with Real Embeddings');
        console.log('='.repeat(60));
        
        // Initialize RKNN
        await this.sendRequest('rknn.init', {
            model_path: '/home/x/Projects/nano/models/qwen2vl2b/Qwen2-VL-2B-Instruct.rknn',
            flags: 0
        });
        console.log('✅ RKNN initialized');
        
        // Initialize RKLLM with multimodal settings
        await this.sendRequest('rkllm.init', {
            model_path: '/home/x/Projects/nano/models/qwen2vl2b/Qwen2-VL-2B-Instruct.rkllm',
            param: {
                max_context_len: 2048,
                max_new_tokens: 512,
                temperature: 0.7,
                top_k: 1,
                top_p: 0.9,
                skip_special_token: true,
                img_start: '<|vision_start|>',
                img_end: '<|vision_end|>',
                img_content: '<|image_pad|>',
                extend_param: {
                    base_domain_id: 1  // Critical for multimodal!
                }
            }
        });
        console.log('✅ RKLLM initialized with base_domain_id=1');
        
        // Set chat template
        await this.sendRequest('rkllm.set_chat_template', {
            system_template: "<|im_start|>system\\nYou are a helpful assistant.<|im_end|>\\n",
            user_template: "<|im_start|>user\\n",
            assistant_template: "<|im_end|>\\n<|im_start|>assistant\\n"
        });
        console.log('✅ Chat template set');
        
        // Process demo.jpg image
        const imagePath = '/home/x/Projects/nano/tests/images/demo.jpg';
        const imageBuffer = fs.readFileSync(imagePath);
        const imageBase64 = imageBuffer.toString('base64');
        
        console.log(`\n🖼️ Processing ${path.basename(imagePath)}: ${imageBuffer.length} bytes`);
        
        // Set image as input
        await this.sendRequest('rknn.inputs_set', {
            inputs: [{
                index: 0,
                type: 'RKNN_TENSOR_UINT8',
                size: imageBuffer.length,
                fmt: 'RKNN_TENSOR_NHWC',
                data: imageBase64
            }]
        });
        console.log('✅ Image set as RKNN input');
        
        // Run vision encoder
        await this.sendRequest('rknn.run', {});
        console.log('✅ Vision encoding completed');
        
        // Get embeddings with full data
        const outputsResult = await this.sendRequest('rknn.outputs_get', {
            num_outputs: 1,
            outputs: [{
                want_float: true
            }],
            extend: {
                return_full_data: true
            }
        });
        
        if (outputsResult.error) {
            throw new Error('Failed to get embeddings: ' + outputsResult.error.message);
        }
        
        const output = outputsResult.result.outputs[0];
        console.log(`✅ Got embeddings: ${output.num_floats} floats`);
        
        // Decode base64 embeddings
        let embeddings;
        if (output.data_base64) {
            const binaryData = Buffer.from(output.data_base64, 'base64');
            embeddings = new Float32Array(binaryData.buffer, binaryData.byteOffset, binaryData.length / 4);
            console.log(`✅ Decoded ${embeddings.length} embeddings from base64`);
        } else {
            throw new Error('No base64 embedding data received');
        }
        
        // Convert embeddings back to base64 for RKLLM
        const embeddingsBuffer = Buffer.from(embeddings.buffer);
        const embeddingsBase64 = embeddingsBuffer.toString('base64');
        
        // Test multimodal inference
        console.log('\n💬 Testing multimodal inference...');
        const result = await this.sendRequest('rkllm.run', {
            input_type: 'RKLLM_INPUT_MULTIMODAL',
            role: 'user',
            prompt: '<image>What do you see in this image?',
            multimodal: {
                image_embed_base64: embeddingsBase64,
                n_image_tokens: 196,
                n_image: 1,
                image_height: 392,
                image_width: 392
            }
        });
        
        console.log('\n📝 Result:', result.result ? 'SUCCESS' : 'FAILED');
        if (result.error) {
            console.log('❌ Error:', result.error);
        }
        if (result.result) {
            console.log('🤖 Response:', result.result);
        }
        
        // Cleanup
        await this.sendRequest('rknn.destroy', {});
        await this.sendRequest('rkllm.destroy', {});
        console.log('\n✅ Cleanup completed');
        
        this.socket.end();
    }
}

const tester = new Qwen2VLTester();
tester.connect().then(() => tester.test()).catch(console.error);