#!/usr/bin/env node

import { spawn } from 'child_process';
import http from 'http';

function makeRequest(method, params) {
    const requestData = JSON.stringify({
        "jsonrpc": "2.0",
        "id": Math.floor(Math.random() * 1000),
        "method": method,
        "params": params || {}
    });

    const options = {
        hostname: '127.0.0.1',
        port: 8082,
        path: '/',
        method: 'POST',
        headers: {
            'Content-Type': 'application/json',
            'Content-Length': Buffer.byteLength(requestData)
        }
    };

    return new Promise((resolve, reject) => {
        const req = http.request(options, (res) => {
            let body = '';
            res.on('data', (chunk) => body += chunk);
            res.on('end', () => {
                try {
                    const parsed = JSON.parse(body);
                    resolve(parsed);
                } catch (e) {
                    reject(new Error(`Invalid JSON response: ${body}`));
                }
            });
        });

        req.on('error', (err) => reject(err));
        req.write(requestData);
        req.end();
    });
}

async function testHttpBufferManager() {
    console.log('🧪 Testing HTTP Buffer Manager Implementation');
    console.log('============================================');
    
    // Start server
    console.log('📍 Starting server...');
    const server = spawn('./build/mcp_server', [], { cwd: '/home/x/Projects/nano' });
    
    server.stdout.on('data', (data) => {
        console.log(`[SERVER] ${data.toString().trim()}`);
    });
    
    server.stderr.on('data', (data) => {
        console.log(`[SERVER ERROR] ${data.toString().trim()}`);
    });
    
    // Wait for server to start and enter event loop
    console.log('⏳ Waiting for server to initialize...');
    await new Promise(resolve => setTimeout(resolve, 5000));
    
    // Try to establish connection to verify server is accepting requests
    console.log('🔍 Verifying server is accepting connections...');
    let connected = false;
    for (let i = 0; i < 10 && !connected; i++) {
        try {
            console.log(`   Connection attempt ${i + 1}/10...`);
            const testResult = await makeRequest('rkllm_list_functions', {});
            if (testResult) {
                console.log('✅ Server is accepting HTTP requests');
                connected = true;
            }
        } catch (error) {
            console.log(`   Attempt ${i + 1} failed: ${error.message}`);
            await new Promise(resolve => setTimeout(resolve, 1000));
        }
    }
    
    if (!connected) {
        throw new Error('Server failed to accept HTTP connections after 10 attempts');
    }
    
    try {
        console.log('1️⃣ Testing HTTP buffer manager status...');
        const statusResult = await makeRequest('http_buffer_manager_status', {});
        console.log('✅ Status response:', JSON.stringify(statusResult, null, 2));
        
        console.log('2️⃣ Testing buffer creation and chunk management...');
        const bufferResult = await makeRequest('test_http_buffer_manager', {
            request_id: 'test_123',
            mock_chunks: [
                { seq: 0, delta: 'Hello', end: false },
                { seq: 1, delta: ' world', end: false },
                { seq: 2, delta: '!', end: true }
            ]
        });
        console.log('✅ Buffer test response:', JSON.stringify(bufferResult, null, 2));
        
        console.log('3️⃣ Testing buffer cleanup...');
        const cleanupResult = await makeRequest('test_buffer_cleanup', {
            create_expired_buffers: 2,
            timeout_seconds: 1
        });
        console.log('✅ Cleanup test response:', JSON.stringify(cleanupResult, null, 2));
        
        console.log('🎉 All HTTP Buffer Manager tests completed successfully!');
        
    } catch (error) {
        console.error('❌ Test failed:', error.message);
    } finally {
        console.log('🛑 Stopping server...');
        server.kill('SIGTERM');
        setTimeout(() => server.kill('SIGKILL'), 2000);
    }
}

testHttpBufferManager().catch(console.error);