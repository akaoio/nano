const net = require('net');

async function testConnection() {
    console.log('🔍 Testing basic connection...');
    
    return new Promise((resolve, reject) => {
        const client = net.createConnection('/tmp/rkllm.sock');
        
        client.on('connect', () => {
            console.log('✅ Connected successfully');
            
            // Send a simple test request
            const request = {
                jsonrpc: "2.0",
                method: "rkllm.init",
                params: {
                    model_path: './models/qwen2vl2b/Qwen2-VL-2B-Instruct.rkllm',
                    param: {
                        max_context_len: 1024,
                        max_new_tokens: 100,
                        temperature: 0.8,
                        top_k: 1,
                        top_p: 0.9
                    }
                },
                id: 1
            };
            
            console.log('📤 Sending RKLLM init request...');
            client.write(JSON.stringify(request) + '\n');
            
            // Set timeout
            setTimeout(() => {
                console.log('⏰ Request timed out after 30 seconds');
                client.end();
                resolve();
            }, 30000);
        });
        
        client.on('data', (data) => {
            console.log('📥 Received data:', data.toString());
            client.end();
            resolve();
        });
        
        client.on('error', (err) => {
            console.error('❌ Connection error:', err.message);
            reject(err);
        });
        
        client.on('close', () => {
            console.log('🔌 Connection closed');
        });
    });
}

testConnection().catch(console.error);