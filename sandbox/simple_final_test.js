const net = require('net');

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
        
        // Initialize LLM only
        console.log('\n📤 rkllm.init');
        await client.request('rkllm.init', { 
            model_path: './models/qwen2vl2b/Qwen2-VL-2B-Instruct.rkllm' 
        });
        console.log('✅ RKLLM initialized');
        
        // Test basic text inference
        console.log('\n💬 Testing basic text inference...\n');
        console.log('🤖 Response: ');
        
        const responses = await client.request('rkllm.run', {
            input_type: 'RKLLM_INPUT_TEXT',
            role: 'user', 
            prompt: 'Hello! Please tell me a short joke.'
        });
        
        // Print streamed text
        const fullText = responses
            .filter(r => r.result && r.result.text)
            .map(r => r.result.text)
            .join('');
            
        console.log('\n\n📝 Full response:', fullText || '(no response)');
        
        // Cleanup
        await client.request('rkllm.destroy', {});
        
    } catch (error) {
        console.error('❌ Error:', error);
    } finally {
        client.close();
    }
}

main();