const net = require('net');
const fs = require('fs');

async function testRKNNOnly() {
    const client = net.createConnection('/tmp/rkllm.sock');
    await new Promise(r => client.on('connect', r));
    
    let id = 1;
    const send = (method, params) => {
        const req = { jsonrpc: '2.0', method, params, id: id++ };
        console.log(`\n📤 ${method}:`, params);
        client.write(JSON.stringify(req) + '\n');
        
        return new Promise(resolve => {
            const handler = data => {
                data.toString().split('\n').filter(l => l).forEach(line => {
                    try {
                        const resp = JSON.parse(line);
                        if (resp.id === req.id) {
                            client.off('data', handler);
                            resolve(resp);
                        }
                    } catch(e) {}
                });
            };
            client.on('data', handler);
        });
    };
    
    try {
        // Test RKNN vision encoder only
        console.log('Testing RKNN vision encoder...');
        
        const initResp = await send('rknn.init', {
            model_path: './models/qwen2vl2b/Qwen2-VL-2B-Instruct.rknn',
            flags: 0
        });
        console.log('✅ RKNN init result:', initResp.result);
        
        const imageData = fs.readFileSync('./tests/images/demo.jpg');
        console.log(`📸 Image size: ${imageData.length} bytes`);
        
        const inputResp = await send('rknn.inputs_set', {
            inputs: [{
                index: 0,
                type: 'RKNN_TENSOR_UINT8',
                size: imageData.length,
                fmt: 'RKNN_TENSOR_NHWC',
                data: imageData.toString('base64')
            }]
        });
        console.log('✅ Inputs set result:', inputResp.result);
        
        const runResp = await send('rknn.run', {});
        console.log('✅ Run result:', runResp.result);
        
        const outputResp = await send('rknn.outputs_get', {
            n_outputs: 1,
            outputs: [{ want_float: true }]
        });
        console.log('✅ Outputs result:', JSON.stringify(outputResp.result, null, 2));
        
        if (outputResp.result && outputResp.result.outputs && outputResp.result.outputs.length > 0) {
            const output = outputResp.result.outputs[0];
            console.log(`📊 Output details: ${output.num_floats} floats, size: ${output.size} bytes`);
        }
        
    } catch (error) {
        console.error('❌ Error:', error);
    } finally {
        client.end();
    }
}

testRKNNOnly();