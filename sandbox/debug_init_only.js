const net = require('net');

async function testInitOnly() {
    const client = new net.Socket();
    
    await new Promise((resolve, reject) => {
        client.connect('/tmp/rkllm.sock', resolve);
        client.on('error', reject);
    });
    
    let requestId = 1;
    
    async function sendRequest(method, params) {
        const request = { jsonrpc: '2.0', method, params, id: requestId++ };
        console.log(`\n📤 ${method}:`);
        console.log(JSON.stringify(params, null, 2));
        client.write(JSON.stringify(request) + '\n');
        
        return new Promise((resolve, reject) => {
            let buffer = '';
            const handleData = (data) => {
                buffer += data.toString();
                const lines = buffer.split('\n');
                
                for (let i = 0; i < lines.length - 1; i++) {
                    try {
                        const response = JSON.parse(lines[i]);
                        if (response.id === request.id) {
                            client.removeListener('data', handleData);
                            console.log(`✅ ${method} result:`, response.result ? 'SUCCESS' : 'FAILED');
                            if (response.error) console.log('Error:', response.error);
                            resolve(response);
                            return;
                        }
                    } catch (e) {}
                }
                buffer = lines[lines.length - 1];
            };
            client.on('data', handleData);
            
            // 30 second timeout for init operations
            setTimeout(() => {
                client.removeListener('data', handleData);
                console.log(`❌ ${method} timed out`);
                resolve({ error: 'timeout' });
            }, 30000);
        });
    }
    
    console.log('🧪 Testing model initialization only...');
    
    // Test RKLLM init
    await sendRequest('rkllm.init', {
        model_path: './models/qwen2vl2b/Qwen2-VL-2B-Instruct.rkllm',
        param: {
            max_context_len: 2048,
            max_new_tokens: 512,
            temperature: 0.7,
            top_k: 1,
            top_p: 0.9,
            skip_special_token: true,
            img_start: '<|vision_start|>',
            img_end: '<|vision_end|>',
            img_content: '<|image_pad|>',
            extend_param: {
                base_domain_id: 1
            }
        }
    });
    
    console.log('🔄 Waiting a moment before testing simple text...');
    await new Promise(resolve => setTimeout(resolve, 2000));
    
    // Try simple text
    const textResult = await sendRequest('rkllm.run', {
        prompt: 'Say hello'
    });
    
    client.end();
    console.log('🏁 Test completed');
}

testInitOnly().catch(console.error);