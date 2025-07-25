const net = require('net');

class TextOnlyTester {
    constructor() {
        this.socket = null;
        this.requestId = 1;
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

    sendRequest(method, params, timeout = 30000) {
        return new Promise((resolve, reject) => {
            const id = this.requestId++;
            const request = { jsonrpc: "2.0", method, params, id };
            
            console.log(`📤 ${method.toUpperCase()}`);
            
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
                        // Ignore partial JSON
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

    async testTextGeneration() {
        console.log('🔧 Initialize RKLLM for text generation');
        const result = await this.sendRequest('rkllm.init', {
            model_path: '/home/x/Projects/nano/models/qwen2vl2b/Qwen2-VL-2B-Instruct.rkllm',
            param: {
                max_context_len: 1024,
                max_new_tokens: 200,
                temperature: 0.8,
                top_k: 1,
                top_p: 0.9
            }
        });
        
        if (result.error) {
            throw new Error(`Init failed: ${result.error.message}`);
        }
        console.log('✅ RKLLM initialized');
        
        // Test text generation
        console.log('\n💬 Testing text generation');
        const textResult = await this.sendRequest('rkllm.run', {
            input_type: 'RKLLM_INPUT_PROMPT',
            role: 'user',
            prompt: 'Describe what an astronaut on the moon might see.'
        }, 60000);
        
        if (textResult.error) {
            throw new Error(`Text generation failed: ${textResult.error.message}`);
        }
        
        console.log('📝 Generated text:', textResult.result.text || JSON.stringify(textResult.result));
        
        // Test multimodal with mock embeddings
        console.log('\n🖼️ Testing multimodal with mock embeddings');
        
        // Create mock image embeddings (196 * 1536 = 301056 floats)
        const embeddings = new Float32Array(301056);
        for (let i = 0; i < embeddings.length; i++) {
            embeddings[i] = (Math.random() - 0.5) * 2;
        }
        
        const buffer = Buffer.from(embeddings.buffer);
        const embeddingsBase64 = buffer.toString('base64');
        console.log(`📊 Created ${embeddings.length} mock embeddings (${embeddingsBase64.length} base64 chars)`);
        
        const multimodalResult = await this.sendRequest('rkllm.run', {
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
        }, 90000);
        
        if (multimodalResult.error) {
            throw new Error(`Multimodal generation failed: ${multimodalResult.error.message}`);
        }
        
        console.log('🖼️ Multimodal response:', multimodalResult.result.text || JSON.stringify(multimodalResult.result));
        
        // Cleanup
        await this.sendRequest('rkllm.destroy', {});
        console.log('✅ Cleanup completed');
    }

    disconnect() {
        if (this.socket) {
            this.socket.end();
        }
    }
}

async function runTest() {
    console.log('🧪 TEXT AND MOCK MULTIMODAL TEST');
    console.log('='.repeat(50));
    
    const tester = new TextOnlyTester();
    
    try {
        await tester.connect();
        await tester.testTextGeneration();
        console.log('\n🎉 All tests completed successfully!');
    } catch (error) {
        console.error('\n❌ Test failed:', error.message);
        console.error('Stack:', error.stack);
    } finally {
        tester.disconnect();
    }
}

if (require.main === module) {
    runTest().catch(console.error);
}

module.exports = { TextOnlyTester, runTest };