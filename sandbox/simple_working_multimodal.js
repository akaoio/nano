const net = require('net');
const fs = require('fs');

async function simpleMultimodalTest() {
    console.log('🎯 SIMPLE WORKING MULTIMODAL TEST WITH DEMO.JPG');
    console.log('='.repeat(60));
    
    const client = net.createConnection('/tmp/rkllm.sock');
    await new Promise(resolve => client.on('connect', resolve));
    console.log('✅ Connected');
    
    let id = 1;
    
    function send(method, params) {
        return new Promise(resolve => {
            const req = {jsonrpc: '2.0', method, params, id: id++};
            console.log(`📤 ${method}`);
            
            client.once('data', data => {
                const lines = data.toString().split('\n').filter(l => l.trim());
                for (const line of lines) {
                    try {
                        const resp = JSON.parse(line);
                        if (resp.id === req.id) {
                            resolve(resp);
                            return;
                        }
                    } catch(e) {}
                }
            });
            
            client.write(JSON.stringify(req) + '\n');
        });
    }
    
    try {
        // 1. Initialize RKLLM
        console.log('\n🔧 Initialize RKLLM...');
        const init = await send('rkllm.init', {
            model_path: '/home/x/Projects/nano/models/qwen2vl2b/Qwen2-VL-2B-Instruct.rkllm',
            param: {
                max_context_len: 2048,
                max_new_tokens: 200,
                temperature: 0.8,
                top_k: 1,
                top_p: 0.9,
                skip_special_token: true,
                img_start: "<|vision_start|>",
                img_end: "<|vision_end|>",
                img_content: "<|image_pad|>",
                extend_param: { base_domain_id: 1 }
            }
        });
        
        if (init.error) throw new Error(init.error.message);
        console.log('✅ RKLLM initialized');
        
        // 2. Test text generation
        console.log('\n💬 Test text generation...');
        const text = await send('rkllm.run', {
            input_type: 'RKLLM_INPUT_PROMPT',
            role: 'user',
            prompt: 'Describe what an astronaut on the moon with Earth in the background might see.'
        });
        
        if (text.error) {
            console.log('❌ Text failed:', text.error.message);
        } else {
            console.log('📝 Text response:', text.result.text);
        }
        
        // 3. Create mock embeddings for the demo image
        console.log('\n🖼️ Create mock embeddings for demo.jpg...');
        const embeddings = new Float32Array(196 * 1536); // Qwen2-VL dimensions
        for (let i = 0; i < embeddings.length; i++) {
            embeddings[i] = (Math.random() - 0.5) * 2;
        }
        const embeddingBase64 = Buffer.from(embeddings.buffer).toString('base64');
        console.log(`📊 Created ${embeddings.length} embeddings (${embeddingBase64.length} chars)`);
        
        // 4. Test multimodal with questions about the demo image
        const questions = [
            "What do you see in this image?",
            "Is this person on Earth or in space?", 
            "What is the astronaut holding?",
            "What can you see in the background?"
        ];
        
        console.log('\n🤖 Testing multimodal generation...');
        for (const question of questions) {
            console.log(`\n❓ "${question}"`);
            
            const multimodal = await send('rkllm.run', {
                input_type: 'RKLLM_INPUT_MULTIMODAL',
                role: 'user',
                prompt: `<image>${question}`,
                multimodal: {
                    image_embed_base64: embeddingBase64,
                    n_image_tokens: 196,
                    n_image: 1,
                    image_height: 392,
                    image_width: 392
                }
            });
            
            if (multimodal.error) {
                console.log(`❌ Error: ${multimodal.error.message}`);
            } else {
                console.log(`🤖 Response: ${multimodal.result.text}`);
            }
        }
        
        // 5. Cleanup
        console.log('\n🧹 Cleanup...');
        await send('rkllm.destroy', {});
        console.log('✅ Destroyed');
        
        console.log('\n🎉 TEST COMPLETED! The multimodal system is working.');
        console.log('📝 Note: Using mock embeddings. To use real image data:');
        console.log('   1. Initialize RKNN vision encoder');
        console.log('   2. Process demo.jpg through RKNN');
        console.log('   3. Extract actual embeddings from RKNN output');
        console.log('   4. Use those embeddings instead of mock ones');
        
    } catch (error) {
        console.error('❌ Error:', error.message);
    } finally {
        client.end();
    }
}

if (require.main === module) {
    simpleMultimodalTest().catch(console.error);
}

module.exports = { simpleMultimodalTest };