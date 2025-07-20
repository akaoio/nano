#!/usr/bin/env node

/**
 * RKLLM JavaScript Test Client
 * 
 * Comprehensive test suite for all RKLLM functions, constants, and enums
 * Tests the MCP server architecture with JSON-RPC 2.0 protocol
 * 
 * This client spawns the MCP server process and runs tests concurrently
 */

const http = require('http');
const WebSocket = require('ws');
const net = require('net');
const { spawn } = require('child_process');
const path = require('path');

// =============================================================================
// RKLLM CONSTANTS AND ENUMS (from rkllm.h)
// =============================================================================

// CPU Masks for enabled_cpus_mask parameter
const RKLLM_CPU = {
    CPU0: 1 << 0,  // 0x01
    CPU1: 1 << 1,  // 0x02
    CPU2: 1 << 2,  // 0x04
    CPU3: 1 << 3,  // 0x08
    CPU4: 1 << 4,  // 0x10
    CPU5: 1 << 5,  // 0x20
    CPU6: 1 << 6,  // 0x40
    CPU7: 1 << 7,  // 0x80
};

// LLM Call States
const LLMCallState = {
    RKLLM_RUN_NORMAL: 0,   // The LLM call is in a normal running state
    RKLLM_RUN_WAITING: 1,  // The LLM call is waiting for complete UTF-8 encoded character
    RKLLM_RUN_FINISH: 2,   // The LLM call has finished execution
    RKLLM_RUN_ERROR: 3,    // An error occurred during the LLM call
};

// Input Types
const RKLLMInputType = {
    RKLLM_INPUT_PROMPT: 0,      // Input is a text prompt
    RKLLM_INPUT_TOKEN: 1,       // Input is a sequence of tokens
    RKLLM_INPUT_EMBED: 2,       // Input is an embedding vector
    RKLLM_INPUT_MULTIMODAL: 3,  // Input is multimodal (e.g., text and image)
};

// Inference Modes
const RKLLMInferMode = {
    RKLLM_INFER_GENERATE: 0,                  // The LLM generates text based on input
    RKLLM_INFER_GET_LAST_HIDDEN_LAYER: 1,    // The LLM retrieves the last hidden layer
    RKLLM_INFER_GET_LOGITS: 2,               // The LLM retrieves logits
};

// =============================================================================
// RKLLM FUNCTION DEFINITIONS
// =============================================================================

const RKLLM_FUNCTIONS = [
    'rkllm_createDefaultParam',
    'rkllm_init',
    'rkllm_load_lora',
    'rkllm_load_prompt_cache',
    'rkllm_release_prompt_cache',
    'rkllm_destroy',
    'rkllm_run',
    'rkllm_run_async',
    'rkllm_abort',
    'rkllm_is_running',
    'rkllm_clear_kv_cache',
    'rkllm_get_kv_cache_size',
    'rkllm_set_chat_template',
    'rkllm_set_function_tools',
    'rkllm_set_cross_attn_params',
    'rkllm_list_functions',   // Custom function to list all available functions
    'rkllm_get_constants'     // NEW: Get all RKLLM constants and enums
];

// =============================================================================
// TEST CLIENT CLASS
// =============================================================================

class RKLLMTestClient {
    constructor() {
        this.baseUrl = 'http://localhost:8082';
        this.wsUrl = 'ws://localhost:8083';
        this.tcpPort = 8080;
        this.requestId = 1;
        this.serverProcess = null;
        this.serverReady = false;
        this.testResults = {
            total: 0,
            passed: 0,
            failed: 0,
            functions: {},
            constants: {},
            enums: {}
        };
    }

    // Generate unique request ID
    getRequestId() {
        return this.requestId++;
    }

    // =============================================================================
    // SERVER MANAGEMENT
    // =============================================================================

    // Start the MCP server process
    async startServer() {
        return new Promise((resolve, reject) => {
            console.log('🚀 Starting MCP Server...');
            
            // Path to the server executable
            const serverPath = path.join(__dirname, '..', 'build', 'mcp_server');
            
            // Spawn the server process with HTTP-only mode to avoid STDIO conflicts
            this.serverProcess = spawn(serverPath, [
                '--disable-stdio',  // Disable STDIO to avoid conflicts
                '--disable-udp',    // Only use HTTP, WS, TCP for testing
                '--http', '8082',
                '--ws', '8083', 
                '--tcp', '8080'
            ], {
                stdio: ['ignore', 'pipe', 'pipe']  // Capture stdout/stderr
            });

            let startupOutput = '';
            let ready = false;

            // Monitor server output
            this.serverProcess.stdout.on('data', (data) => {
                const output = data.toString();
                startupOutput += output;
                
                // Look for server ready indicators
                if (output.includes('Server running') || output.includes('started successfully') || output.includes('listening for connections')) {
                    if (!ready) {
                        ready = true;
                        this.serverReady = true;
                        console.log('✅ MCP Server started successfully');
                        // Give server a moment to fully initialize
                        setTimeout(() => resolve(), 1000);
                    }
                }
            });

            this.serverProcess.stderr.on('data', (data) => {
                const output = data.toString();
                startupOutput += output;
                console.log(`[SERVER] ${output.trim()}`);
                
                // Also check stderr for ready indicators
                if (output.includes('Server running') || output.includes('started successfully') || output.includes('listening for connections')) {
                    if (!ready) {
                        ready = true;
                        this.serverReady = true;
                        console.log('✅ MCP Server started successfully');
                        setTimeout(() => resolve(), 1000);
                    }
                }
            });

            this.serverProcess.on('error', (error) => {
                console.error('❌ Failed to start server:', error.message);
                reject(error);
            });

            this.serverProcess.on('exit', (code, signal) => {
                console.log(`[SERVER] Process exited with code ${code}, signal ${signal}`);
                this.serverReady = false;
            });

            // Timeout after 10 seconds if server doesn't start
            setTimeout(() => {
                if (!ready) {
                    console.error('❌ Server startup timeout');
                    console.log('Server output:', startupOutput);
                    this.stopServer();
                    reject(new Error('Server startup timeout'));
                }
            }, 10000);
        });
    }

    // Stop the MCP server process
    async stopServer() {
        if (this.serverProcess) {
            console.log('🛑 Stopping MCP Server...');
            this.serverProcess.kill('SIGTERM');
            
            // Give it time to shut down gracefully
            await new Promise(resolve => {
                setTimeout(() => {
                    if (this.serverProcess && !this.serverProcess.killed) {
                        this.serverProcess.kill('SIGKILL');
                    }
                    this.serverProcess = null;
                    this.serverReady = false;
                    console.log('✅ MCP Server stopped');
                    resolve();
                }, 2000);
            });
        }
    }

    // Wait for server to be ready
    async waitForServer() {
        const maxRetries = 20;
        const retryDelay = 500;
        
        for (let i = 0; i < maxRetries; i++) {
            try {
                // Try to connect to HTTP endpoint
                const response = await this.httpRequest('rkllm_list_functions', {});
                if (response) {
                    console.log('✅ Server is responding to requests');
                    return true;
                }
            } catch (error) {
                // Server not ready yet, wait and retry
                await new Promise(resolve => setTimeout(resolve, retryDelay));
            }
        }
        throw new Error('Server failed to respond after maximum retries');
    }

    // Create JSON-RPC 2.0 request
    createJsonRpcRequest(method, params = {}) {
        return {
            jsonrpc: "2.0",
            id: this.getRequestId(),
            method: method,
            params: params
        };
    }

    // =============================================================================
    // TRANSPORT METHODS
    // =============================================================================

    // HTTP Transport
    async httpRequest(method, params) {
        return new Promise((resolve, reject) => {
            const data = JSON.stringify(this.createJsonRpcRequest(method, params));
            
            const options = {
                hostname: 'localhost',
                port: 8080,
                path: '/',
                method: 'POST',
                headers: {
                    'Content-Type': 'application/json',
                    'Content-Length': Buffer.byteLength(data)
                }
            };

            const req = http.request(options, (res) => {
                let body = '';
                res.on('data', (chunk) => body += chunk);
                res.on('end', () => {
                    try {
                        resolve(JSON.parse(body));
                    } catch (e) {
                        reject(new Error(`Invalid JSON response: ${body}`));
                    }
                });
            });

            req.on('error', reject);
            req.write(data);
            req.end();
        });
    }

    // WebSocket Transport  
    async wsRequest(method, params) {
        return new Promise((resolve, reject) => {
            const ws = new WebSocket(this.wsUrl);
            const data = JSON.stringify(this.createJsonRpcRequest(method, params));

            ws.on('open', () => {
                ws.send(data);
            });

            ws.on('message', (data) => {
                try {
                    const response = JSON.parse(data.toString());
                    ws.close();
                    resolve(response);
                } catch (e) {
                    reject(new Error(`Invalid JSON response: ${data}`));
                }
            });

            ws.on('error', reject);
        });
    }

    // TCP Transport
    async tcpRequest(method, params) {
        return new Promise((resolve, reject) => {
            const client = new net.Socket();
            const data = JSON.stringify(this.createJsonRpcRequest(method, params)) + '\n';

            client.connect(this.tcpPort, 'localhost', () => {
                client.write(data);
            });

            let buffer = '';
            client.on('data', (data) => {
                buffer += data.toString();
                if (buffer.includes('\n')) {
                    try {
                        const response = JSON.parse(buffer.trim());
                        client.destroy();
                        resolve(response);
                    } catch (e) {
                        reject(new Error(`Invalid JSON response: ${buffer}`));
                    }
                }
            });

            client.on('error', reject);
        });
    }

    // =============================================================================
    // TEST METHODS
    // =============================================================================

    async testFunction(functionName, params = {}, transport = 'http') {
        const testName = `${functionName} (${transport})`;
        this.testResults.total++;

        try {
            let response;
            switch (transport) {
                case 'http':
                    response = await this.httpRequest(functionName, params);
                    break;
                case 'ws':
                    response = await this.wsRequest(functionName, params);
                    break;
                case 'tcp':
                    response = await this.tcpRequest(functionName, params);
                    break;
                default:
                    throw new Error(`Unknown transport: ${transport}`);
            }

            if (response.error) {
                console.log(`❌ ${testName}: ERROR - ${response.error.message}`);
                this.testResults.failed++;
                this.testResults.functions[functionName] = { 
                    status: 'failed', 
                    error: response.error.message,
                    transport 
                };
            } else {
                console.log(`✅ ${testName}: SUCCESS`);
                console.log(`   Response:`, JSON.stringify(response.result, null, 2));
                this.testResults.passed++;
                this.testResults.functions[functionName] = { 
                    status: 'passed', 
                    result: response.result,
                    transport 
                };
            }
        } catch (error) {
            console.log(`❌ ${testName}: EXCEPTION - ${error.message}`);
            this.testResults.failed++;
            this.testResults.functions[functionName] = { 
                status: 'failed', 
                error: error.message,
                transport 
            };
        }
    }

    // Test all RKLLM functions
    async testAllFunctions() {
        console.log('🚀 Testing All RKLLM Functions');
        console.log('='.repeat(50));

        // Test basic functions first
        await this.testFunction('rkllm_list_functions');
        await this.testFunction('rkllm_get_constants');  // NEW: Test constants function
        await this.testFunction('rkllm_createDefaultParam');

        // Test initialization functions
        const initParams = {
            model_path: 'models/qwen3/model.rkllm',
            max_context_len: 512,
            max_new_tokens: 100,
            top_k: 40,
            top_p: 0.9,
            temperature: 0.8,
            repeat_penalty: 1.1,
            skip_special_token: false,
            is_async: false,
            extend_param: {
                enabled_cpus_num: 4,
                enabled_cpus_mask: RKLLM_CPU.CPU4 | RKLLM_CPU.CPU5 | RKLLM_CPU.CPU6 | RKLLM_CPU.CPU7,
                n_batch: 1,
                use_cross_attn: 0
            }
        };

        await this.testFunction('rkllm_init', initParams);

        // Test utility functions
        await this.testFunction('rkllm_is_running');
        await this.testFunction('rkllm_get_kv_cache_size');

        // Test inference function
        const runParams = {
            role: 'user',
            enable_thinking: false,
            input_type: RKLLMInputType.RKLLM_INPUT_PROMPT,
            prompt_input: 'What is AI?',
            mode: RKLLMInferMode.RKLLM_INFER_GENERATE,
            keep_history: 1
        };

        await this.testFunction('rkllm_run', runParams);

        // Test streaming function
        const streamParams = {
            ...runParams,
            stream: true
        };

        await this.testFunction('rkllm_run', streamParams);

        // Test advanced functions
        await this.testFunction('rkllm_set_chat_template', {
            system_prompt: 'You are a helpful assistant.',
            prompt_prefix: '[USER]: ',
            prompt_postfix: '\n[ASSISTANT]: '
        });

        await this.testFunction('rkllm_set_function_tools', {
            system_prompt: 'You are a function-calling assistant.',
            tools: JSON.stringify([{
                name: 'get_weather',
                description: 'Get weather information',
                parameters: {
                    type: 'object',
                    properties: {
                        location: { type: 'string' }
                    }
                }
            }]),
            tool_response_str: '[TOOL_RESPONSE]'
        });

        // Test Lora functions
        await this.testFunction('rkllm_load_lora', {
            lora_adapter_path: '/path/to/lora/adapter',
            lora_adapter_name: 'test_adapter',
            scale: 1.0
        });

        // Test prompt cache functions
        await this.testFunction('rkllm_load_prompt_cache', {
            prompt_cache_path: '/path/to/cache.bin'
        });

        await this.testFunction('rkllm_release_prompt_cache');

        // Test KV cache functions
        await this.testFunction('rkllm_clear_kv_cache', {
            keep_system_prompt: 1,
            start_pos: null,
            end_pos: null
        });

        // Test async and abort functions
        await this.testFunction('rkllm_run_async', runParams);
        await this.testFunction('rkllm_abort');

        // Test cleanup
        await this.testFunction('rkllm_destroy');
    }

    // Test functions on different transports
    async testMultipleTransports() {
        console.log('\n🌐 Testing Multiple Transports');
        console.log('='.repeat(50));

        const testFunctions = ['rkllm_list_functions', 'rkllm_createDefaultParam'];
        const transports = ['http', 'ws', 'tcp'];

        for (const func of testFunctions) {
            for (const transport of transports) {
                await this.testFunction(func, {}, transport);
            }
        }
    }

    // Test RKLLM constants and enums
    testConstants() {
        console.log('\n📊 Testing RKLLM Constants and Enums');
        console.log('='.repeat(50));

        console.log('🔢 CPU Masks:');
        Object.entries(RKLLM_CPU).forEach(([name, value]) => {
            console.log(`   ${name}: 0x${value.toString(16).padStart(2, '0')} (${value})`);
        });

        console.log('\n🔄 LLM Call States:');
        Object.entries(LLMCallState).forEach(([name, value]) => {
            console.log(`   ${name}: ${value}`);
        });

        console.log('\n📝 Input Types:');
        Object.entries(RKLLMInputType).forEach(([name, value]) => {
            console.log(`   ${name}: ${value}`);
        });

        console.log('\n🧠 Inference Modes:');
        Object.entries(RKLLMInferMode).forEach(([name, value]) => {
            console.log(`   ${name}: ${value}`);
        });

        // Test constant combinations
        console.log('\n🔧 Example CPU Mask Combinations:');
        console.log(`   All 8 CPUs: 0x${(RKLLM_CPU.CPU0 | RKLLM_CPU.CPU1 | RKLLM_CPU.CPU2 | RKLLM_CPU.CPU3 | RKLLM_CPU.CPU4 | RKLLM_CPU.CPU5 | RKLLM_CPU.CPU6 | RKLLM_CPU.CPU7).toString(16)} (${RKLLM_CPU.CPU0 | RKLLM_CPU.CPU1 | RKLLM_CPU.CPU2 | RKLLM_CPU.CPU3 | RKLLM_CPU.CPU4 | RKLLM_CPU.CPU5 | RKLLM_CPU.CPU6 | RKLLM_CPU.CPU7})`);
        console.log(`   High-perf CPUs (4-7): 0x${(RKLLM_CPU.CPU4 | RKLLM_CPU.CPU5 | RKLLM_CPU.CPU6 | RKLLM_CPU.CPU7).toString(16)} (${RKLLM_CPU.CPU4 | RKLLM_CPU.CPU5 | RKLLM_CPU.CPU6 | RKLLM_CPU.CPU7})`);
        console.log(`   Low-power CPUs (0-3): 0x${(RKLLM_CPU.CPU0 | RKLLM_CPU.CPU1 | RKLLM_CPU.CPU2 | RKLLM_CPU.CPU3).toString(16)} (${RKLLM_CPU.CPU0 | RKLLM_CPU.CPU1 | RKLLM_CPU.CPU2 | RKLLM_CPU.CPU3})`);

        this.testResults.constants = RKLLM_CPU;
        this.testResults.enums = { LLMCallState, RKLLMInputType, RKLLMInferMode };
    }

    // Test streaming functionality
    async testStreaming() {
        console.log('\n🌊 Testing Streaming Functionality');
        console.log('='.repeat(50));

        // Test HTTP streaming
        try {
            const streamRequest = this.createJsonRpcRequest('rkllm_run', {
                role: 'user',
                enable_thinking: false,
                input_type: RKLLMInputType.RKLLM_INPUT_PROMPT,
                prompt_input: 'Count from 1 to 5',
                mode: RKLLMInferMode.RKLLM_INFER_GENERATE,
                stream: true,
                max_new_tokens: 50
            });

            console.log('📡 Sending streaming request...');
            const response = await this.httpRequest('rkllm_run', streamRequest.params);
            
            if (response.result && response.result.stream_id) {
                console.log(`✅ Streaming initiated with ID: ${response.result.stream_id}`);
            } else {
                console.log('❌ Streaming failed to initialize');
            }
        } catch (error) {
            console.log(`❌ Streaming test failed: ${error.message}`);
        }
    }

    // Generate test report
    generateReport() {
        console.log('\n📋 Test Report');
        console.log('='.repeat(50));
        console.log(`Total Tests: ${this.testResults.total}`);
        console.log(`Passed: ${this.testResults.passed}`);
        console.log(`Failed: ${this.testResults.failed}`);
        console.log(`Success Rate: ${((this.testResults.passed / this.testResults.total) * 100).toFixed(1)}%`);

        console.log('\n📊 Function Test Results:');
        Object.entries(this.testResults.functions).forEach(([func, result]) => {
            const status = result.status === 'passed' ? '✅' : '❌';
            console.log(`   ${status} ${func} (${result.transport || 'http'})`);
            if (result.error) {
                console.log(`      Error: ${result.error}`);
            }
        });

        console.log('\n🔢 Available Constants:');
        console.log(`   CPU Masks: ${Object.keys(RKLLM_CPU).length} constants`);
        console.log(`   Call States: ${Object.keys(LLMCallState).length} states`);
        console.log(`   Input Types: ${Object.keys(RKLLMInputType).length} types`);
        console.log(`   Inference Modes: ${Object.keys(RKLLMInferMode).length} modes`);

        console.log('\n📈 RKLLM Function Coverage:');
        console.log(`   Total RKLLM Functions: ${RKLLM_FUNCTIONS.length}`);
        console.log(`   Functions Tested: ${Object.keys(this.testResults.functions).length}`);
        console.log(`   Coverage: ${((Object.keys(this.testResults.functions).length / RKLLM_FUNCTIONS.length) * 100).toFixed(1)}%`);
    }

    // Main test runner
    async run() {
        console.log('🚀 RKLLM JavaScript Test Client');
        console.log('='.repeat(50));
        console.log('Testing comprehensive RKLLM functionality via MCP server');
        console.log(`Server: ${this.baseUrl}`);
        console.log(`WebSocket: ${this.wsUrl}`);
        console.log(`TCP: localhost:${this.tcpPort}`);
        console.log('');

        try {
            // Start the MCP server
            await this.startServer();
            
            // Wait for server to be ready
            await this.waitForServer();

            // Test constants first (no server needed for display, but test server function too)
            this.testConstants();

            // Test server functions
            await this.testAllFunctions();
            
            // Test multiple transports
            await this.testMultipleTransports();
            
            // Test streaming
            await this.testStreaming();

            // Generate final report
            this.generateReport();
            
        } catch (error) {
            console.error('❌ Test execution failed:', error.message);
            throw error;
        } finally {
            // Always stop the server when done
            await this.stopServer();
        }
    }
}

// =============================================================================
// MAIN EXECUTION
// =============================================================================

if (require.main === module) {
    const client = new RKLLMTestClient();
    
    // Handle graceful shutdown
    process.on('SIGINT', async () => {
        console.log('\n🛑 Received interrupt signal, shutting down...');
        await client.stopServer();
        process.exit(0);
    });
    
    process.on('SIGTERM', async () => {
        console.log('\n🛑 Received terminate signal, shutting down...');
        await client.stopServer();
        process.exit(0);
    });
    
    client.run().catch(async (error) => {
        console.error('❌ Test execution failed:', error.message);
        await client.stopServer();
        process.exit(1);
    });
}

module.exports = { RKLLMTestClient, RKLLM_CPU, LLMCallState, RKLLMInputType, RKLLMInferMode };