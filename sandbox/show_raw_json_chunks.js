const net = require('net');

const client = net.createConnection('/tmp/rkllm.sock');

client.on('connect', () => {
    console.log('🔗 Connected to RKLLM server');
    
    // Send streaming request
    const request = {
        jsonrpc: "2.0",
        method: "rkllm.run",
        params: [
            null,
            {
                role: "user",
                enable_thinking: false,
                input_type: 0,
                prompt_input: "Write a short story."
            },
            {
                mode: 0,
                lora_params: null,
                prompt_cache_params: null,
                keep_history: 0
            },
            null
        ],
        id: 999
    };
    
    console.log('📤 Sending request...');
    client.write(JSON.stringify(request) + '\n');
});

client.on('data', (data) => {
    const chunks = data.toString().split('\n').filter(chunk => chunk.trim());
    
    chunks.forEach((chunk, index) => {
        console.log(`\n🔥 RAW JSON CHUNK ${index + 1}:`);
        console.log('─'.repeat(80));
        console.log(chunk);
        console.log('─'.repeat(80));
        
        try {
            const parsed = JSON.parse(chunk);
            console.log(`📝 EXTRACTED TEXT: "${parsed.result.text}"`);
            console.log(`🎯 TOKEN ID: ${parsed.result.token_id}`);
            console.log(`📊 CALLBACK STATE: ${parsed.result._callback_state}`);
        } catch (e) {
            console.log('❌ Failed to parse as JSON');
        }
    });
});

client.on('error', (err) => {
    console.error('❌ Connection error:', err.message);
});

setTimeout(() => {
    client.end();
    process.exit(0);
}, 10000);
