const net = require('net');

class RKLLMClient {
    constructor() {
        this.socket = null;
        this.requestId = 0;
        this.pendingRequests = new Map();
    }

    connect() {
        return new Promise((resolve, reject) => {
            this.socket = net.createConnection('/tmp/rkllm.sock');
            
            this.socket.on('connect', () => {
                console.log('✓ Connected to RKLLM server');
                resolve();
            });

            this.socket.on('error', (err) => {
                console.error('✗ Connection error:', err.message);
                reject(err);
            });

            this.socket.on('data', (data) => {
                this.handleResponse(data.toString());
            });

            this.socket.on('close', () => {
                console.log('Disconnected from server');
            });
        });
    }

    handleResponse(data) {
        try {
            const response = JSON.parse(data);
            console.log('Raw response:', JSON.stringify(response, null, 2));
            
            if (response.id && this.pendingRequests.has(response.id)) {
                const { resolve, reject } = this.pendingRequests.get(response.id);
                this.pendingRequests.delete(response.id);
                
                if (response.error) {
                    reject(new Error(response.error.message));
                } else {
                    resolve(response.result);
                }
            }
        } catch (err) {
            console.error('Failed to parse response:', err.message);
            console.error('Raw data:', data);
        }
    }

    async request(method, params = {}, timeout = 10000) {
        return new Promise((resolve, reject) => {
            const id = ++this.requestId;
            const request = {
                jsonrpc: "2.0",
                method,
                params,
                id
            };

            this.pendingRequests.set(id, { resolve, reject });

            // Set timeout
            const timer = setTimeout(() => {
                if (this.pendingRequests.has(id)) {
                    this.pendingRequests.delete(id);
                    reject(new Error(`Request timed out after ${timeout}ms`));
                }
            }, timeout);

            // Clear timeout when request completes
            const originalResolve = resolve;
            const originalReject = reject;
            
            this.pendingRequests.set(id, {
                resolve: (result) => {
                    clearTimeout(timer);
                    originalResolve(result);
                },
                reject: (error) => {
                    clearTimeout(timer);
                    originalReject(error);
                }
            });

            console.log('Sending request:', JSON.stringify(request, null, 2));
            this.socket.write(JSON.stringify(request) + '\n');
        });
    }

    disconnect() {
        if (this.socket) {
            this.socket.end();
        }
    }
}

async function runDiagnostic() {
    console.log('🔍 Starting LoRa Diagnostic Test\n');
    
    const client = new RKLLMClient();

    try {
        await client.connect();

        console.log('\n=== Step 1: Initialize Base Model ===');
        const initResult = await client.request('rkllm.init', {
            model_path: '/home/x/Projects/nano/models/lora/model.rkllm',
            max_context_len: 512,
            max_new_tokens: 50,
            top_k: 1,
            top_p: 0.9,
            temperature: 0.8,
            repeat_penalty: 1.1,
            frequency_penalty: 0.0,
            presence_penalty: 0.0
        }, 15000);
        console.log('Init result:', initResult);

        console.log('\n=== Step 2: Load LoRa Adapter ===');
        const loraResult = await client.request('rkllm.load_lora', {
            lora_path: '/home/x/Projects/nano/models/lora/lora.rkllm'
        }, 15000);
        console.log('LoRa result:', loraResult);

        console.log('\n=== Step 3: Test Simple Inference ===');
        console.log('Attempting minimal inference with 5 second timeout...');
        
        try {
            const inferenceResult = await client.request('rkllm.run', {
                prompt: "Hello",
                max_new_tokens: 5
            }, 5000);
            console.log('✓ Inference successful:', inferenceResult);
        } catch (inferenceError) {
            console.log('✗ Inference failed:', inferenceError.message);
            
            console.log('\n=== Step 4: Check Server Status ===');
            try {
                const statusResult = await client.request('rkllm.is_running', {}, 2000);
                console.log('Server status:', statusResult);
            } catch (statusError) {
                console.log('✗ Status check failed:', statusError.message);
            }
        }

    } catch (error) {
        console.error('✗ Diagnostic failed:', error.message);
    } finally {
        client.disconnect();
    }
}

if (require.main === module) {
    runDiagnostic().catch(console.error);
}