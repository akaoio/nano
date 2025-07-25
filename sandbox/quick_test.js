const net = require('net');
const fs = require('fs');

async function quickTest() {
    const client = new net.Socket();
    
    await new Promise((resolve, reject) => {
        client.connect('/tmp/rkllm.sock', resolve);
        client.on('error', reject);
    });
    
    async function sendRequest(method, params) {
        const request = {
            jsonrpc: "2.0",
            method: method,
            params: params,
            id: Date.now()
        };
        
        client.write(JSON.stringify(request) + '\n');
        
        return new Promise((resolve) => {
            client.once('data', (data) => {
                const response = JSON.parse(data.toString());
                resolve(response);
            });
        });
    }
    
    console.log('Testing RKNN outputs_get parameter handling...');
    
    // Test with just basic initialization
    const initResult = await sendRequest({
        method: 'rknn.init',
        params: {
            model_path: './models/qwen2vl2b/Qwen2-VL-2B-Instruct.rknn',
            flags: 0
        }
    });
    
    console.log('RKNN init result:', initResult.result ? 'SUCCESS' : 'FAILED');
    
    // Test outputs_get with num_outputs parameter
    const outputsResult = await sendRequest('rknn.outputs_get', {
        num_outputs: 1,
        outputs: [{ want_float: false }]  // Don't request float data
    });
    
    console.log('RKNN outputs_get result:', outputsResult.result ? 'SUCCESS' : 'FAILED');
    if (outputsResult.error) {
        console.log('Error:', outputsResult.error.message);
    }
    
    client.end();
}

quickTest().catch(console.error);