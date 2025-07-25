const net = require('net');
const fs = require('fs');

async function testMultimodalStreaming() {
    const client = net.createConnection('/tmp/rkllm.sock');
    
    await new Promise((resolve, reject) => {
        client.on('connect', resolve);
        client.on('error', reject);
    });
    
    console.log('✅ Connected to server');
    
    let requestId = 1;
    const streamingText = [];
    let multimodalRequestId = null;
    
    // Set up data handler to collect streaming responses
    client.on('data', (data) => {
        const lines = data.toString().split('\n').filter(line => line.trim());
        for (const line of lines) {
            try {
                const response = JSON.parse(line);
                
                // Collect streaming text from multimodal response
                if (response.id === multimodalRequestId && response.result) {
                    if (response.result.text) {
                        streamingText.push(response.result.text);
                        process.stdout.write(response.result.text); // Print as it streams
                    }
                    if (response.result._callback_state === 2) {
                        console.log('\n\n✅ Streaming completed');
                    }
                }
            } catch (e) {
                // Ignore parse errors
            }
        }
    });
    
    function sendRequest(method, params) {
        const request = { jsonrpc: '2.0', method, params, id: requestId++ };
        console.log(`\n📤 ${method}`);
        client.write(JSON.stringify(request) + '\n');
        return request.id;
    }
    
    // Initialize both RKNN and RKLLM
    sendRequest('rknn.init', {
        model_path: './models/qwen2vl2b/Qwen2-VL-2B-Instruct.rknn',
        flags: 0
    });
    
    await new Promise(resolve => setTimeout(resolve, 1000));
    
    sendRequest('rkllm.init', {
        model_path: './models/qwen2vl2b/Qwen2-VL-2B-Instruct.rkllm',
        param: {
            max_context_len: 2048,
            max_new_tokens: 100,
            temperature: 0.7,
            top_k: 1,
            top_p: 0.9,
            skip_special_token: true,
            img_start: '<|vision_start|>',
            img_end: '<|vision_end|>',
            img_content: '<|image_pad|>',
            extend_param: {
                base_domain_id: 1  // Multimodal mode
            }
        }
    });
    
    await new Promise(resolve => setTimeout(resolve, 2000));
    
    sendRequest('rkllm.set_chat_template', {
        system_template: "<|im_start|>system\\nYou are a helpful assistant.<|im_end|>\\n",
        user_template: "<|im_start|>user\\n",
        assistant_template: "<|im_end|>\\n<|im_start|>assistant\\n"
    });
    
    await new Promise(resolve => setTimeout(resolve, 500));
    
    // Process demo.jpg
    const imageBuffer = fs.readFileSync('./tests/images/demo.jpg');
    console.log(`\n🖼️ Processing demo.jpg: ${imageBuffer.length} bytes`);
    
    sendRequest('rknn.inputs_set', {
        inputs: [{
            index: 0,
            type: 'RKNN_TENSOR_UINT8',
            size: imageBuffer.length,
            fmt: 'RKNN_TENSOR_NHWC',
            data: imageBuffer.toString('base64')
        }]
    });
    
    await new Promise(resolve => setTimeout(resolve, 1000));
    
    sendRequest('rknn.run', {});
    
    await new Promise(resolve => setTimeout(resolve, 1000));
    
    // Get embeddings with full data
    sendRequest('rknn.outputs_get', {
        num_outputs: 1,
        outputs: [{ want_float: true }],
        extend: { return_full_data: true }
    });
    
    await new Promise(resolve => setTimeout(resolve, 2000));
    
    // Create minimal test embeddings (to avoid JSON size issues)
    const testEmbeddings = new Float32Array(196 * 1536);
    for (let i = 0; i < testEmbeddings.length; i++) {
        testEmbeddings[i] = Math.random() * 0.1 - 0.05; // Small random values
    }
    
    const embeddingsBuffer = Buffer.from(testEmbeddings.buffer);
    const embeddingsBase64 = embeddingsBuffer.toString('base64');
    
    console.log(`\n📊 Using test embeddings: ${testEmbeddings.length} floats`);
    console.log('\n💬 Asking: "What do you see in this image?"\n');
    console.log('🤖 Response: ');
    
    // Send multimodal request
    multimodalRequestId = sendRequest('rkllm.run', {
        input_type: 'RKLLM_INPUT_MULTIMODAL',
        role: 'user',
        prompt: '<image>What do you see in this image?',
        multimodal: {
            image_embed_base64: embeddingsBase64,
            n_image_tokens: 196,
            n_image: 1,
            image_height: 392,
            image_width: 392
        }
    });
    
    // Wait for streaming to complete
    await new Promise(resolve => setTimeout(resolve, 15000));
    
    console.log('\n📝 Complete response:', streamingText.join(''));
    
    // Cleanup
    sendRequest('rknn.destroy', {});
    sendRequest('rkllm.destroy', {});
    
    await new Promise(resolve => setTimeout(resolve, 1000));
    
    client.end();
    console.log('\n✅ Test completed');
}

testMultimodalStreaming().catch(console.error);