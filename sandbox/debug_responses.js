const net = require('net');

class DebugClient {
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
                        console.log('📨 FULL RESPONSE STRUCTURE:');
                        console.log(JSON.stringify(response, null, 2));
                        console.log('📨 RESPONSE KEYS:', Object.keys(response));
                        if (response.result) {
                            console.log('📨 RESULT KEYS:', Object.keys(response.result));
                            console.log('📨 RESULT CONTENT:', response.result);
                        }
                        
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

    sendRequest(method, params) {
        return new Promise((resolve, reject) => {
            const id = this.requestId++;
            const request = {
                jsonrpc: "2.0",
                method,
                params,
                id
            };
            
            console.log('📤 SENDING REQUEST:');
            console.log(JSON.stringify(request, null, 2));
            
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

async function debugResponseFormats() {
    console.log('🔍 Debugging Response Formats');
    
    const client = new DebugClient();
    
    try {
        await client.connect();
        
        // Test RKLLM init
        console.log('\n=== Testing RKLLM Init ===');
        const initResult = await client.sendRequest('rkllm.init', {
            model_path: './models/qwen2vl2b/Qwen2-VL-2B-Instruct.rkllm',
            param: {
                num_npu_core: 1,
                max_context_len: 512,
                max_new_tokens: 128,
                temperature: 0.7
            }
        });
        
        if (initResult.error) {
            console.log('❌ RKLLM init failed:', initResult.error);
            return;
        }
        
        // Test simple text generation
        console.log('\n=== Testing Text Generation ===');
        const textResult = await client.sendRequest('rkllm.run', {
            input_type: 'RKLLM_INPUT_PROMPT',
            prompt: 'Hello, how are you?'
        });
        
        console.log('\n=== Final Analysis ===');
        console.log('Text result structure:', typeof textResult);
        console.log('Has result property:', 'result' in textResult);
        console.log('Has error property:', 'error' in textResult);
        
        // Cleanup
        await client.sendRequest('rkllm.destroy', {});
        
    } catch (error) {
        console.error('❌ Debug failed:', error);
    } finally {
        client.disconnect();
    }
}

if (require.main === module) {
    debugResponseFormats().catch(console.error);
}