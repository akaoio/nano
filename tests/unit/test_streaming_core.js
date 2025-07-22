import { exec } from 'child_process';
import { promisify } from 'util';
import { promises as fs } from 'fs';
import path from 'path';

const execAsync = promisify(exec);

// RKLLMTestClient class for testing streaming infrastructure
class RKLLMTestClient {
    constructor() {
        this.serverProcess = null;
        this.serverPort = 8082; // HTTP port for testing
        this.serverUrl = `http://localhost:${this.serverPort}`;
        this.requestId = 1;
    }
    
    async startServer() {
        console.log('Starting RKLLM MCP server...');
        try {
            // Build the server first
            await this.buildServer();
            
            // Start server in HTTP mode
            this.serverProcess = exec(`./build/mcp_server --transport http --port ${this.serverPort}`, {
                cwd: '/home/x/Projects/nano'
            });
            
            // Log server output for debugging
            this.serverProcess.stdout.on('data', (data) => {
                console.log(`[SERVER] ${data.toString().trim()}`);
            });
            
            this.serverProcess.stderr.on('data', (data) => {
                console.error(`[SERVER ERROR] ${data.toString().trim()}`);
            });
            
        } catch (error) {
            console.error('Failed to start server:', error);
            throw error;
        }
    }
    
    async buildServer() {
        console.log('Building server...');
        try {
            const { stdout, stderr } = await execAsync('npm run build', {
                cwd: '/home/x/Projects/nano'
            });
            console.log('Build output:', stdout);
            if (stderr) console.error('Build warnings:', stderr);
        } catch (error) {
            console.error('Build failed:', error);
            throw error;
        }
    }
    
    async waitForServer() {
        console.log('Waiting for server to be ready...');
        let attempts = 0;
        const maxAttempts = 30; // 15 seconds
        
        while (attempts < maxAttempts) {
            try {
                // Try a simple request to test server readiness
                const response = await fetch(this.serverUrl, {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/json' },
                    body: JSON.stringify({
                        jsonrpc: "2.0",
                        id: 1,
                        method: "stream_manager_status",
                        params: {}
                    })
                });
                if (response.status === 200 || response.status === 500) { // Accept any response
                    console.log('Server is ready');
                    return;
                }
            } catch (error) {
                // Server not ready yet
            }
            
            attempts++;
            await new Promise(resolve => setTimeout(resolve, 500));
        }
        
        throw new Error('Server failed to start within timeout period');
    }
    
    async stopServer() {
        if (this.serverProcess) {
            console.log('Stopping server...');
            this.serverProcess.kill();
            this.serverProcess = null;
            
            // Wait a moment for clean shutdown
            await new Promise(resolve => setTimeout(resolve, 1000));
        }
    }
    
    async httpRequest(method, params = {}) {
        const requestBody = {
            jsonrpc: "2.0",
            id: this.requestId++,
            method: method,
            params: params
        };
        
        console.log(`Sending request: ${method}`, JSON.stringify(requestBody, null, 2));
        
        try {
            const response = await fetch(this.serverUrl, {
                method: 'POST',
                headers: {
                    'Content-Type': 'application/json'
                },
                body: JSON.stringify(requestBody)
            });
            
            if (!response.ok) {
                throw new Error(`HTTP ${response.status}: ${response.statusText}`);
            }
            
            const result = await response.json();
            console.log(`Response from ${method}:`, JSON.stringify(result, null, 2));
            return result;
            
        } catch (error) {
            console.error(`Request failed for ${method}:`, error);
            throw error;
        }
    }
}

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

// Main test suite
async function runTests() {
    console.log('🚀 Starting RKLLM MCP Streaming Core Tests');
    console.log('==========================================');
    
    await describe('Core Streaming Infrastructure', async () => {
        let client;
        
        // Setup
        client = new RKLLMTestClient();
        await client.startServer();
        await client.waitForServer();
        
        try {
            await it('should initialize stream manager successfully', async () => {
                const response = await client.httpRequest('stream_manager_status', {});
                expect(response.error).toBeUndefined();
                expect(response.result.initialized).toBeTrue();
                expect(response.result.active_sessions).toEqual(0);
            });
            
            await it('should create streaming session for rkllm_run_async', async () => {
                const response = await client.httpRequest('rkllm_run_async', {
                    input_type: 0,
                    prompt_input: 'Test streaming',
                    stream: true
                });
                
                // Note: This may fail if RKLLM model is not available, but should at least
                // show that streaming session creation is attempted
                console.log('rkllm_run_async response:', response);
                
                // Check if we get either success or a meaningful error
                if (response.error) {
                    // Acceptable errors: model not found, RKLLM init failed, etc.
                    // These indicate the streaming system is working but RKLLM is not available
                    const errorMsg = response.error.message || '';
                    const isAcceptableError = errorMsg.includes('model') || 
                                              errorMsg.includes('RKLLM') ||
                                              errorMsg.includes('init');
                    if (isAcceptableError) {
                        console.log('  ℹ️  Acceptable error (RKLLM not available for testing):', errorMsg);
                    } else {
                        throw new Error(`Unexpected error: ${errorMsg}`);
                    }
                } else {
                    // Success case
                    expect(response.result.session_id).toBeA('string');
                    expect(response.result.status).toEqual('streaming_started');
                }
            });
            
            await it('should handle streaming callbacks properly', async () => {
                const response = await client.httpRequest('test_streaming_callback', {
                    mock_tokens: ['Hello', ' world', '!'],
                    delay_ms: 100
                });
                
                expect(response.error).toBeUndefined();
                expect(response.result.callback_registered).toBeTrue();
                expect(response.result.tokens_processed).toEqual(3);
                expect(response.result.session_id).toBeA('string');
            });
            
        } finally {
            // Cleanup
            await client.stopServer();
        }
    });
    
    console.log('\n🎉 All streaming core tests completed successfully!');
}

// Run tests if this is the main module
if (import.meta.url === `file://${process.argv[1]}`) {
    runTests().catch(error => {
        console.error('\n💥 Test suite failed:', error);
        process.exit(1);
    });
}

export { RKLLMTestClient, runTests };