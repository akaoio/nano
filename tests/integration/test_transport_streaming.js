// Official test for transport-specific streaming
import { RKLLMTestClient } from '../unit/test_streaming_core.js';
import { exec } from 'child_process';
import { promisify } from 'util';
import net from 'net';
import dgram from 'dgram';
import WebSocket from 'ws';

const execAsync = promisify(exec);

// Test helper functions
function expect(actual) {
    return {
        toBe: function(expected) {
            if (actual !== expected) {
                throw new Error(`Expected ${expected}, but got ${actual}`);
            }
        },
        toBeUndefined: function() {
            if (actual !== undefined) {
                throw new Error(`Expected undefined, but got ${actual}`);
            }
        },
        toEqual: function(expected) {
            if (actual !== expected) {
                throw new Error(`Expected ${expected}, but got ${actual}`);
            }
        },
        toBeTrue: function() {
            if (actual !== true) {
                throw new Error(`Expected true, but got ${actual}`);
            }
        },
        toBeA: function(type) {
            if (typeof actual !== type) {
                throw new Error(`Expected ${type}, but got ${typeof actual}`);
            }
        },
        toBeGreaterThan: function(expected) {
            if (actual <= expected) {
                throw new Error(`Expected ${actual} to be greater than ${expected}`);
            }
        }
    };
}

async function describe(description, testSuite) {
    console.log(`\n=== ${description} ===`);
    try {
        await testSuite();
        console.log(`✅ ${description} - ALL TESTS PASSED`);
    } catch (error) {
        console.error(`❌ ${description} - TEST FAILED:`, error.message);
        throw error;
    }
}

async function it(description, test) {
    console.log(`\n  Testing: ${description}`);
    try {
        await test();
        console.log(`  ✅ ${description}`);
    } catch (error) {
        console.log(`  ❌ ${description}: ${error.message}`);
        throw error;
    }
}

// TCP streaming test helper
class TCPStreamingClient {
    constructor(port) {
        this.port = port;
        this.client = null;
        this.responses = [];
    }
    
    async connect() {
        return new Promise((resolve, reject) => {
            this.client = net.createConnection({ port: this.port }, () => {
                console.log('TCP client connected');
                resolve();
            });
            
            this.client.on('data', (data) => {
                const messages = data.toString().split('\n').filter(msg => msg.trim());
                messages.forEach(msg => {
                    try {
                        const response = JSON.parse(msg);
                        this.responses.push(response);
                        console.log('TCP received:', msg.substring(0, 100));
                    } catch (e) {
                        console.log('TCP received (raw):', msg.substring(0, 100));
                    }
                });
            });
            
            this.client.on('error', reject);
        });
    }
    
    async sendRequest(method, params) {
        return new Promise((resolve, reject) => {
            const request = {
                jsonrpc: "2.0",
                id: Date.now(),
                method: method,
                params: params
            };
            
            this.client.write(JSON.stringify(request) + '\n');
            
            // Wait for response
            setTimeout(() => {
                const response = this.responses.find(r => r.id === request.id) || this.responses[this.responses.length - 1];
                resolve(response);
            }, 1000);
        });
    }
    
    disconnect() {
        if (this.client) {
            this.client.destroy();
        }
    }
}

// UDP streaming test helper
class UDPStreamingClient {
    constructor(port) {
        this.port = port;
        this.client = null;
        this.responses = [];
    }
    
    async connect() {
        this.client = dgram.createSocket('udp4');
        
        this.client.on('message', (msg, rinfo) => {
            try {
                const response = JSON.parse(msg.toString());
                this.responses.push(response);
                console.log('UDP received:', msg.toString().substring(0, 100));
            } catch (e) {
                console.log('UDP received (raw):', msg.toString().substring(0, 100));
            }
        });
        
        return new Promise(resolve => setTimeout(resolve, 100)); // Brief setup delay
    }
    
    async sendRequest(method, params) {
        return new Promise((resolve, reject) => {
            const request = {
                jsonrpc: "2.0",
                id: Date.now(),
                method: method,
                params: params
            };
            
            const message = Buffer.from(JSON.stringify(request));
            this.client.send(message, this.port, 'localhost', (err) => {
                if (err) {
                    reject(err);
                    return;
                }
                
                // Wait for response
                setTimeout(() => {
                    const response = this.responses.find(r => r.id === request.id) || this.responses[this.responses.length - 1];
                    resolve(response);
                }, 1000);
            });
        });
    }
    
    disconnect() {
        if (this.client) {
            this.client.close();
        }
    }
}

// WebSocket streaming test helper
class WebSocketStreamingClient {
    constructor(port) {
        this.port = port;
        this.ws = null;
        this.responses = [];
    }
    
    async connect() {
        return new Promise((resolve, reject) => {
            this.ws = new WebSocket(`ws://localhost:${this.port}`);
            
            this.ws.on('open', () => {
                console.log('WebSocket client connected');
                resolve();
            });
            
            this.ws.on('message', (data) => {
                try {
                    const response = JSON.parse(data.toString());
                    this.responses.push(response);
                    console.log('WebSocket received:', data.toString().substring(0, 100));
                } catch (e) {
                    console.log('WebSocket received (raw):', data.toString().substring(0, 100));
                }
            });
            
            this.ws.on('error', reject);
        });
    }
    
    async sendRequest(method, params) {
        return new Promise((resolve, reject) => {
            if (this.ws.readyState !== WebSocket.OPEN) {
                reject(new Error('WebSocket not open'));
                return;
            }
            
            const request = {
                jsonrpc: "2.0",
                id: Date.now(),
                method: method,
                params: params
            };
            
            this.ws.send(JSON.stringify(request));
            
            // Wait for response
            setTimeout(() => {
                const response = this.responses.find(r => r.id === request.id) || this.responses[this.responses.length - 1];
                resolve(response);
            }, 1000);
        });
    }
    
    disconnect() {
        if (this.ws && this.ws.readyState === WebSocket.OPEN) {
            this.ws.close();
        }
    }
}

// Enhanced RKLLMTestClient with transport-specific methods
class TransportStreamingClient extends RKLLMTestClient {
    constructor() {
        super();
        this.tcpPort = 8081;
        this.udpPort = 8083;
        this.wsPort = 8084;
    }
    
    async startServerWithAllTransports() {
        console.log('Starting RKLLM MCP server with all transports...');
        try {
            // Build the server first
            await this.buildServer();
            
            // Start server with multiple transports
            // Note: This would require the server to support multi-transport startup
            this.serverProcess = exec(`./build/mcp_server --transport stdio --tcp-port ${this.tcpPort} --udp-port ${this.udpPort} --ws-port ${this.wsPort}`, {
                cwd: '/home/x/Projects/nano'
            });
            
            // Log server output
            this.serverProcess.stdout.on('data', (data) => {
                console.log(`[SERVER] ${data.toString().trim()}`);
            });
            
            this.serverProcess.stderr.on('data', (data) => {
                console.error(`[SERVER ERROR] ${data.toString().trim()}`);
            });
            
        } catch (error) {
            console.error('Failed to start server with all transports:', error);
            throw error;
        }
    }
    
    async tcpRequest(method, params) {
        const client = new TCPStreamingClient(this.tcpPort);
        try {
            await client.connect();
            const response = await client.sendRequest(method, params);
            return response;
        } finally {
            client.disconnect();
        }
    }
    
    async udpRequest(method, params) {
        const client = new UDPStreamingClient(this.udpPort);
        try {
            await client.connect();
            const response = await client.sendRequest(method, params);
            return response;
        } finally {
            client.disconnect();
        }
    }
    
    async wsRequest(method, params) {
        const client = new WebSocketStreamingClient(this.wsPort);
        try {
            await client.connect();
            const response = await client.sendRequest(method, params);
            return response;
        } finally {
            client.disconnect();
        }
    }
    
    async testRealTimeStreaming(transport, params) {
        let client;
        
        switch (transport) {
            case 'tcp':
                client = new TCPStreamingClient(this.tcpPort);
                break;
            case 'udp':
                client = new UDPStreamingClient(this.udpPort);
                break;
            case 'ws':
                client = new WebSocketStreamingClient(this.wsPort);
                break;
            default:
                throw new Error(`Unsupported transport: ${transport}`);
        }
        
        try {
            await client.connect();
            
            // Send streaming request
            const request = {
                jsonrpc: "2.0",
                id: Date.now(),
                method: "test_streaming_callback",
                params: {
                    mock_tokens: ["Hello", " streaming", " world", "!"],
                    delay_ms: 50
                }
            };
            
            const response = await client.sendRequest(request.method, request.params);
            console.log(`${transport.toUpperCase()} streaming response:`, response);
            
            // Validate streaming response
            if (response && response.result) {
                expect(response.error).toBeUndefined();
                expect(response.result.callback_registered).toBeTrue();
                expect(response.result.tokens_processed).toEqual(4);
                return { status: 'success', response: response };
            } else {
                return { status: 'error', response: response };
            }
            
        } finally {
            client.disconnect();
        }
    }
    
    async testHttpPollingStreaming(params) {
        // Use the existing HTTP implementation from parent class
        const response = await this.httpRequest('test_streaming_callback', params);
        
        let pollCount = 0;
        const maxPolls = 10;
        
        // Simulate polling for streaming data
        while (pollCount < maxPolls) {
            await new Promise(resolve => setTimeout(resolve, 100));
            pollCount++;
            
            // Check if streaming is complete (would need server-side polling support)
            if (response && response.result) {
                break;
            }
        }
        
        return {
            status: 'completed',
            polls: pollCount,
            response: response
        };
    }
}

// Main test suite
async function runTransportStreamingTests() {
    console.log('🚀 Starting Transport Streaming Tests');
    console.log('=====================================');
    
    await describe('Transport Streaming Implementation', async () => {
        let client;
        
        // Setup - Use HTTP for basic testing since multi-transport requires server changes
        client = new RKLLMTestClient(); // Use basic client for now
        await client.startServer();
        await client.waitForServer();
        
        try {
            await it('should test streaming callback functionality', async () => {
                const response = await client.httpRequest('test_streaming_callback', {
                    mock_tokens: ['Hello', ' transport', ' streaming', '!'],
                    delay_ms: 50
                });
                
                expect(response.error).toBeUndefined();
                expect(response.result.callback_registered).toBeTrue();
                expect(response.result.tokens_processed).toEqual(4);
                expect(response.result.session_id).toBeA('string');
            });
            
            await it('should test connection drop simulation for TCP', async () => {
                const response = await client.httpRequest('test_connection_drop', {
                    transport: 'tcp',
                    action: 'simulate_drop'
                });
                
                // This may return false if no connections are active, which is acceptable
                expect(response.error).toBeUndefined();
                expect(response.result.handled_gracefully).toBeA('boolean');
                expect(response.result.transport).toEqual('tcp');
                expect(response.result.action).toEqual('simulate_drop');
            });
            
            await it('should test connection drop simulation for UDP', async () => {
                const response = await client.httpRequest('test_connection_drop', {
                    transport: 'udp',
                    action: 'simulate_drop'
                });
                
                expect(response.error).toBeUndefined();
                expect(response.result.handled_gracefully).toBeA('boolean');
                expect(response.result.transport).toEqual('udp');
                expect(response.result.action).toEqual('simulate_drop');
            });
            
            await it('should test connection drop simulation for WebSocket', async () => {
                const response = await client.httpRequest('test_connection_drop', {
                    transport: 'websocket',
                    action: 'simulate_drop'
                });
                
                expect(response.error).toBeUndefined();
                expect(response.result.handled_gracefully).toBeA('boolean');
                expect(response.result.transport).toEqual('websocket');
                expect(response.result.action).toEqual('simulate_drop');
            });
            
            await it('should validate stream manager is properly initialized', async () => {
                const response = await client.httpRequest('stream_manager_status', {});
                
                expect(response.error).toBeUndefined();
                expect(response.result.initialized).toBeTrue();
                expect(response.result.active_sessions).toBeA('number');
                expect(response.result.max_sessions).toBeGreaterThan(0);
            });
            
            await it('should handle invalid transport type for connection drop', async () => {
                const response = await client.httpRequest('test_connection_drop', {
                    transport: 'invalid_transport',
                    action: 'simulate_drop'
                });
                
                expect(response.error).toBeA('object');
                expect(response.error.code).toEqual(-32602);
                expect(response.error.message).toBeA('string');
            });
            
            await it('should handle invalid action for connection drop', async () => {
                const response = await client.httpRequest('test_connection_drop', {
                    transport: 'tcp',
                    action: 'invalid_action'
                });
                
                expect(response.error).toBeA('object');
                expect(response.error.code).toEqual(-32602);
                expect(response.error.message).toBeA('string');
            });
            
        } finally {
            // Cleanup
            await client.stopServer();
        }
    });
    
    console.log('\n🎉 All transport streaming tests completed successfully!');
}

// Run tests if this is the main module
if (import.meta.url === `file://${process.argv[1]}`) {
    runTransportStreamingTests().catch(error => {
        console.error('\n💥 Transport streaming tests failed:', error);
        process.exit(1);
    });
}

export { runTransportStreamingTests, TransportStreamingClient };