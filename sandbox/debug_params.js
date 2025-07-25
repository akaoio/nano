const net = require('net');

class ParamsDebugger {
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
                        console.log('📨 RAW SERVER RESPONSE:');
                        console.log(JSON.stringify(response, null, 2));
                        
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
            
            console.log('\n📤 SENDING TO SERVER:');
            console.log(JSON.stringify(request, null, 2));
            console.log(`📊 Params type: ${typeof params}`);
            console.log(`📊 Params keys: ${Object.keys(params || {})}`);
            
            this.pendingRequests.set(id, { resolve, reject });
            this.socket.write(JSON.stringify(request) + '\n');
            
            setTimeout(() => {
                if (this.pendingRequests.has(id)) {
                    this.pendingRequests.delete(id);
                    reject(new Error('Request timeout'));
                }
            }, 10000);
        });
    }

    disconnect() {
        if (this.socket) {
            this.socket.end();
        }
    }
}

async function debugParameterHandling() {
    console.log('🔍 DEBUGGING PARAMETER HANDLING');
    console.log('=' * 50);
    
    const client = new ParamsDebugger();
    
    try {
        await client.connect();
        
        // Test 1: Simple RKNN init
        console.log('\n🧪 TEST 1: RKNN Init with Object Parameters');
        console.log('-' * 40);
        
        const rknnResult = await client.sendRequest('rknn.init', {
            model_path: './models/qwen2vl2b/Qwen2-VL-2B-Instruct.rknn',
            flags: 0
        });
        
        // Test 2: RKLLM init with nested params
        console.log('\n🧪 TEST 2: RKLLM Init with Nested Parameters');
        console.log('-' * 40);
        
        const rkllmResult = await client.sendRequest('rkllm.init', {
            model_path: './models/qwen2vl2b/Qwen2-VL-2B-Instruct.rkllm',
            param: {
                max_context_len: 512,
                max_new_tokens: 128,
                temperature: 0.7
            }
        });
        
        // Test 3: Simple method with no complex params
        console.log('\n🧪 TEST 3: Simple Destroy Call');
        console.log('-' * 40);
        
        if (!rkllmResult.error) {
            const destroyResult = await client.sendRequest('rkllm.destroy', {});
        }
        
        if (!rknnResult.error) {
            const destroyResult = await client.sendRequest('rknn.destroy', {});
        }
        
    } catch (error) {
        console.error('❌ Debug failed:', error);
    } finally {
        client.disconnect();
    }
}

if (require.main === module) {
    debugParameterHandling().catch(console.error);
}

module.exports = { ParamsDebugger, debugParameterHandling };