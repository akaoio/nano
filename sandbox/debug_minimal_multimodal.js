const net = require('net');

async function testMinimalMultimodal() {
    const client = net.createConnection('/tmp/rkllm.sock');
    
    await new Promise((resolve, reject) => {
        client.on('connect', resolve);
        client.on('error', reject);
    });
    
    let requestId = 1;
    
    function sendRequest(method, params) {
        return new Promise((resolve) => {
            const request = { jsonrpc: '2.0', method, params, id: requestId++ };
            console.log(`📤 ${method.toUpperCase()}:`);
            
            if (method === 'rkllm.run') {
                // For multimodal run, don't log the huge base64 data
                const logParams = { ...params };
                if (logParams.multimodal && logParams.multimodal.image_embed_base64) {
                    logParams.multimodal.image_embed_base64 = `[${logParams.multimodal.image_embed_base64.length} base64 chars]`;
                }
                console.log(JSON.stringify(logParams, null, 2));
            } else {
                console.log(JSON.stringify(params, null, 2));
            }
            
            client.write(JSON.stringify(request) + '\n');
            
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
    
    console.log('🧪 Minimal multimodal test with base64 embeddings');
    
    // Create small test embeddings (196 * 1536 = 301056 floats)
    const embeddings = new Float32Array(301056);
    for (let i = 0; i < embeddings.length; i++) {
        embeddings[i] = Math.random() * 2 - 1;
    }
    
    // Convert to base64
    const buffer = Buffer.from(embeddings.buffer);
    const embeddingsBase64 = buffer.toString('base64');
    
    console.log(`📊 Created ${embeddings.length} embeddings as ${embeddingsBase64.length} base64 chars`);
    
    // Test multimodal run
    const result = await sendRequest('rkllm.run', {
        input_type: 'RKLLM_INPUT_MULTIMODAL',
        role: 'user',
        prompt: '<image>What do you see?',
        multimodal: {
            image_embed_base64: embeddingsBase64,
            n_image_tokens: 196,
            n_image: 1,
            image_height: 392,
            image_width: 392
        }
    });
    
    console.log('✅ Result:', result.result ? 'SUCCESS' : 'FAILED');
    if (result.error) console.log('❌ Error:', result.error);
    if (result.result) console.log('📝 Response:', result.result.text || JSON.stringify(result.result));
    
    client.end();
}

testMinimalMultimodal().catch(console.error);