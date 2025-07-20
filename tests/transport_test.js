#!/usr/bin/env node

/**
 * Multi-Transport Test for RKLLM MCP Server
 * 
 * Tests all available transports: STDIO, TCP, UDP, HTTP, WebSocket
 */

const http = require('http');
const net = require('net');
const dgram = require('dgram');
const { spawn } = require('child_process');
const path = require('path');

class MultiTransportTester {
    constructor() {
        this.serverProcess = null;
        this.results = {
            stdio: { status: 'pending', error: null, response: null },
            tcp: { status: 'pending', error: null, response: null },
            udp: { status: 'pending', error: null, response: null },
            http: { status: 'pending', error: null, response: null },
            websocket: { status: 'pending', error: null, response: null }
        };
    }

    // Start server with all transports enabled
    async startServer() {
        return new Promise((resolve, reject) => {
            console.log('🚀 Starting MCP Server with all transports...');
            
            const serverPath = path.join(__dirname, '..', 'build', 'mcp_server');
            
            // Start with all default transports enabled and --force to kill previous runs
            this.serverProcess = spawn(serverPath, ['--force'], {
                stdio: ['pipe', 'pipe', 'pipe']  // We'll use STDIO transport too
            });

            let serverReady = false;
            let outputBuffer = '';

            this.serverProcess.stdout.on('data', (data) => {
                const output = data.toString();
                outputBuffer += output;
                console.log(`[SERVER-OUT] ${output.trim()}`);
                
                if (output.includes('MCP Server started successfully') || output.includes('listening for connections')) {
                    if (!serverReady) {
                        serverReady = true;
                        console.log('✅ Server started, waiting for transports to initialize...');
                        setTimeout(() => resolve(), 3000);  // Give time for all transports
                    }
                }
            });

            this.serverProcess.stderr.on('data', (data) => {
                const output = data.toString();
                outputBuffer += output;
                console.log(`[SERVER-ERR] ${output.trim()}`);
                
                if (output.includes('listening for connections') && !serverReady) {
                    serverReady = true;
                    console.log('✅ Server started, waiting for transports to initialize...');
                    setTimeout(() => resolve(), 3000);
                }
            });

            this.serverProcess.on('error', (error) => {
                console.error('❌ Failed to start server:', error.message);
                reject(error);
            });

            this.serverProcess.on('exit', (code) => {
                console.log(`[SERVER] Process exited with code ${code}`);
            });

            // Timeout
            setTimeout(() => {
                if (!serverReady) {
                    console.error('❌ Server startup timeout');
                    console.log('Server output:', outputBuffer);
                    reject(new Error('Server startup timeout'));
                }
            }, 15000);
        });
    }

    // Test STDIO transport
    async testSTDIO() {
        console.log('\n📡 Testing STDIO Transport...');
        
        return new Promise((resolve) => {
            if (!this.serverProcess) {
                this.results.stdio = { status: 'failed', error: 'Server not started', response: null };
                resolve();
                return;
            }

            const request = JSON.stringify({
                jsonrpc: "2.0",
                id: 1,
                method: "rkllm_get_constants",
                params: {}
            }) + '\n';

            let responseBuffer = '';
            let timeout;

            const dataHandler = (data) => {
                responseBuffer += data.toString();
                
                // Look for JSON response lines (lines that start with '{' and contain 'jsonrpc')
                const lines = responseBuffer.split('\n');
                for (const line of lines) {
                    const trimmedLine = line.trim();
                    if (trimmedLine.startsWith('{') && trimmedLine.includes('jsonrpc')) {
                        clearTimeout(timeout);
                        try {
                            const response = JSON.parse(trimmedLine);
                            console.log('✅ STDIO: Success');
                            this.results.stdio = { status: 'success', error: null, response };
                        } catch (e) {
                            console.log('❌ STDIO: Invalid JSON response');
                            this.results.stdio = { status: 'failed', error: 'Invalid JSON', response: trimmedLine };
                        }
                        this.serverProcess.stdout.off('data', dataHandler);
                        resolve();
                        return;
                    }
                }
            };

            this.serverProcess.stdout.on('data', dataHandler);

            // Send request via STDIN
            this.serverProcess.stdin.write(request);

            timeout = setTimeout(() => {
                console.log('❌ STDIO: Timeout');
                this.results.stdio = { status: 'failed', error: 'Timeout', response: null };
                this.serverProcess.stdout.off('data', dataHandler);
                resolve();
            }, 10000);
        });
    }

    // Test TCP transport
    async testTCP() {
        console.log('\n📡 Testing TCP Transport (port 8080)...');
        
        return new Promise((resolve) => {
            const client = new net.Socket();
            const request = JSON.stringify({
                jsonrpc: "2.0",
                id: 2,
                method: "rkllm_get_constants",
                params: {}
            }) + '\n';

            let timeout = setTimeout(() => {
                console.log('❌ TCP: Timeout');
                this.results.tcp = { status: 'failed', error: 'Timeout', response: null };
                client.destroy();
                resolve();
            }, 10000);

            client.connect(8080, '127.0.0.1', () => {
                console.log('📡 TCP: Connected, sending request...');
                client.write(request);
            });

            let responseBuffer = '';
            client.on('data', (data) => {
                responseBuffer += data.toString();
                if (responseBuffer.includes('\n')) {
                    clearTimeout(timeout);
                    try {
                        const response = JSON.parse(responseBuffer.trim());
                        console.log('✅ TCP: Success');
                        this.results.tcp = { status: 'success', error: null, response };
                    } catch (e) {
                        console.log('❌ TCP: Invalid JSON response');
                        this.results.tcp = { status: 'failed', error: 'Invalid JSON', response: responseBuffer };
                    }
                    client.destroy();
                    resolve();
                }
            });

            client.on('error', (error) => {
                clearTimeout(timeout);
                console.log(`❌ TCP: Connection error - ${error.message}`);
                this.results.tcp = { status: 'failed', error: error.message, response: null };
                resolve();
            });
        });
    }

    // Test UDP transport
    async testUDP() {
        console.log('\n📡 Testing UDP Transport (port 8081)...');
        
        return new Promise((resolve) => {
            const client = dgram.createSocket('udp4');
            const request = JSON.stringify({
                jsonrpc: "2.0",
                id: 3,
                method: "rkllm_get_constants", 
                params: {}
            });

            let timeout = setTimeout(() => {
                console.log('❌ UDP: Timeout');
                this.results.udp = { status: 'failed', error: 'Timeout', response: null };
                client.close();
                resolve();
            }, 10000);

            client.on('message', (msg, rinfo) => {
                clearTimeout(timeout);
                try {
                    const response = JSON.parse(msg.toString());
                    console.log('✅ UDP: Success');
                    this.results.udp = { status: 'success', error: null, response };
                } catch (e) {
                    console.log('❌ UDP: Invalid JSON response');
                    this.results.udp = { status: 'failed', error: 'Invalid JSON', response: msg.toString() };
                }
                client.close();
                resolve();
            });

            client.on('error', (error) => {
                clearTimeout(timeout);
                console.log(`❌ UDP: Error - ${error.message}`);
                this.results.udp = { status: 'failed', error: error.message, response: null };
                client.close();
                resolve();
            });

            console.log('📡 UDP: Sending request...');
            client.send(request, 8081, '127.0.0.1', (error) => {
                if (error) {
                    clearTimeout(timeout);
                    console.log(`❌ UDP: Send error - ${error.message}`);
                    this.results.udp = { status: 'failed', error: error.message, response: null };
                    client.close();
                    resolve();
                }
            });
        });
    }

    // Test HTTP transport
    async testHTTP() {
        console.log('\n📡 Testing HTTP Transport (port 8082)...');
        
        return new Promise((resolve) => {
            const data = JSON.stringify({
                jsonrpc: "2.0",
                id: 4,
                method: "rkllm_get_constants",
                params: {}
            });
            
            const options = {
                hostname: '127.0.0.1',
                port: 8082,
                path: '/',
                method: 'POST',
                headers: {
                    'Content-Type': 'application/json',
                    'Content-Length': Buffer.byteLength(data)
                },
                timeout: 10000
            };

            const req = http.request(options, (res) => {
                let body = '';
                res.on('data', (chunk) => body += chunk);
                res.on('end', () => {
                    try {
                        const response = JSON.parse(body);
                        console.log('✅ HTTP: Success');
                        this.results.http = { status: 'success', error: null, response };
                    } catch (e) {
                        console.log('❌ HTTP: Invalid JSON response');
                        this.results.http = { status: 'failed', error: 'Invalid JSON', response: body };
                    }
                    resolve();
                });
            });

            req.on('error', (error) => {
                console.log(`❌ HTTP: Request error - ${error.message}`);
                this.results.http = { status: 'failed', error: error.message, response: null };
                resolve();
            });

            req.on('timeout', () => {
                console.log('❌ HTTP: Timeout');
                this.results.http = { status: 'failed', error: 'Timeout', response: null };
                req.destroy();
                resolve();
            });

            console.log('📡 HTTP: Sending request...');
            req.write(data);
            req.end();
        });
    }

    // Test WebSocket transport with proper WebSocket protocol
    async testWebSocket() {
        console.log('\n📡 Testing WebSocket Transport (port 8083)...');
        
        return new Promise((resolve) => {
            try {
                const WebSocket = require('ws');
                
                const ws = new WebSocket('ws://127.0.0.1:8083/');
                
                let timeout = setTimeout(() => {
                    console.log('❌ WebSocket: Timeout');
                    this.results.websocket = { status: 'failed', error: 'Timeout', response: null };
                    ws.close();
                    resolve();
                }, 10000);

                ws.on('open', () => {
                    console.log('📡 WebSocket: Connected, sending request...');
                    
                    const request = JSON.stringify({
                        jsonrpc: "2.0",
                        id: 5,
                        method: "rkllm_get_constants",
                        params: {}
                    });
                    
                    ws.send(request);
                });

                ws.on('message', (data) => {
                    clearTimeout(timeout);
                    try {
                        const response = JSON.parse(data.toString());
                        console.log('✅ WebSocket: Success');
                        this.results.websocket = { status: 'success', error: null, response };
                    } catch (e) {
                        console.log('❌ WebSocket: Invalid JSON response');
                        this.results.websocket = { status: 'failed', error: 'Invalid JSON', response: data.toString() };
                    }
                    ws.close();
                    resolve();
                });

                ws.on('error', (error) => {
                    clearTimeout(timeout);
                    console.log(`❌ WebSocket: Error - ${error.message}`);
                    this.results.websocket = { status: 'failed', error: error.message, response: null };
                    resolve();
                });

                ws.on('close', () => {
                    // Connection closed, check if we got a result
                    if (this.results.websocket.status === 'pending') {
                        clearTimeout(timeout);
                        console.log('❌ WebSocket: Connection closed without response');
                        this.results.websocket = { status: 'failed', error: 'Connection closed', response: null };
                        resolve();
                    }
                });

            } catch (error) {
                console.log(`❌ WebSocket: Failed to load ws library - ${error.message}`);
                this.results.websocket = { status: 'failed', error: `ws library error: ${error.message}`, response: null };
                resolve();
            }
        });
    }

    // Stop server
    async stopServer() {
        if (this.serverProcess) {
            console.log('\n🛑 Stopping MCP Server...');
            this.serverProcess.kill('SIGTERM');
            
            await new Promise(resolve => {
                setTimeout(() => {
                    if (this.serverProcess && !this.serverProcess.killed) {
                        this.serverProcess.kill('SIGKILL');
                    }
                    this.serverProcess = null;
                    console.log('✅ Server stopped');
                    resolve();
                }, 3000);
            });
        }
    }

    // Generate report
    generateReport() {
        console.log('\n📊 Transport Test Results');
        console.log('==========================');
        
        let totalTests = 0;
        let successfulTests = 0;
        
        Object.entries(this.results).forEach(([transport, result]) => {
            totalTests++;
            const status = result.status === 'success' ? '✅' : 
                          result.status === 'partial' ? '🟡' : '❌';
            
            console.log(`${status} ${transport.toUpperCase()}: ${result.status}`);
            
            if (result.error) {
                console.log(`   Error: ${result.error}`);
            }
            
            if (result.status === 'success') {
                successfulTests++;
                if (result.response && result.response.result) {
                    const constants = result.response.result;
                    const cpuMasks = constants.CPU_MASKS ? Object.keys(constants.CPU_MASKS).length : 0;
                    const callStates = constants.LLM_CALL_STATES ? Object.keys(constants.LLM_CALL_STATES).length : 0;
                    console.log(`   Constants: ${cpuMasks} CPU masks, ${callStates} call states`);
                }
            } else if (result.status === 'partial') {
                successfulTests += 0.5;
            }
        });
        
        console.log(`\n📈 Summary:`);
        console.log(`   Total Transports: ${totalTests}`);
        console.log(`   Successful: ${successfulTests}`);
        console.log(`   Success Rate: ${((successfulTests / totalTests) * 100).toFixed(1)}%`);
        
        // Show which constants are available
        const successfulResult = Object.values(this.results).find(r => r.status === 'success' && r.response);
        if (successfulResult && successfulResult.response.result) {
            console.log('\n🔧 Available RKLLM Constants:');
            const constants = successfulResult.response.result;
            Object.entries(constants).forEach(([category, values]) => {
                if (typeof values === 'object' && values !== null) {
                    console.log(`   ${category}: ${Object.keys(values).length} items`);
                }
            });
        }
    }

    // Main test runner
    async run() {
        console.log('🧪 Multi-Transport RKLLM Test');
        console.log('==============================');
        
        try {
            // Start server
            await this.startServer();
            
            // Test all transports
            await this.testSTDIO();
            await this.testTCP(); 
            await this.testUDP();
            await this.testHTTP();
            await this.testWebSocket();
            
            // Generate report
            this.generateReport();
            
        } catch (error) {
            console.error('❌ Test execution failed:', error.message);
        } finally {
            await this.stopServer();
        }
    }
}

// Main execution
if (require.main === module) {
    const tester = new MultiTransportTester();
    
    // Handle graceful shutdown
    process.on('SIGINT', async () => {
        console.log('\n🛑 Received interrupt signal...');
        await tester.stopServer();
        process.exit(0);
    });
    
    tester.run().catch(console.error);
}

module.exports = MultiTransportTester;