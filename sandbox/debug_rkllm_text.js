const net = require('net');

async function testRKLLMTextOnly() {
    const client = new net.Socket();
    
    await new Promise((resolve, reject) => {
        client.connect('/tmp/rkllm.sock', resolve);
        client.on('error', reject);
    });
    
    let requestId = 1;
    
    async function sendRequest(method, params) {
        const request = { jsonrpc: '2.0', method, params, id: requestId++ };
        console.log('Sending:', method, JSON.stringify(params, null, 2));
        client.write(JSON.stringify(request) + '\n');
        
        return new Promise((resolve) => {
            let buffer = '';
            const handleData = (data) => {
                buffer += data.toString();
                const lines = buffer.split('\n');
                
                for (let i = 0; i < lines.length - 1; i++) {
                    try {
                        const response = JSON.parse(lines[i]);
                        if (response.id === request.id) {
                            client.removeListener('data', handleData);
                            resolve(response);
                            return;
                        }
                    } catch (e) {}
                }
                buffer = lines[lines.length - 1];
            };
            client.on('data', handleData);
        });
    }
    
    console.log('🧪 Testing RKLLM text-only mode...');
    
    // Test simple text prompt
    const result = await sendRequest('rkllm.run', {
        input_type: 'RKLLM_INPUT_PROMPT',
        role: 'user',
        prompt: 'Hello! Can you tell me a short joke?'
    });
    
    console.log('✅ RKLLM Text Result:');
    console.log(JSON.stringify(result, null, 2));
    
    client.end();
}

testRKLLMTextOnly().catch(console.error);