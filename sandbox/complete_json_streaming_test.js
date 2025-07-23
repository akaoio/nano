const net = require('net');

class JSONStreamingInspector {
    constructor() {
        this.socket = null;
        this.requestId = 1;
    }

    async connect() {
        return new Promise((resolve, reject) => {
            this.socket = net.createConnection('/tmp/rkllm.sock');
            
            this.socket.on('connect', () => {
                console.log('🔗 Connected to RKLLM server');
                resolve();
            });
            
            this.socket.on('error', reject);
            
            this.socket.on('data', (data) => {
                console.log('\n🔥 RAW DATA RECEIVED:');
                console.log('=' .repeat(100));
                console.log(data.toString());
                console.log('=' .repeat(100));
                
                // Split by newlines and show each JSON chunk
                const lines = data.toString().split('\n').filter(line => line.trim());
                lines.forEach((line, index) => {
                    console.log(`\n📦 JSON CHUNK ${index + 1}:`);
                    console.log('─' .repeat(80));
                    console.log(line);
                    console.log('─' .repeat(80));
                    
                    try {
                        const parsed = JSON.parse(line);
                        if (parsed.result && parsed.result.text !== null) {
                            console.log(`🎯 EXTRACTED TEXT: "${parsed.result.text}"`);
                            console.log(`🔢 TOKEN ID: ${parsed.result.token_id}`);
                            console.log(`📊 CALLBACK STATE: ${parsed.result._callback_state}`);
                            console.log(`�� REQUEST ID: ${parsed.id}`);
                        }
                    } catch (e) {
                        console.log('❌ Not valid JSON or different format');
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
        
        console.log(`\n📤 SENDING REQUEST: ${method}`);
        console.log(JSON.stringify(request, null, 2));
        
        this.socket.write(JSON.stringify(request) + '\n');
        
        // Wait for response
        await new Promise(resolve => setTimeout(resolve, 1000));
    }

    disconnect() {
        if (this.socket) {
            this.socket.end();
        }
    }
}

async function inspectJSONStreaming() {
    console.log('🔍 JSON STREAMING INSPECTOR');
    console.log('🎯 This will show you EXACTLY what JSON is sent for each streaming chunk');
    console.log('');
    
    const inspector = new JSONStreamingInspector();
    
    try {
        await inspector.connect();
        
        // Step 1: Initialize async model
        console.log('\n🔧 STEP 1: Initialize async model...');
        await inspector.sendRequest('rkllm.init', [
            null,
            {
                model_path: "/home/x/Projects/nano/models/qwen3/model.rkllm",
                max_context_len: 512,
                max_new_tokens: 50,
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
                is_async: true,  // ENABLE ASYNC STREAMING
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
        
        await new Promise(resolve => setTimeout(resolve, 3000));
        
        // Step 2: Start streaming generation
        console.log('\n🚀 STEP 2: Start streaming generation...');
        console.log('🔥 Watch for multiple JSON-RPC responses (one per token):');
        
        await inspector.sendRequest('rkllm.run', [
            null,
            {
                role: "user",
                enable_thinking: false,
                input_type: 0,
                prompt_input: "Write a short creative story about robots."
            },
            {
                mode: 0,
                lora_params: null,
                prompt_cache_params: null,
                keep_history: 0
            },
            null
        ]);
        
        // Wait for streaming to complete
        await new Promise(resolve => setTimeout(resolve, 10000));
        
    } catch (error) {
        console.error('❌ Error:', error.message);
    } finally {
        inspector.disconnect();
        console.log('\n�� JSON STREAMING INSPECTION COMPLETE');
        process.exit(0);
    }
}

inspectJSONStreaming();
