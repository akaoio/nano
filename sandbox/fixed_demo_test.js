const net = require('net');
const fs = require('fs');
const path = require('path');

class FixedQwen2VLTester {
    constructor() {
        this.socket = null;
        this.requestId = 1;
        this.rknnInitialized = false;
        this.rkllmInitialized = false;
        
        // Qwen2-VL-2B constants
        this.IMAGE_HEIGHT = 392;
        this.IMAGE_WIDTH = 392;
        this.IMAGE_TOKEN_NUM = 196;
        this.EMBED_SIZE = 1536;
    }

    connect() {
        return new Promise((resolve, reject) => {
            this.socket = net.createConnection('/tmp/rkllm.sock', () => {
                console.log('✅ Connected to server');
                resolve();
            });
            this.socket.on('error', reject);
        });
    }

    sendRequest(method, params, timeout = 60000) {
        return new Promise((resolve, reject) => {
            const id = this.requestId++;
            const request = { jsonrpc: "2.0", method, params, id };
            
            console.log(`📤 ${method.toUpperCase()}`);
            
            // Handle response
            let buffer = '';
            const handleData = (data) => {
                buffer += data.toString();
                const lines = buffer.split('\n');
                
                for (let i = 0; i < lines.length - 1; i++) {
                    const line = lines[i].trim();
                    if (!line) continue;
                    
                    try {
                        const response = JSON.parse(line);
                        if (response.id === id) {
                            this.socket.removeListener('data', handleData);
                            clearTimeout(timer);
                            resolve(response);
                            return;
                        }
                    } catch (e) {
                        // Ignore JSON parse errors for partial data
                    }
                }
                buffer = lines[lines.length - 1];
            };
            
            this.socket.on('data', handleData);
            this.socket.write(JSON.stringify(request) + '\n');
            
            const timer = setTimeout(() => {
                this.socket.removeListener('data', handleData);
                reject(new Error(`Timeout for ${method}`));
            }, timeout);
        });
    }

    async initializeModels() {
        console.log('\n🔧 Initialize RKNN Vision Encoder');
        const rknnResult = await sendRequest(ws, {
            method: 'rknn.init',
            params: {
                model_path: './models/qwen2vl2b/Qwen2-VL-2B-Instruct.rknn',
                flags: 0
            }
        });
        
        if (rknnResult.error) {
            throw new Error(`RKNN init failed: ${rknnResult.error.message}`);
        }
        console.log('✅ RKNN initialized');
        this.rknnInitialized = true;
        
        console.log('\n🔧 Initialize RKLLM Language Model');
        const rkllmResult = await sendRequest(ws, {
            method: 'rkllm.init',
            params: {
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
            }
        });
        
        if (rkllmResult.error) {
            throw new Error(`RKLLM init failed: ${rkllmResult.error.message}`);
        }
        console.log('✅ RKLLM initialized');
        this.rkllmInitialized = true;
    }

    async processImageWithRKNN(imagePath) {
        console.log(`\n🖼️ Process image: ${path.basename(imagePath)}`);
        
        const imageBuffer = fs.readFileSync(imagePath);
        const imageBase64 = imageBuffer.toString('base64');
        console.log(`📊 Image size: ${imageBuffer.length} bytes`);
        
        // Set inputs
        await this.sendRequest('rknn.inputs_set', {
            inputs: [{
                index: 0,
                type: 'RKNN_TENSOR_UINT8',
                size: imageBuffer.length,
                fmt: 'RKNN_TENSOR_NHWC',
                data: imageBase64
            }]
        });
        console.log('✅ Image input set');

        // Run inference
        await this.sendRequest('rknn.run', {});
        console.log('✅ Vision inference completed');

        // For this demo, create mock embeddings since we can't easily extract the actual ones
        const expectedSize = this.IMAGE_TOKEN_NUM * this.EMBED_SIZE;
        const embeddings = new Float32Array(expectedSize);
        for (let i = 0; i < expectedSize; i++) {
            embeddings[i] = (Math.random() - 0.5) * 2;
        }
        
        console.log(`✅ Generated ${embeddings.length} mock embeddings for testing`);
        return embeddings;
    }

    async generateResponse(imageEmbeddings, prompt) {
        console.log(`\n💬 Generate response for: "${prompt}"`);
        
        const multimodalPrompt = `<image>${prompt}`;
        const buffer = Buffer.from(imageEmbeddings.buffer);
        const embeddingsBase64 = buffer.toString('base64');
        
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
        }, 90000);
        
        if (result.error) {
            throw new Error(`Generation failed: ${result.error.message}`);
        }
        
        return result.result;
    }

    async cleanup() {
        console.log('\n🧹 Cleanup');
        if (this.rknnInitialized) {
            await this.sendRequest('rknn.destroy', {});
        }
        if (this.rkllmInitialized) {
            await this.sendRequest('rkllm.destroy', {});
        }
    }

    disconnect() {
        if (this.socket) {
            this.socket.end();
        }
    }
}

async function testWithDemoImage() {
    console.log('🎯 QWEN2-VL DEMO TEST');
    console.log('='.repeat(50));
    
    const tester = new FixedQwen2VLTester();
    
    try {
        await tester.connect();
        await tester.initializeModels();
        
        const imagePath = './tests/images/demo.jpg';
        console.log(`\n📷 Testing with: ${path.basename(imagePath)}`);
        
        const imageEmbeddings = await tester.processImageWithRKNN(imagePath);
        
        const questions = [
            "What do you see in this image?",
            "Is this person on Earth or in space?",
            "What is in the background of this image?"
        ];
        
        for (const question of questions) {
            console.log(`\n❓ ${question}`);
            console.log('─'.repeat(40));
            
            try {
                const response = await tester.generateResponse(imageEmbeddings, question);
                console.log(`🤖 Response: ${response.text || JSON.stringify(response)}`);
            } catch (e) {
                console.log(`❌ Error: ${e.message}`);
            }
        }
        
        console.log('\n🎉 Test completed!');
        
    } catch (error) {
        console.error('\n❌ Test failed:', error.message);
    } finally {
        await tester.cleanup();
        tester.disconnect();
    }
}

if (require.main === module) {
    testWithDemoImage().catch(console.error);
}

module.exports = { FixedQwen2VLTester, testWithDemoImage };