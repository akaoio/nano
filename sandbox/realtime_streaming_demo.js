const net = require('net');

// Real-time visual streaming test - shows tokens as they arrive
class RealTimeStreamingTest {
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
                        
                        // Show streaming tokens in real-time
                        if (response.result && response.result.text !== null && response.result.text !== undefined) {
                            // Print token immediately as it arrives
                            process.stdout.write(response.result.text);
                            
                            // Show token details on new line occasionally
                            if (Math.random() < 0.1) { // 10% chance to show details
                                console.log(`\n[TOKEN ${response.result.token_id}]`);
                            }
                        }
                        
                        // Show completion message
                        if (response.result && response.result._callback_state === 2) {
                            console.log('\n\n✅ GENERATION COMPLETE!');
                            console.log(`📊 Performance: ${response.result.perf.generate_time_ms}ms for ${response.result.perf.generate_tokens} tokens`);
                            console.log(`💾 Memory usage: ${response.result.perf.memory_usage_mb}MB`);
                        }
                    } catch (e) {
                        // Invalid JSON, ignore
                    }
                });
            });
        });
    }

    async sendStreamingRequest(prompt) {
        const request = {
            jsonrpc: "2.0",
            method: "rkllm.run",
            params: [
                null,
                {
                    role: "user",
                    enable_thinking: false,
                    input_type: 0,
                    prompt_input: prompt
                },
                {
                    mode: 0,
                    lora_params: null,
                    prompt_cache_params: null,
                    keep_history: 0
                },
                null
            ],
            id: this.requestId++
        };
        
        console.log(`🚀 STARTING REAL-TIME GENERATION FOR: "${prompt}"`);
        console.log('📝 LIVE OUTPUT (tokens will appear instantly):');
        console.log('─'.repeat(60));
        
        this.socket.write(JSON.stringify(request) + '\n');
    }

    async initAsyncModel() {
        const initRequest = {
            jsonrpc: "2.0",
            method: "rkllm.init",
            params: [
                null,
                {
                    model_path: "/home/x/Projects/nano/models/qwen3/model.rkllm",
                    max_context_len: 512,
                    max_new_tokens: 150, // More tokens for longer streaming
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
                    is_async: true, // CRITICAL: Async mode for streaming
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
            ],
            id: this.requestId++
        };
        
        return new Promise((resolve) => {
            this.socket.write(JSON.stringify(initRequest) + '\n');
            setTimeout(resolve, 3000); // Wait for model to initialize
        });
    }

    async disconnect() {
        if (this.socket) {
            this.socket.end();
        }
    }
}

async function runRealTimeStreamingDemo() {
    console.log('🎬 REAL-TIME STREAMING DEMO');
    console.log('═'.repeat(60));
    console.log('🎯 This will show tokens appearing INSTANTLY as they are generated');
    console.log('⚡ You will see the text building up character by character in real-time');
    console.log('');
    
    const client = new RealTimeStreamingTest();
    
    try {
        await client.connect();
        
        console.log('📋 STEP 1: Initializing async model for streaming...');
        await client.initAsyncModel();
        console.log('✅ Model ready for real-time streaming\n');
        
        // Test multiple prompts to show continuous streaming
        const prompts = [
            "Tell me a creative story about a magical forest. Make it detailed and interesting.",
            "Explain quantum physics in simple terms with analogies.",
            "Write a poem about artificial intelligence and the future."
        ];
        
        for (let i = 0; i < prompts.length; i++) {
            console.log(`\n🎪 DEMO ${i + 1}/${prompts.length}:`);
            await client.sendStreamingRequest(prompts[i]);
            
            // Wait for generation to complete before next prompt
            await new Promise(resolve => setTimeout(resolve, 15000));
            
            console.log('\n');
            console.log('─'.repeat(60));
        }
        
    } catch (error) {
        console.error('❌ Real-time streaming demo failed:', error.message);
    } finally {
        await client.disconnect();
        console.log('\n🎬 REAL-TIME STREAMING DEMO COMPLETE');
        process.exit(0);
    }
}

// Run the real-time demo
runRealTimeStreamingDemo().catch(console.error);