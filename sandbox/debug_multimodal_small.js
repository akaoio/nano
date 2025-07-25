const net = require('net');

class SimpleMultimodalTest {
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
                        console.error('❌ Parse error:', e.message);
                    }
                }
            });
        });
    }

    sendRequest(method, params) {
        return new Promise((resolve, reject) => {
            const id = this.requestId++;
            const request = { jsonrpc: "2.0", method, params, id };
            
            console.log(`📤 ${method}:`);
            console.log(JSON.stringify(params, null, 2));
            
            this.pendingRequests.set(id, { resolve, reject });
            this.socket.write(JSON.stringify(request) + '\n');
        });
    }

    async test() {
        console.log('🧪 Testing RKLLM multimodal with small embedding sample');
        
        // Initialize RKLLM
        await this.sendRequest('rkllm.init', {
            model_path: './models/qwen2vl2b/Qwen2-VL-2B-Instruct.rkllm',
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
                extend_param: { base_domain_id: 1 }
            }
        });
        
        console.log('✅ RKLLM initialized');
        
        // Create small embedding sample (first 100 floats repeated to reach 301056 total)
        const smallSample = [];
        for (let i = 0; i < 100; i++) {
            smallSample.push(Math.random() * 2 - 1); // Random floats between -1 and 1
        }
        
        // Repeat to get exactly 301056 floats (196 * 1536)
        const fullEmbeddings = [];
        const targetSize = 196 * 1536; // 301056
        for (let i = 0; i < targetSize; i++) {
            fullEmbeddings.push(smallSample[i % smallSample.length]);
        }
        
        console.log(`📊 Created ${fullEmbeddings.length} embedding values`);
        
        // Test multimodal run
        const result = await this.sendRequest('rkllm.run', {
            input_type: 'RKLLM_INPUT_MULTIMODAL',
            role: 'user',
            prompt: '<image>What do you see in this image?',
            multimodal: {
                image_embed: fullEmbeddings,
                n_image_tokens: 196,
                n_image: 1,
                image_height: 392,
                image_width: 392
            }
        });
        
        console.log('✅ RKLLM multimodal result:');
        console.log(JSON.stringify(result, null, 2));
        
        this.socket.end();
    }
}

const tester = new SimpleMultimodalTest();
tester.connect().then(() => tester.test()).catch(console.error);