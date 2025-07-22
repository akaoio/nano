#!/usr/bin/env node

/**
 * Node.js Streaming Client for RKLLM MCP Server
 * Tests real-time streaming across different transports
 */

const net = require('net');
const dgram = require('dgram');
const http = require('http');

class MCPStreamingClient {
    constructor() {
        this.requestId = 1;
    }

    // Create JSON-RPC 2.0 request
    createRequest(method, params = {}) {
        return {
            jsonrpc: '2.0',
            id: this.requestId++,
            method: method,
            params: params
        };
    }

    // Test TCP streaming
    async testTCPStreaming(port = 8081) {
        console.log('\n🚗 Testing TCP Streaming...');
        
        return new Promise((resolve, reject) => {
            const client = new net.Socket();
            const chunks = [];
            
            client.connect(port, '127.0.0.1', () => {
                console.log('✅ Connected to TCP server');
                
                // Send streaming request
                const request = this.createRequest('rkllm_run_async', {
                    input: {
                        input_type: 0,
                        prompt_input: 'Hello, how are you?'
                    },
                    infer_params: {
                        mode: 0,
                        keep_history: 1
                    }
                });
                
                const requestString = JSON.stringify(request) + '\n';
                console.log('📤 Sending request:', requestString);
                client.write(requestString);
            });
            
            client.on('data', (data) => {
                const lines = data.toString().split('\n').filter(line => line.trim());
                
                for (const line of lines) {
                    try {
                        const response = JSON.parse(line);
                        console.log('📥 Received:', JSON.stringify(response, null, 2));
                        
                        if (response.result && response.result.chunk) {
                            chunks.push(response.result.chunk);
                            
                            // Check if this is the final chunk
                            if (response.result.chunk.end) {
                                console.log('🏁 Stream completed. Total chunks:', chunks.length);
                                client.end();
                                resolve(chunks);
                                return;
                            }
                        }
                    } catch (e) {
                        console.log('📄 Non-JSON response:', line);
                    }
                }
            });
            
            client.on('close', () => {
                console.log('🔌 TCP connection closed');
            });
            
            client.on('error', (err) => {
                console.error('❌ TCP error:', err.message);
                reject(err);
            });
            
            // Timeout after 10 seconds
            setTimeout(() => {
                client.end();
                resolve(chunks);
            }, 10000);
        });
    }

    // Test UDP streaming
    async testUDPStreaming(port = 8082) {
        console.log('\n🛸 Testing UDP Streaming...');
        
        return new Promise((resolve, reject) => {
            const client = dgram.createSocket('udp4');
            const chunks = [];
            
            client.on('message', (msg, rinfo) => {
                try {
                    const response = JSON.parse(msg.toString());
                    console.log('📥 Received UDP:', JSON.stringify(response, null, 2));
                    
                    if (response.result && response.result.chunk) {
                        chunks.push(response.result.chunk);
                        
                        if (response.result.chunk.end) {
                            console.log('🏁 UDP Stream completed. Total chunks:', chunks.length);
                            client.close();
                            resolve(chunks);
                            return;
                        }
                    }
                } catch (e) {
                    console.log('📄 Non-JSON UDP response:', msg.toString());
                }
            });
            
            const request = this.createRequest('rkllm_run_async', {
                input: {
                    input_type: 0,
                    prompt_input: 'Tell me a short story'
                },
                infer_params: {
                    mode: 0,
                    keep_history: 1
                }
            });
            
            const requestBuffer = Buffer.from(JSON.stringify(request));
            console.log('📤 Sending UDP request');
            
            client.send(requestBuffer, port, '127.0.0.1', (err) => {
                if (err) {
                    console.error('❌ UDP send error:', err.message);
                    reject(err);
                }
            });
            
            // Timeout after 10 seconds
            setTimeout(() => {
                client.close();
                resolve(chunks);
            }, 10000);
        });
    }

    // Test HTTP polling
    async testHTTPPolling(port = 8080) {
        console.log('\n🌐 Testing HTTP Polling...');
        
        const chunks = [];
        
        // Send initial request
        const request = this.createRequest('rkllm_run_async', {
            input: {
                input_type: 0,
                prompt_input: 'What is artificial intelligence?'
            },
            infer_params: {
                mode: 0,
                keep_history: 1
            }
        });
        
        console.log('📤 Sending HTTP request');
        
        return new Promise((resolve, reject) => {
            const postData = JSON.stringify(request);
            
            const options = {
                hostname: '127.0.0.1',
                port: port,
                path: '/',
                method: 'POST',
                headers: {
                    'Content-Type': 'application/json',
                    'Content-Length': Buffer.byteLength(postData)
                }
            };
            
            const req = http.request(options, (res) => {
                let data = '';
                
                res.on('data', (chunk) => {
                    data += chunk;
                });
                
                res.on('end', () => {
                    try {
                        const response = JSON.parse(data);
                        console.log('📥 Initial HTTP response:', JSON.stringify(response, null, 2));
                        
                        // Start polling for chunks
                        this.pollForChunks(port, request.id, chunks, resolve);
                    } catch (e) {
                        console.error('❌ HTTP parse error:', e.message);
                        reject(e);
                    }
                });
            });
            
            req.on('error', (err) => {
                console.error('❌ HTTP error:', err.message);
                reject(err);
            });
            
            req.write(postData);
            req.end();
        });
    }

    // Poll for HTTP chunks
    async pollForChunks(port, originalId, chunks, resolve) {
        const pollRequest = this.createRequest('poll', {});
        
        const poll = () => {
            const postData = JSON.stringify(pollRequest);
            
            const options = {
                hostname: '127.0.0.1',
                port: port,
                path: '/',
                method: 'POST',
                headers: {
                    'Content-Type': 'application/json',
                    'Content-Length': Buffer.byteLength(postData)
                }
            };
            
            const req = http.request(options, (res) => {
                let data = '';
                
                res.on('data', (chunk) => {
                    data += chunk;
                });
                
                res.on('end', () => {
                    try {
                        const response = JSON.parse(data);
                        
                        if (response.result && response.result.chunk) {
                            console.log('📥 HTTP Polling chunk:', JSON.stringify(response.result.chunk));
                            chunks.push(response.result.chunk);
                            
                            if (response.result.chunk.end) {
                                console.log('🏁 HTTP Polling completed. Total chunks:', chunks.length);
                                resolve(chunks);
                                return;
                            }
                        }
                        
                        // Continue polling
                        setTimeout(poll, 500); // Poll every 500ms
                    } catch (e) {
                        console.error('❌ HTTP polling parse error:', e.message);
                        resolve(chunks);
                    }
                });
            });
            
            req.on('error', (err) => {
                console.error('❌ HTTP polling error:', err.message);
                resolve(chunks);
            });
            
            req.write(postData);
            req.end();
        };
        
        // Start polling after a short delay
        setTimeout(poll, 100);
    }

    // Test basic connectivity
    async testConnectivity() {
        console.log('\n🔌 Testing Basic Connectivity...');
        
        const transports = [
            { name: 'HTTP', port: 8080, test: () => this.pingHTTP(8080) },
            { name: 'TCP', port: 8081, test: () => this.pingTCP(8081) },
            { name: 'UDP', port: 8082, test: () => this.pingUDP(8082) }
        ];
        
        for (const transport of transports) {
            try {
                await transport.test();
                console.log(`✅ ${transport.name} (port ${transport.port}): Connected`);
            } catch (err) {
                console.log(`❌ ${transport.name} (port ${transport.port}): ${err.message}`);
            }
        }
    }
    
    async pingHTTP(port) {
        const request = this.createRequest('ping');
        const postData = JSON.stringify(request);
        
        return new Promise((resolve, reject) => {
            const options = {
                hostname: '127.0.0.1',
                port: port,
                path: '/',
                method: 'POST',
                headers: {
                    'Content-Type': 'application/json',
                    'Content-Length': Buffer.byteLength(postData)
                }
            };
            
            const req = http.request(options, (res) => {
                res.on('data', () => {});
                res.on('end', () => resolve());
            });
            
            req.on('error', reject);
            req.write(postData);
            req.end();
            
            setTimeout(() => reject(new Error('timeout')), 2000);
        });
    }
    
    async pingTCP(port) {
        return new Promise((resolve, reject) => {
            const client = new net.Socket();
            
            client.connect(port, '127.0.0.1', () => {
                const request = this.createRequest('ping');
                client.write(JSON.stringify(request) + '\n');
                client.end();
                resolve();
            });
            
            client.on('error', reject);
            setTimeout(() => reject(new Error('timeout')), 2000);
        });
    }
    
    async pingUDP(port) {
        return new Promise((resolve, reject) => {
            const client = dgram.createSocket('udp4');
            const request = this.createRequest('ping');
            const requestBuffer = Buffer.from(JSON.stringify(request));
            
            client.send(requestBuffer, port, '127.0.0.1', (err) => {
                client.close();
                if (err) reject(err);
                else resolve();
            });
            
            setTimeout(() => reject(new Error('timeout')), 2000);
        });
    }
}

// Main test runner
async function main() {
    console.log('🚀 RKLLM MCP Server - Node.js Streaming Client Test');
    console.log('=' .repeat(60));
    
    const client = new MCPStreamingClient();
    
    try {
        // Test basic connectivity first
        await client.testConnectivity();
        
        console.log('\n' + '=' .repeat(60));
        console.log('🌊 Starting Streaming Tests...');
        
        // Test each transport's streaming capability
        const results = {};
        
        try {
            results.tcp = await client.testTCPStreaming();
        } catch (err) {
            console.log('❌ TCP streaming failed:', err.message);
        }
        
        try {
            results.udp = await client.testUDPStreaming();
        } catch (err) {
            console.log('❌ UDP streaming failed:', err.message);
        }
        
        try {
            results.http = await client.testHTTPPolling();
        } catch (err) {
            console.log('❌ HTTP polling failed:', err.message);
        }
        
        // Summary
        console.log('\n' + '=' .repeat(60));
        console.log('📊 STREAMING TEST RESULTS');
        console.log('=' .repeat(60));
        
        for (const [transport, chunks] of Object.entries(results)) {
            if (chunks && chunks.length > 0) {
                console.log(`✅ ${transport.toUpperCase()}: ${chunks.length} chunks received`);
            } else {
                console.log(`❌ ${transport.toUpperCase()}: No chunks received`);
            }
        }
        
        console.log('\n✨ Testing completed!');
        
    } catch (error) {
        console.error('💥 Test suite failed:', error.message);
        process.exit(1);
    }
}

// Run the tests
if (require.main === module) {
    main().catch(console.error);
}

module.exports = MCPStreamingClient;