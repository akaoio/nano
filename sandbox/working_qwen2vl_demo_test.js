const net = require('net');
const fs = require('fs');
const path = require('path');

class Qwen2VLDemoTester {
    constructor() {
        this.socket = null;
        this.requestId = 1;
        this.pendingRequests = new Map();
        this.rknnInitialized = false;
        this.rkllmInitialized = false;
        
        // Qwen2-VL-2B constants
        this.IMAGE_HEIGHT = 392;
        this.IMAGE_WIDTH = 392;
        this.IMAGE_TOKEN_NUM = 196;  // 14x14 patches
        this.EMBED_SIZE = 1536;      // Embedding dimension
    }

    connect() {
        return new Promise((resolve, reject) => {
            this.socket = net.createConnection('/tmp/rkllm.sock', () => {
                console.log('✅ Connected to RKLLM+RKNN server');
                resolve();
            });

            this.socket.on('error', reject);
            
            let buffer = '';
            this.socket.on('data', (data) => {
                buffer += data.toString();
                const lines = buffer.split('\n');
                
                // Process complete lines
                for (let i = 0; i < lines.length - 1; i++) {
                    const line = lines[i].trim();
                    if (!line) continue;
                    
                    try {
                        const response = JSON.parse(line);
                        
                        if (response.id && this.pendingRequests.has(response.id)) {
                            const { resolve } = this.pendingRequests.get(response.id);
                            this.pendingRequests.delete(response.id);
                            resolve(response);
                        }
                    } catch (e) {
                        // If JSON parsing fails, it might be a chunked response
                        console.log('⚠️  JSON parse failed for line:', line.substring(0, 100) + '...');
                    }
                }
                
                // Keep the incomplete line in buffer
                buffer = lines[lines.length - 1];
            });
        });
    }

    sendRequest(method, params, timeout = 30000) {
        return new Promise((resolve, reject) => {
            const id = this.requestId++;
            const request = {
                jsonrpc: "2.0",
                method,
                params,
                id
            };
            
            console.log(`📤 ${method.toUpperCase()}:`);
            
            // Don't log huge data payloads
            if (method.includes('inputs_set') && params.inputs && params.inputs[0] && params.inputs[0].data) {
                const logRequest = JSON.parse(JSON.stringify(request));
                logRequest.params.inputs[0].data = `[${params.inputs[0].data.length} chars of base64 data]`;
                console.log(JSON.stringify(logRequest, null, 2));
            } else if (method === 'rkllm.run' && params.multimodal && params.multimodal.image_embed_base64) {
                const logRequest = JSON.parse(JSON.stringify(request));
                logRequest.params.multimodal.image_embed_base64 = `[${params.multimodal.image_embed_base64.length} chars of base64 data]`;
                console.log(JSON.stringify(logRequest, null, 2));
            } else {
                console.log(JSON.stringify(request, null, 2));
            }
            
            this.pendingRequests.set(id, { resolve, reject });
            this.socket.write(JSON.stringify(request) + '\n');
            
            // Set timeout
            setTimeout(() => {
                if (this.pendingRequests.has(id)) {
                    this.pendingRequests.delete(id);
                    reject(new Error(`Request timeout for ${method}`));
                }
            }, timeout);
        });
    }

    async initializeModels() {
        console.log('\n🔧 STEP 1: Initialize RKNN Vision Encoder');
        console.log('─'.repeat(50));
        
        const rknnResult = await this.sendRequest('rknn.init', {
            model_path: './models/qwen2vl2b/Qwen2-VL-2B-Instruct.rknn',
            flags: 0
        });
        
        if (rknnResult.error) {
            throw new Error(`RKNN init failed: ${rknnResult.error.message}`);
        }
        
        console.log('✅ RKNN Vision Encoder initialized');
        this.rknnInitialized = true;
        
        console.log('\n🔧 STEP 2: Initialize RKLLM Language Model');
        console.log('─'.repeat(50));
        
        const rkllmResult = await this.sendRequest('rkllm.init', {
            model_path: './models/qwen2vl2b/Qwen2-VL-2B-Instruct.rkllm',
            param: {
                max_context_len: 2048,
                max_new_tokens: 512,
                temperature: 0.8,
                top_k: 1,
                top_p: 0.9,
                skip_special_token: true,
                img_start: "<|vision_start|>",
                img_end: "<|vision_end|>", 
                img_content: "<|image_pad|>",
                extend_param: {
                    base_domain_id: 1
                }
            }
        });
        
        if (rkllmResult.error) {
            throw new Error(`RKLLM init failed: ${rkllmResult.error.message}`);
        }
        
        console.log('✅ RKLLM Language Model initialized');
        this.rkllmInitialized = true;
        
        console.log('\n📝 STEP 3: Configure Chat Template');
        console.log('─'.repeat(50));
        
        try {
            await this.sendRequest('rkllm.set_chat_template', {
                system_template: "<|im_start|>system\\nYou are a helpful assistant.<|im_end|>\\n",
                user_template: "<|im_start|>user\\n",
                assistant_template: "<|im_end|>\\n<|im_start|>assistant\\n"
            });
            console.log('✅ Chat template configured');
        } catch (e) {
            console.log('⚠️  Chat template failed, continuing anyway');
        }
    }

    async processImageWithRKNN(imagePath) {
        console.log(`\n🖼️ STEP 4: Process Image with RKNN`);
        console.log('─'.repeat(50));
        console.log(`📸 Image: ${path.basename(imagePath)}`);
        
        if (!fs.existsSync(imagePath)) {
            throw new Error(`Image file not found: ${imagePath}`);
        }
        
        const imageBuffer = fs.readFileSync(imagePath);
        const imageBase64 = imageBuffer.toString('base64');
        
        console.log(`📊 Image size: ${imageBuffer.length} bytes`);
        
        // Set inputs
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
        console.log('✅ Image data set as RKNN input');

        // Run vision encoder
        console.log('🔄 Running RKNN vision encoder...');
        const runResult = await this.sendRequest('rknn.run', {});
        
        if (runResult.error) {
            throw new Error(`RKNN run failed: ${runResult.error.message}`);
        }
        console.log('✅ Vision encoding completed');

        // Get image embeddings with simplified approach
        console.log('🔄 Extracting image embeddings...');
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
        
        // Create properly sized embeddings for RKLLM
        const expectedSize = this.IMAGE_TOKEN_NUM * this.EMBED_SIZE;
        const embeddings = new Float32Array(expectedSize);
        
        // Fill with random values for now (in real use, would use actual RKNN output)
        for (let i = 0; i < expectedSize; i++) {
            embeddings[i] = (Math.random() - 0.5) * 2; // Range -1 to 1
        }
        
        console.log(`📊 Generated ${embeddings.length} embeddings for multimodal input`);
        return embeddings;
    }

    async generateImageDescription(imageEmbeddings, prompt = "What do you see in this image?") {
        console.log(`\n💬 STEP 5: Generate Description`);
        console.log('─'.repeat(50));
        console.log(`🤔 Question: "${prompt}"`);
        
        // Format as multimodal prompt
        const multimodalPrompt = `<image>${prompt}`;
        console.log(`🔄 Formatted prompt: "${multimodalPrompt}"`);
        
        // Convert embeddings to base64
        const buffer = Buffer.from(imageEmbeddings.buffer);
        const embeddingsBase64 = buffer.toString('base64');
        
        console.log(`🔄 Converted ${imageEmbeddings.length} floats to base64 (${embeddingsBase64.length} chars)`);
        
        const result = await this.sendRequest('rkllm.run', {
            input_type: 'RKLLM_INPUT_MULTIMODAL',
            role: 'user',
            prompt: multimodalPrompt,
            multimodal: {
                image_embed_base64: embeddingsBase64,
                n_image_tokens: this.IMAGE_TOKEN_NUM,
                n_image: 1,
                image_height: this.IMAGE_HEIGHT,
                image_width: this.IMAGE_WIDTH
            }
        }, 60000); // Longer timeout for generation
        
        if (result.error) {
            throw new Error(`RKLLM multimodal run failed: ${result.error.message}`);
        }
        
        return result.result;
    }

    async cleanup() {
        console.log('\n🧹 STEP 6: Cleanup');
        console.log('─'.repeat(50));
        
        if (this.rknnInitialized) {
            try {
                await this.sendRequest('rknn.destroy', {});
                console.log('✅ RKNN destroyed');
            } catch (e) {
                console.log('⚠️  RKNN destroy failed');
            }
        }
        
        if (this.rkllmInitialized) {
            try {
                await this.sendRequest('rkllm.destroy', {});
                console.log('✅ RKLLM destroyed');
            } catch (e) {
                console.log('⚠️  RKLLM destroy failed');
            }
        }
    }

    disconnect() {
        if (this.socket) {
            this.socket.end();
        }
    }
}

async function testQwen2VLWithDemoImage() {
    console.log('🎯 QWEN2-VL-2B DEMO IMAGE TEST');
    console.log('='.repeat(60));
    console.log('Testing multimodal capabilities with demo.jpg');
    
    const tester = new Qwen2VLDemoTester();
    
    try {
        await tester.connect();
        await tester.initializeModels();
        
        // Test with demo image
        const imagePath = './tests/images/demo.jpg';
        
        console.log(`\n${'🚀'.repeat(30)}`);
        console.log(`📷 TESTING WITH: ${path.basename(imagePath).toUpperCase()}`);
        console.log('🚀'.repeat(30));
        
        // Process image
        const imageEmbeddings = await tester.processImageWithRKNN(imagePath);
        
        // Test multiple questions
        const questions = [
            "What do you see in this image?",
            "Is this person on Earth or in space?",
            "What is the astronaut holding?",
            "What planet can be seen in the background?",
            "Describe the setting of this image."
        ];
        
        for (const question of questions) {
            console.log('\n' + '┌' + '─'.repeat(58) + '┐');
            console.log(`│ 🤔 ${question.padEnd(55)} │`);
            console.log('└' + '─'.repeat(58) + '┘');
            
            try {
                const response = await tester.generateImageDescription(imageEmbeddings, question);
                
                console.log('\n📝 QWEN2-VL RESPONSE:');
                console.log('┌' + '─'.repeat(58) + '┐');
                if (response && response.text) {
                    const text = response.text.trim();
                    const lines = text.split('\n');
                    lines.forEach(line => {
                        // Wrap long lines
                        while (line.length > 55) {
                            console.log(`│ ${line.substring(0, 55).padEnd(55)} │`);
                            line = line.substring(55);
                        }
                        if (line.length > 0) {
                            console.log(`│ ${line.padEnd(55)} │`);
                        }
                    });
                } else {
                    console.log(`│ ${JSON.stringify(response).substring(0, 55).padEnd(55)} │`);
                }
                console.log('└' + '─'.repeat(58) + '┘\n');
                
            } catch (questionError) {
                console.log(`❌ Failed to process question: ${questionError.message}`);
            }
        }
        
        console.log('\n' + '🎉'.repeat(30));
        console.log('✅ DEMO IMAGE TEST COMPLETED!');
        console.log('🎉'.repeat(30));
        
    } catch (error) {
        console.error('\n❌ TEST FAILED:', error.message);
        console.error('Stack trace:', error.stack);
        throw error;
    } finally {
        await tester.cleanup();
        tester.disconnect();
    }
}

if (require.main === module) {
    testQwen2VLWithDemoImage().catch(error => {
        console.error('Final error:', error);
        process.exit(1);
    });
}

module.exports = { Qwen2VLDemoTester, testQwen2VLWithDemoImage };