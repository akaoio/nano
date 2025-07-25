const net = require('net');

async function simpleTest() {
    const client = net.createConnection('/tmp/rkllm.sock');
    await new Promise(r => client.on('connect', r));
    
    let id = 1;
    const send = (method, params) => {
        console.log(`\n📤 ${method}`);
        client.write(JSON.stringify({ jsonrpc: '2.0', method, params, id: id++ }) + '\n');
    };
    
    // Collect all responses
    client.on('data', data => {
        data.toString().split('\n').filter(l => l.trim()).forEach(line => {
            try {
                const resp = JSON.parse(line);
                if (resp.result && resp.result.text !== undefined) {
                    process.stdout.write(resp.result.text || '');
                }
                if (resp.result && resp.result._callback_state === 2) {
                    console.log('\n✅ Done');
                }
            } catch(e) {}
        });
    });
    
    // Init RKLLM with multimodal
    send('rkllm.init', {
        model_path: './models/qwen2vl2b/Qwen2-VL-2B-Instruct.rkllm',
        param: {
            max_new_tokens: 50,
            extend_param: { base_domain_id: 1 }
        }
    });
    
    await new Promise(r => setTimeout(r, 2000));
    
    // Create dummy embeddings
    const embeddings = new Float32Array(196 * 1536).fill(0.01);
    const base64 = Buffer.from(embeddings.buffer).toString('base64');
    
    console.log('\n🤖 Response:');
    send('rkllm.run', {
        input_type: 'RKLLM_INPUT_MULTIMODAL',
        prompt: '<image>Test',
        multimodal: {
            image_embed_base64: base64,
            n_image_tokens: 196,
            n_image: 1,
            image_height: 392,
            image_width: 392
        }
    });
    
    await new Promise(r => setTimeout(r, 5000));
    client.end();
}

simpleTest();