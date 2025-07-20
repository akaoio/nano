#!/usr/bin/env node

/**
 * Test STDIO transport specifically
 */

const { spawn } = require('child_process');
const path = require('path');

async function testSTDIO() {
    console.log('🧪 Testing STDIO Transport');
    console.log('===========================');
    
    const serverPath = path.join(__dirname, '..', 'build', 'mcp_server');
    const serverProcess = spawn(serverPath, ['--disable-tcp', '--disable-udp', '--disable-http', '--disable-ws'], {
        stdio: ['pipe', 'pipe', 'pipe']
    });
    
    // Wait for server to start
    await new Promise(resolve => {
        serverProcess.stderr.on('data', (data) => {
            console.log(`[SERVER] ${data.toString().trim()}`);
            if (data.toString().includes('listening for connections')) {
                setTimeout(resolve, 1000);
            }
        });
    });
    
    console.log('\n📡 Sending rkllm_get_constants request via STDIO...');
    
    const request = JSON.stringify({
        jsonrpc: "2.0",
        id: 1,
        method: "rkllm_get_constants",
        params: {}
    }) + '\n';
    
    return new Promise((resolve) => {
        let responseBuffer = '';
        let timeout;
        
        const dataHandler = (data) => {
            const output = data.toString();
            responseBuffer += output;
            
            // Look for JSON response
            try {
                const lines = responseBuffer.split('\n');
                for (const line of lines) {
                    if (line.trim().startsWith('{') && line.includes('jsonrpc')) {
                        clearTimeout(timeout);
                        const response = JSON.parse(line.trim());
                        console.log('✅ STDIO Response received!');
                        console.log('📋 Constants available:');
                        
                        if (response.result) {
                            Object.entries(response.result).forEach(([category, values]) => {
                                if (typeof values === 'object' && values !== null) {
                                    console.log(`   ${category}: ${Object.keys(values).length} items`);
                                    if (category === 'CPU_MASKS') {
                                        console.log(`     ${Object.entries(values).map(([k,v]) => `${k}=0x${v.toString(16)}`).join(', ')}`);
                                    }
                                }
                            });
                        }
                        
                        serverProcess.kill('SIGTERM');
                        resolve(true);
                        return;
                    }
                }
            } catch (e) {
                // Not valid JSON yet, continue
            }
        };
        
        serverProcess.stdout.on('data', dataHandler);
        
        // Send the request
        console.log('📤 Sending request...');
        serverProcess.stdin.write(request);
        
        timeout = setTimeout(() => {
            console.log('❌ STDIO request timeout');
            console.log('Raw response buffer:', responseBuffer);
            serverProcess.kill('SIGTERM');
            resolve(false);
        }, 10000);
    });
}

testSTDIO().then(success => {
    console.log(success ? '\n🎉 STDIO transport working!' : '\n❌ STDIO transport failed');
    process.exit(success ? 0 : 1);
}).catch(console.error);