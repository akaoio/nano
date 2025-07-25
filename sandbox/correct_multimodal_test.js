const net = require('net');
const fs = require('fs');
const path = require('path');

class Qwen2VLMultimodalClient {
    constructor() {
        this.socket = null;
        this.requestId = 1;
        this.pendingRequests = new Map();
        this.rknnInitialized = false;
        this.rkllmInitialized = false;
    }

    connect() {
        return new Promise((resolve, reject) => {
            this.socket = net.createConnection('/tmp/rkllm.sock', () => {
                console.log('✅ Connected to RKLLM+RKNN server');
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

    sendRequest(method, params, showLogs = false) {
        return new Promise((resolve, reject) => {
            const id = this.requestId++;
            const request = {
                jsonrpc: "2.0",
                method,
                params,
                id
            };
            
            if (showLogs) {
                console.log('📤 REQUEST:', JSON.stringify(request, null, 2));
            }
            
            this.pendingRequests.set(id, { resolve, reject });
            this.socket.write(JSON.stringify(request) + '\n');
            
            setTimeout(() => {
                if (this.pendingRequests.has(id)) {
                    this.pendingRequests.delete(id);
                    reject(new Error(`Request timeout for method: ${method}`));
                }
            }, 60000);
        });
    }

    async initializeRKNN() {
        console.log('\n🔧 Initializing RKNN Vision Encoder...');
        const result = await this.sendRequest('rknn.init', {
            model_path: './models/qwen2vl2b/Qwen2-VL-2B-Instruct.rknn',
            flags: 0
        });
        
        if (result.error) {
            throw new Error(`RKNN init failed: ${result.error.message}`);
        }
        
        this.rknnInitialized = true;
        console.log('✅ RKNN Vision Encoder initialized');
        return result;
    }

    async initializeRKLLM() {
        console.log('\n🔧 Initializing RKLLM Language Model...');
        const result = await this.sendRequest('rkllm.init', {
            model_path: './models/qwen2vl2b/Qwen2-VL-2B-Instruct.rkllm',
            param: {
                num_npu_core: 3,  // Triple core for best performance as recommended
                max_context_len: 2048,
                max_new_tokens: 512,
                temperature: 0.7,
                top_p: 0.9,
                top_k: 1,
                skip_special_token: true,
                img_start: "<|vision_start|>",
                img_end: "<|vision_end|>",
                img_content: "<|image_pad|>",
                extend_param: {
                    base_domain_id: 1
                }
            }
        });
        
        if (result.error) {
            throw new Error(`RKLLM init failed: ${result.error.message}`);
        }
        
        this.rkllmInitialized = true;
        console.log('✅ RKLLM Language Model initialized');
        return result;
    }

    async setChatTemplate() {
        console.log('\n📝 Setting chat template...');
        const result = await this.sendRequest('rkllm.set_chat_template', {
            system_template: "<|im_start|>system\\nYou are a helpful assistant.<|im_end|>\\n",
            user_template: "<|im_start|>user\\n",
            assistant_template: "<|im_end|>\\n<|im_start|>assistant\\n"
        });
        
        if (result.error) {
            console.log('⚠️ Chat template setting failed, continuing anyway');
        } else {
            console.log('✅ Chat template set');
        }
        return result;
    }

    async processImageWithRKNN(imagePath) {
        console.log(`\n🖼️ Processing image: ${imagePath}`);
        
        if (!fs.existsSync(imagePath)) {
            throw new Error(`Image file not found: ${imagePath}`);
        }
        
        const imageBuffer = fs.readFileSync(imagePath);
        const imageBase64 = imageBuffer.toString('base64');
        
        console.log(`📊 Image size: ${imageBuffer.length} bytes`);
        
        // Step 1: Set inputs for RKNN
        console.log('🔄 Setting RKNN inputs...');
        const inputsResult = await this.sendRequest('rknn.inputs_set', {
            inputs: [{
                index: 0,
                type: 'RKNN_TENSOR_UINT8',
                size: imageBuffer.length,
                fmt: 'RKNN_TENSOR_NHWC',
                data: imageBase64
            }]
        });
        
        if (inputsResult.error) {
            throw new Error(`RKNN inputs_set failed: ${inputsResult.error.message}`);
        }
        console.log('✅ RKNN inputs set');

        // Step 2: Run RKNN inference
        console.log('🔄 Running RKNN vision inference...');
        const runResult = await this.sendRequest('rknn.run', {});
        
        if (runResult.error) {
            throw new Error(`RKNN run failed: ${runResult.error.message}`);
        }
        console.log('✅ RKNN vision inference completed');

        // Step 3: Get outputs (image embeddings)
        console.log('🔄 Getting image embeddings...');
        const outputsResult = await this.sendRequest('rknn.outputs_get', {
            num_outputs: 1,
            outputs: [{
                want_float: true
            }]
        });
        
        if (outputsResult.error) {
            throw new Error(`RKNN outputs_get failed: ${outputsResult.error.message}`);
        }
        
        console.log('✅ Image embeddings extracted');
        return outputsResult.result.outputs[0].data;
    }

    async generateTextWithRKLLM(prompt, imageEmbeddings = null) {
        console.log(`\n💬 Generating response for: "${prompt}"`);
        
        let request_params;
        
        if (imageEmbeddings && prompt.includes('<image>')) {
            // Multimodal input
            console.log('🖼️ Using multimodal input (text + image)');
            request_params = {
                input_type: 'RKLLM_INPUT_MULTIMODAL',
                role: 'user',
                multimodal: {
                    prompt: prompt,
                    image_embed: imageEmbeddings,
                    n_image_tokens: 196,  // For Qwen2-VL-2B
                    n_image: 1,
                    image_height: 392,
                    image_width: 392
                }
            };
        } else {
            // Text-only input
            console.log('📝 Using text-only input');
            request_params = {
                input_type: 'RKLLM_INPUT_PROMPT',
                role: 'user',
                prompt: prompt
            };
        }
        
        const result = await this.sendRequest('rkllm.run', request_params);
        
        if (result.error) {
            throw new Error(`RKLLM run failed: ${result.error.message}`);
        }
        
        return result.result;
    }

    async cleanup() {
        console.log('\n🧹 Cleaning up resources...');
        
        if (this.rknnInitialized) {
            await this.sendRequest('rknn.destroy', {});
            console.log('✅ RKNN destroyed');
        }
        
        if (this.rkllmInitialized) {
            await this.sendRequest('rkllm.destroy', {});
            console.log('✅ RKLLM destroyed');
        }
    }

    disconnect() {
        if (this.socket) {
            this.socket.end();
        }
    }
}

async function runComprehensiveMultimodalTest() {
    console.log('🚀 Starting Comprehensive Qwen2-VL Multimodal Test');
    console.log('=' * 60);
    
    const client = new Qwen2VLMultimodalClient();
    
    try {
        // Connect to server
        await client.connect();
        
        // Initialize models
        await client.initializeRKNN();
        await client.initializeRKLLM();
        await client.setChatTemplate();
        
        // Test images
        const testImages = [
            './tests/images/image1.jpg',
            './tests/images/image2.png'
        ];
        
        for (const imagePath of testImages) {
            console.log(`\\n${'=' * 50}`);
            console.log(`🎯 Testing with: ${path.basename(imagePath)}`);
            console.log('=' * 50);
            
            try {
                // Process image
                const imageEmbeddings = await client.processImageWithRKNN(imagePath);
                
                // Test multimodal questions
                const questions = [
                    '<image>What do you see in this image?',
                    '<image>Describe this image in detail.',
                    '<image>What colors are prominent in this image?',
                    '<image>What is the main subject of this image?'
                ];
                
                for (const question of questions) {
                    console.log(`\\n🤔 Question: ${question}`);
                    console.log('─' * 40);
                    
                    const response = await client.generateTextWithRKLLM(question, imageEmbeddings);
                    console.log(`🤖 Answer: ${response.text || response}`);
                }
                
            } catch (imageError) {
                console.error(`❌ Failed to process ${imagePath}:`, imageError.message);
            }
        }
        
        // Test pure text generation
        console.log(`\\n${'=' * 50}`);
        console.log('📝 Testing Pure Text Generation');
        console.log('=' * 50);
        
        const textQuestions = [
            'What is artificial intelligence?',
            'Explain the concept of machine learning.',
            'What are the benefits of using NPU processors?'
        ];
        
        for (const question of textQuestions) {
            console.log(`\\n🤔 Question: ${question}`);
            console.log('─' * 40);
            
            const response = await client.generateTextWithRKLLM(question);
            console.log(`🤖 Answer: ${response.text || response}`);
        }
        
        console.log(`\\n${'🎉' * 20}`);
        console.log('✅ ALL TESTS COMPLETED SUCCESSFULLY!');
        console.log('🎉' * 20);
        
    } catch (error) {
        console.error('\\n❌ TEST FAILED:', error);
        console.error('Stack trace:', error.stack);
        throw error;
    } finally {
        await client.cleanup();
        client.disconnect();
    }
}

if (require.main === module) {
    runComprehensiveMultimodalTest().catch(error => {
        console.error('Final error:', error);
        process.exit(1);
    });
}

module.exports = { Qwen2VLMultimodalClient, runComprehensiveMultimodalTest };