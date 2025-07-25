const net = require('net');
const fs = require('fs');

// Simple client
const socket = net.createConnection('/tmp/rkllm.sock', () => {
    console.log('Connected');
    
    // Send rknn.outputs_get request
    const req = {
        jsonrpc: '2.0',
        method: 'rknn.outputs_get',
        params: {
            n_outputs: 1,
            outputs: [{ want_float: true }],
            extend: { return_full_data: true }
        },
        id: 1
    };
    
    socket.write(JSON.stringify(req) + '\n');
});

socket.on('data', (data) => {
    console.log('Raw response:', data.toString());
    socket.end();
});

socket.on('error', (err) => {
    console.error('Error:', err);
});