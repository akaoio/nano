#!/usr/bin/env node

const { spawn } = require('child_process');
const http = require('http');

async function sleep(ms) {
    return new Promise(resolve => setTimeout(resolve, ms));
}

async function testHTTPBufferManager() {
    console.log('🧪 Testing Phase 3 HTTP Buffer Manager Implementation');
    console.log('='.repeat(60));
    
    // Start the MCP server with HTTP only
    console.log('🚀 Starting MCP server...');
    const server = spawn('./build/mcp_server', [
        '--http', '8082',
        '--disable-stdio',
        '--disable-tcp', 
        '--disable-udp',
        '--disable-ws'
    ], {
        cwd: '/home/x/Projects/nano',
        stdio: ['pipe', 'pipe', 'pipe']
    });
    
    let serverOutput = '';
    server.stdout.on('data', (data) => {
        serverOutput += data.toString();
        console.log('[SERVER]', data.toString().trim());
    });
    
    server.stderr.on('data', (data) => {
        serverOutput += data.toString();
        console.log('[SERVER]', data.toString().trim());
    });
    
    // Wait for server to start
    await sleep(2000);
    
    try {
        // Test 1: Start streaming request
        console.log('\n📤 Test 1: Starting streaming request...');
        const streamRequest = {
            jsonrpc: '2.0',
            id: 'test-stream-1',
            method: 'generateText',
            params: {
                model: 'test-model',
                prompt: 'Hello, world!',
                temperature: 0.7,
                stream: true
            }
        };
        
        const startResult = await httpRequest('POST', 'http://localhost:8082/', JSON.stringify(streamRequest));
        console.log('✅ Start streaming response:', startResult.substring(0, 200) + '...');
        
        await sleep(1000);
        
        // Test 2: Poll for streaming chunks  
        console.log('\n📥 Test 2: Polling for streaming chunks...');
        const pollRequest = {
            jsonrpc: '2.0',
            id: 'test-poll-1',
            method: 'streamPoll',
            params: {
                request_id: 'test-stream-1'
            }
        };
        
        const pollResult = await httpRequest('POST', 'http://localhost:8082/', JSON.stringify(pollRequest));
        console.log('✅ Poll response:', pollResult.substring(0, 300) + '...');
        
        await sleep(500);
        
        // Test 3: Test HTTP Buffer Manager operations
        console.log('\n🧪 Test 3: Testing HTTP Buffer Manager operations...');
        const bufferTestRequest = {
            jsonrpc: '2.0',
            id: 'test-buffer-1',
            method: 'testHttpBufferManager',
            params: {}
        };
        
        const bufferResult = await httpRequest('POST', 'http://localhost:8082/', JSON.stringify(bufferTestRequest));
        console.log('✅ Buffer test response:', bufferResult);
        
        await sleep(500);
        
        // Test 4: Test buffer cleanup
        console.log('\n🧹 Test 4: Testing buffer cleanup...');
        const cleanupTestRequest = {
            jsonrpc: '2.0',
            id: 'test-cleanup-1',
            method: 'testBufferCleanup',
            params: {}
        };
        
        const cleanupResult = await httpRequest('POST', 'http://localhost:8082/', JSON.stringify(cleanupTestRequest));
        console.log('✅ Cleanup test response:', cleanupResult);
        
        console.log('\n✅ All HTTP Buffer Manager tests completed successfully!');
        
    } catch (error) {
        console.error('❌ Test failed:', error.message);
    } finally {
        // Cleanup
        console.log('\n🛑 Stopping server...');
        server.kill('SIGTERM');
        
        // Wait for graceful shutdown
        await sleep(2000);
        
        if (!server.killed) {
            console.log('🔪 Force killing server...');
            server.kill('SIGKILL');
        }
        
        console.log('✅ Test completed');
    }
}

function httpRequest(method, url, data) {
    return new Promise((resolve, reject) => {
        const urlObj = new URL(url);
        const options = {
            hostname: urlObj.hostname,
            port: urlObj.port || 80,
            path: urlObj.pathname,
            method: method,
            headers: {
                'Content-Type': 'application/json',
                'Content-Length': data ? Buffer.byteLength(data) : 0
            }
        };
        
        const req = http.request(options, (res) => {
            let responseData = '';
            
            res.on('data', (chunk) => {
                responseData += chunk;
            });
            
            res.on('end', () => {
                if (res.statusCode >= 200 && res.statusCode < 300) {
                    resolve(responseData);
                } else {
                    reject(new Error(`HTTP ${res.statusCode}: ${responseData}`));
                }
            });
        });
        
        req.on('error', (err) => {
            reject(err);
        });
        
        if (data) {
            req.write(data);
        }
        
        req.end();
    });
}

// Run the test
testHTTPBufferManager().catch(console.error);