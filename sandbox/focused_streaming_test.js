const net = require('net');

// Direct streaming test - focus on callback behavior
class StreamingTest {
    constructor() {
        this.socket = null;
        this.requestId = 1;
    }

    connect() {
        return new Promise((resolve, reject) => {
            this.socket = net.createConnection('/tmp/rkllm.sock');
            
            this.socket.on('connect', () => {
                console.log('🔗 Connected to RKLLM server');
                resolve();
            });
            
            this.socket.on('error', reject);
            
            this.socket.on('data', (data) => {
                const messages = data.toString().split('\n').filter(msg => msg.trim());
                messages.forEach(msg => {
                    try {
                        const response = JSON.parse(msg);
                        console.log('📥 STREAMING DATA:', JSON.stringify(response, null, 2));
                        
                        // Check if this is real streaming data
                        if (response.result && response.result.text) {
                            console.log(`🎯 TOKEN: "${response.result.text}" (ID: ${response.result.token_id})`);
                            console.log(`📊 CALLBACK STATE: ${response.result._callback_state}`);
                        }
                    } catch (e) {
                        console.log('📥 RAW DATA:', msg);
                    }
                });
            });
        });
    }

    async sendRequest(method, params) {
        const request = {
            jsonrpc: "2.0",
            method: method,
            params: params,
            id: this.requestId++
        };
        
        console.log(`📤 SENDING: ${method}`);
        console.log(JSON.stringify(request, null, 2));
        
        return new Promise((resolve) => {
            this.socket.write(JSON.stringify(request) + '\n');
            setTimeout(resolve, 1000); // Wait for potential streaming
        });
    }

    async disconnect() {
        if (this.socket) {
            this.socket.end();
        }
    }
}

async function testRealStreaming() {
    console.log('🚀 REAL STREAMING TEST - CHECKING CALLBACK BEHAVIOR');
    console.log('==================================================');
    
    const client = new StreamingTest();
    
    try {
        await client.connect();
        
        // Initialize model with async=true
        console.log('\n📋 STEP 1: Initialize model with is_async=true');
        await client.sendRequest('rkllm.init', [
            null,
            {
                model_path: "/home/x/Projects/nano/models/qwen3/model.rkllm",
                max_context_len: 512,
                max_new_tokens: 100, // More tokens for real streaming
                top_k: 40,
                n_keep: 0,
                top_p: 0.9,
                temperature: 0.8,
                repeat_penalty: 1.1,
                frequency_penalty: 0,
                presence_penalty: 0,
                mirostat: 0,
                mirostat_tau: 5,
                mirostat_eta: 0.1,
                skip_special_token: false,
                is_async: true, // CRITICAL: Enable async mode
                img_start: null,
                img_end: null,
                img_content: null,
                extend_param: {
                    base_domain_id: 0,
                    embed_flash: 0,
                    enabled_cpus_num: 4,
                    enabled_cpus_mask: 15,
                    n_batch: 1,
                    use_cross_attn: 0,
                    reserved: null
                }
            },
            null
        ]);
        
        await new Promise(resolve => setTimeout(resolve, 3000)); // Wait for model init
        
        // Test real streaming with longer prompt
        console.log('\n📋 STEP 2: Send streaming request with longer generation');
        await client.sendRequest('rkllm.run', [
            null,
            {
                role: "user",
                enable_thinking: false,
                input_type: 0,
                prompt_input: "Write a short story about a robot. Make it at least 50 words long."
            },
            {
                mode: 0,
                lora_params: null,
                prompt_cache_params: null,
                keep_history: 0
            },
            null
        ]);
        
        // Wait longer for streaming
        console.log('\n⏳ Waiting 10 seconds for streaming data...');
        await new Promise(resolve => setTimeout(resolve, 10000));
        
    } catch (error) {
        console.error('❌ Test failed:', error.message);
    } finally {
        await client.disconnect();
        process.exit(0);
    }
}

// Run the focused test
testRealStreaming().catch(console.error);