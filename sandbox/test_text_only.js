const net = require('net');

async function testTextOnly() {
    const client = net.createConnection('/tmp/rkllm.sock');
    
    await new Promise((resolve, reject) => {
        client.on('connect', resolve);
        client.on('error', reject);
    });
    
    console.log('✅ Connected to server');
    
    let requestId = 1;
    const allResponses = [];
    
    // Set up data handler to collect ALL responses
    client.on('data', (data) => {
        const lines = data.toString().split('\n').filter(line => line.trim());
        for (const line of lines) {
            try {
                const response = JSON.parse(line);
                console.log('📥 Response:', JSON.stringify(response));
                allResponses.push(response);
            } catch (e) {
                console.log('📄 Non-JSON data:', line);
            }
        }
    });
    
    function sendRequest(method, params) {
        const request = { jsonrpc: '2.0', method, params, id: requestId++ };
        console.log(`\n📤 ${method}:`, JSON.stringify(params, null, 2));
        client.write(JSON.stringify(request) + '\n');
    }
    
    // Initialize RKLLM ONLY
    sendRequest('rkllm.init', {
        model_path: './models/qwen2vl2b/Qwen2-VL-2B-Instruct.rkllm',
        param: {
            max_context_len: 512,
            max_new_tokens: 100,
            temperature: 0.7,
            top_k: 1,
            top_p: 0.9,
            skip_special_token: true,
            extend_param: {
                base_domain_id: 0  // Text-only mode
            }
        }
    });
    
    // Wait for init response
    await new Promise(resolve => setTimeout(resolve, 3000));
    
    // Test simple text generation
    sendRequest('rkllm.run', {
        input_type: 'RKLLM_INPUT_PROMPT',
        role: 'user',
        prompt: 'Hello, what is 2+2?'
    });
    
    // Wait for all responses (including streaming)
    await new Promise(resolve => setTimeout(resolve, 10000));
    
    console.log('\n📊 Total responses received:', allResponses.length);
    
    // Cleanup
    sendRequest('rkllm.destroy', {});
    
    await new Promise(resolve => setTimeout(resolve, 1000));
    
    client.end();
    console.log('\n✅ Test completed');
}

testTextOnly().catch(console.error);