const net = require('net');

async function testRKNNInit() {
    console.log('🔍 Testing RKNN initialization...');
    
    return new Promise((resolve, reject) => {
        const client = net.createConnection('/tmp/rkllm.sock');
        
        client.on('connect', () => {
            console.log('✅ Connected successfully');
            
            // Send RKNN init request
            const request = {
                jsonrpc: "2.0",
                method: "rknn.init",
                params: {
                    model_path: './models/qwen2vl2b/Qwen2-VL-2B-Instruct.rknn',
                    flags: 0
                },
                id: 1
            };
            
            console.log('📤 Sending RKNN init request...');
            console.log('📂 Model path:', request.params.model_path);
            client.write(JSON.stringify(request) + '\n');
            
            // Set timeout
            setTimeout(() => {
                console.log('⏰ RKNN init timed out after 60 seconds');
                client.end();
                resolve();
            }, 60000);
        });
        
        client.on('data', (data) => {
            console.log('📥 Received RKNN response:', data.toString());
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

testRKNNInit().catch(console.error);