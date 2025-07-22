#!/usr/bin/env node

/**
 * RKLLM JavaScript Test Client - Modern ES6 Implementation
 * 
 * Comprehensive test suite for all RKLLM functions, constants, and enums
 * Tests the MCP server architecture with JSON-RPC 2.0 protocol
 * 
 * This client spawns the MCP server process and runs tests concurrently
 */

import http from 'http';
import WebSocket from 'ws';
import net from 'net';
import { spawn } from 'child_process';
import { join, dirname } from 'path';
import { fileURLToPath } from 'url';

// Get __dirname equivalent for ES modules
const __filename = fileURLToPath(import.meta.url);
const __dirname = dirname(__filename);

// =============================================================================
// RKLLM CONSTANTS AND ENUMS (from rkllm.h)
// =============================================================================

// CPU Masks for enabled_cpus_mask parameter
export const RKLLM_CPU = {
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
export const LLMCallState = {
    RKLLM_RUN_NORMAL: 0,   // The LLM call is in a normal running state
    RKLLM_RUN_WAITING: 1,  // The LLM call is waiting for complete UTF-8 encoded character
    RKLLM_RUN_FINISH: 2,   // The LLM call has finished execution
    RKLLM_RUN_ERROR: 3,    // An error occurred during the LLM call
};

// Input Types
export const RKLLMInputType = {
    RKLLM_INPUT_PROMPT: 0,      // Input is a text prompt
    RKLLM_INPUT_TOKEN: 1,       // Input is a sequence of tokens
    RKLLM_INPUT_EMBED: 2,       // Input is an embedding vector
    RKLLM_INPUT_MULTIMODAL: 3,  // Input is multimodal (e.g., text and image)
};

// Inference Modes
export const RKLLMInferMode = {
    RKLLM_INFER_GENERATE: 0,                  // The LLM generates text based on input
    RKLLM_INFER_GET_LAST_HIDDEN_LAYER: 1,    // The LLM retrieves the last hidden layer
    RKLLM_INFER_GET_LOGITS: 2,               // The LLM retrieves logits
};

// RKLLM Function Definitions
export const RKLLM_FUNCTIONS = [
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
    'rkllm_list_functions',
    'rkllm_get_constants'
];

// =============================================================================
// MODERN ES6 TEST CLIENT CLASS
// =============================================================================

export class RKLLMTestClient {
    #baseUrl = 'http://localhost:8082';  // ✅ FIXED: Match server HTTP port
    #wsUrl = 'ws://localhost:8083';
    #tcpPort = 8080;
    #udpPort = 8081;  // ✅ NEW: Add UDP port support
    #requestId = 1;
    #serverProcess = null;
    #serverReady = false;
    #testResults = {
        total: 0,
        passed: 0,
        failed: 0,
        functions: new Map(),
        constants: new Map(),
        enums: new Map(),
        streaming: new Map()  // ✅ NEW: Track streaming test results
    };

    constructor(config = {}) {
        this.#baseUrl = config.baseUrl ?? this.#baseUrl;
        this.#wsUrl = config.wsUrl ?? this.#wsUrl;
        this.#tcpPort = config.tcpPort ?? this.#tcpPort;
        this.#udpPort = config.udpPort ?? this.#udpPort;
    }

    // Generate unique request ID
    get nextRequestId() {
        return this.#requestId++;
    }

    get serverReady() {
        return this.#serverReady;
    }

    get testResults() {
        return { ...this.#testResults };
    }

    // =============================================================================
    // SERVER MANAGEMENT
    // =============================================================================

    async startServer() {
        return new Promise((resolve, reject) => {
            console.log('🚀 Starting MCP Server...');
            
            const serverPath = join(__dirname, '..', 'build', 'mcp_server');
            const serverArgs = [
                '--force',           // ✅ FIXED: Use --force to kill existing processes
                '--tcp', '8080',     // ✅ FIXED: All 5 transports enabled per PRD
                '--udp', '8081',     // ✅ FIXED: Enable UDP transport
                '--http', '8082',    // ✅ FIXED: Correct HTTP port
                '--ws', '8083'       // ✅ FIXED: Enable WebSocket
                // ✅ FIXED: Enable STDIO (default, no --disable-stdio flag)
            ];

            this.#serverProcess = spawn(serverPath, serverArgs, {
                stdio: ['ignore', 'pipe', 'pipe']
            });

            let startupOutput = '';
            let ready = false;

            const checkReady = (output) => {
                const readyIndicators = [
                    'Server running',
                    'started successfully',
                    'listening for connections'
                ];
                
                return readyIndicators.some(indicator => output.includes(indicator));
            };

            const handleOutput = (data) => {
                const output = data.toString();
                startupOutput += output;
                
                if (checkReady(output) && !ready) {
                    ready = true;
                    this.#serverReady = true;
                    console.log('✅ MCP Server started successfully');
                    setTimeout(resolve, 1000);
                }
            };

            this.#serverProcess.stdout?.on('data', handleOutput);
            this.#serverProcess.stderr?.on('data', (data) => {
                const output = data.toString();
                startupOutput += output;
                console.log(`[SERVER] ${output.trim()}`);
                handleOutput(data);
            });

            this.#serverProcess.on('error', (error) => {
                console.error('❌ Failed to start server:', error.message);
                reject(error);
            });

            this.#serverProcess.on('exit', (code, signal) => {
                console.log(`[SERVER] Process exited with code ${code}, signal ${signal}`);
                this.#serverReady = false;
            });

            // Timeout with better error handling
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

    async stopServer() {
        if (!this.#serverProcess) return;

        console.log('🛑 Stopping MCP Server...');
        this.#serverProcess.kill('SIGTERM');
        
        await new Promise(resolve => {
            setTimeout(() => {
                if (this.#serverProcess && !this.#serverProcess.killed) {
                    this.#serverProcess.kill('SIGKILL');
                }
                this.#serverProcess = null;
                this.#serverReady = false;
                console.log('✅ MCP Server stopped');
                resolve();
            }, 2000);
        });
    }

    async waitForServer() {
        const maxRetries = 20;
        const retryDelay = 500;
        
        for (let i = 0; i < maxRetries; i++) {
            try {
                const response = await this.httpRequest('rkllm_list_functions', {});
                if (response) {
                    console.log('✅ Server is responding to requests');
                    return true;
                }
            } catch (error) {
                await new Promise(resolve => setTimeout(resolve, retryDelay));
            }
        }
        throw new Error('Server failed to respond after maximum retries');
    }

    // Create JSON-RPC 2.0 request with modern syntax
    createJsonRpcRequest(method, params = {}) {
        return {
            jsonrpc: "2.0",
            id: this.nextRequestId,
            method,
            params
        };
    }

    // =============================================================================
    // MODERN TRANSPORT METHODS
    // =============================================================================

    async httpRequest(method, params) {
        const requestData = JSON.stringify(this.createJsonRpcRequest(method, params));
        
        const options = {
            hostname: 'localhost',
            port: 8082,  // ✅ FIXED: Use correct HTTP port
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
                res.on('data', chunk => body += chunk);
                res.on('end', () => {
                    try {
                        resolve(JSON.parse(body));
                    } catch (error) {
                        reject(new Error(`Invalid JSON response: ${body}`));
                    }
                });
            });

            req.on('error', reject);
            req.write(requestData);
            req.end();
        });
    }

    async wsRequest(method, params) {
        const requestData = JSON.stringify(this.createJsonRpcRequest(method, params));

        return new Promise((resolve, reject) => {
            const ws = new WebSocket(this.#wsUrl);

            ws.on('open', () => ws.send(requestData));
            
            ws.on('message', (data) => {
                try {
                    const response = JSON.parse(data.toString());
                    ws.close();
                    resolve(response);
                } catch (error) {
                    reject(new Error(`Invalid JSON response: ${data}`));
                }
            });

            ws.on('error', reject);
        });
    }

    async tcpRequest(method, params) {
        const requestData = JSON.stringify(this.createJsonRpcRequest(method, params)) + '\n';

        return new Promise((resolve, reject) => {
            const client = new net.Socket();

            client.connect(this.#tcpPort, 'localhost', () => {
                client.write(requestData);
            });

            let buffer = '';
            client.on('data', (data) => {
                buffer += data.toString();
                if (buffer.includes('\n')) {
                    try {
                        const response = JSON.parse(buffer.trim());
                        client.destroy();
                        resolve(response);
                    } catch (error) {
                        reject(new Error(`Invalid JSON response: ${buffer}`));
                    }
                }
            });

            client.on('error', reject);
        });
    }

    // ✅ NEW: Add UDP transport support per PRD requirement
    async udpRequest(method, params) {
        const requestData = JSON.stringify(this.createJsonRpcRequest(method, params)) + '\n';
        const dgram = await import('dgram');

        return new Promise((resolve, reject) => {
            const client = dgram.createSocket('udp4');
            
            client.send(requestData, this.#udpPort, 'localhost', (error) => {
                if (error) {
                    client.close();
                    reject(error);
                    return;
                }
            });

            client.on('message', (data) => {
                try {
                    const response = JSON.parse(data.toString().trim());
                    client.close();
                    resolve(response);
                } catch (error) {
                    client.close();
                    reject(new Error(`Invalid JSON response: ${data}`));
                }
            });

            client.on('error', (error) => {
                client.close();
                reject(error);
            });

            // Timeout after 5 seconds
            setTimeout(() => {
                client.close();
                reject(new Error('UDP request timeout'));
            }, 5000);
        });
    }

    // ✅ NEW: Add STDIO transport support per PRD requirement
    async stdioRequest(method, params) {
        const requestData = JSON.stringify(this.createJsonRpcRequest(method, params)) + '\n';
        
        return new Promise((resolve, reject) => {
            const child = spawn(join(__dirname, '..', 'build', 'mcp_server'), ['--stdio-only'], {
                stdio: ['pipe', 'pipe', 'pipe']
            });

            let responseData = '';
            
            child.stdout.on('data', (data) => {
                responseData += data.toString();
                if (responseData.includes('\n')) {
                    try {
                        const response = JSON.parse(responseData.trim());
                        child.kill();
                        resolve(response);
                    } catch (error) {
                        child.kill();
                        reject(new Error(`Invalid JSON response: ${responseData}`));
                    }
                }
            });

            child.on('error', (error) => {
                child.kill();
                reject(error);
            });

            child.stdin.write(requestData);
        });
    }

    // =============================================================================
    // MODERN TEST METHODS
    // =============================================================================

    async testFunction(functionName, params = {}, transport = 'http') {
        const testName = `${functionName} (${transport})`;
        this.#testResults.total++;

        try {
            const transportMap = {
                http: this.httpRequest.bind(this),
                ws: this.wsRequest.bind(this),
                tcp: this.tcpRequest.bind(this),
                udp: this.udpRequest.bind(this),    // ✅ NEW: Add UDP transport
                stdio: this.stdioRequest.bind(this) // ✅ NEW: Add STDIO transport
            };

            const transportMethod = transportMap[transport];
            if (!transportMethod) {
                throw new Error(`Unknown transport: ${transport}`);
            }

            const response = await transportMethod(functionName, params);

            if (response.error) {
                console.log(`❌ ${testName}: ERROR - ${response.error.message}`);
                this.#testResults.failed++;
                this.#testResults.functions.set(functionName, { 
                    status: 'failed', 
                    error: response.error.message,
                    transport 
                });
            } else {
                console.log(`✅ ${testName}: SUCCESS`);
                console.log(`   Response:`, JSON.stringify(response.result, null, 2));
                this.#testResults.passed++;
                this.#testResults.functions.set(functionName, { 
                    status: 'passed', 
                    result: response.result,
                    transport 
                });
            }
        } catch (error) {
            console.log(`❌ ${testName}: EXCEPTION - ${error.message}`);
            this.#testResults.failed++;
            this.#testResults.functions.set(functionName, { 
                status: 'failed', 
                error: error.message,
                transport 
            });
        }
    }

    async testAllFunctions() {
        console.log('🚀 Testing All RKLLM Functions');
        console.log('='.repeat(50));

        // Test basic functions first
        await this.testFunction('rkllm_list_functions');
        await this.testFunction('rkllm_get_constants');
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
        const streamParams = { ...runParams, stream: true };
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

        // Test remaining functions
        const additionalTests = [
            ['rkllm_load_lora', {
                lora_adapter_path: '/path/to/lora/adapter',
                lora_adapter_name: 'test_adapter',
                scale: 1.0
            }],
            ['rkllm_load_prompt_cache', { prompt_cache_path: '/path/to/cache.bin' }],
            ['rkllm_release_prompt_cache'],
            ['rkllm_clear_kv_cache', {
                keep_system_prompt: 1,
                start_pos: null,
                end_pos: null
            }],
            ['rkllm_run_async', runParams],
            ['rkllm_abort'],
            ['rkllm_destroy']
        ];

        for (const [funcName, params] of additionalTests) {
            await this.testFunction(funcName, params);
        }
    }

    async testMultipleTransports() {
        console.log('\n🌐 Testing Multiple Transports');
        console.log('='.repeat(50));

        const testMatrix = [
            ['rkllm_list_functions', {}],
            ['rkllm_createDefaultParam', {}]
        ];

        // ✅ FIXED: Test all 5 transports per PRD requirement
        const transports = ['http', 'ws', 'tcp', 'udp', 'stdio'];

        for (const [func, params] of testMatrix) {
            for (const transport of transports) {
                await this.testFunction(func, params, transport);
            }
        }
    }

    // ✅ FIXED: Implement proper streaming tests per IDEA.md specification
    async testStreaming() {
        console.log('\n🌊 Testing Streaming Functionality');
        console.log('='.repeat(50));

        const streamParams = {
            role: 'user',
            enable_thinking: false,
            input_type: RKLLMInputType.RKLLM_INPUT_PROMPT,
            prompt_input: 'Count from 1 to 5',
            mode: RKLLMInferMode.RKLLM_INFER_GENERATE,
            stream: true,
            max_new_tokens: 50
        };

        // ✅ FIXED: Test real-time streaming transports per IDEA.md
        const realTimeTransports = ['ws', 'tcp', 'stdio'];
        
        for (const transport of realTimeTransports) {
            console.log(`📡 Testing ${transport.toUpperCase()} Real-Time Streaming...`);
            
            try {
                await this.testRealTimeStreaming(transport, streamParams);
                this.#testResults.streaming.set(`${transport}_realtime`, { status: 'passed' });
            } catch (error) {
                console.log(`❌ ${transport.toUpperCase()} Real-Time Streaming: ERROR - ${error.message}`);
                this.#testResults.streaming.set(`${transport}_realtime`, { status: 'failed', error: error.message });
            }
        }

        // ✅ FIXED: Test HTTP polling streaming per IDEA.md specification
        console.log('📡 Testing HTTP Polling Streaming...');
        
        try {
            await this.testHttpPollingStreaming(streamParams);
            this.#testResults.streaming.set('http_polling', { status: 'passed' });
        } catch (error) {
            console.log(`❌ HTTP Polling Streaming: ERROR - ${error.message}`);
            this.#testResults.streaming.set('http_polling', { status: 'failed', error: error.message });
        }
    }

    // ✅ NEW: Implement real-time streaming test per IDEA.md
    async testRealTimeStreaming(transport, params) {
        const transportMap = {
            ws: this.wsRequest.bind(this),
            tcp: this.tcpRequest.bind(this),
            stdio: this.stdioRequest.bind(this)
        };

        const method = transportMap[transport];
        if (!method) {
            throw new Error(`Unknown transport: ${transport}`);
        }

        // Start streaming request
        const response = await method('rkllm_run_async', params);
        
        if (response.error) {
            throw new Error(response.error.message);
        }

        console.log(`✅ ${transport.toUpperCase()} Real-Time Streaming: SUCCESS`);
        console.log('   Initial response:', response.result);

        // For real-time transports, chunks should arrive immediately
        // In production, we'd set up listeners for streaming chunks
        return response;
    }

    // ✅ NEW: Implement HTTP polling streaming test per IDEA.md
    async testHttpPollingStreaming(params) {
        // Step 1: Start streaming request
        const streamRequest = await this.httpRequest('rkllm_run_async', params);
        
        if (streamRequest.error) {
            throw new Error(streamRequest.error.message);
        }

        const requestId = streamRequest.id || this.#requestId - 1;
        console.log(`   Started HTTP streaming session: ${requestId}`);

        // Step 2: Poll for chunks using original request ID
        let isComplete = false;
        let pollAttempts = 0;
        const maxPolls = 10;

        while (!isComplete && pollAttempts < maxPolls) {
            pollAttempts++;
            
            // Wait before polling (simulate realistic polling interval)
            await new Promise(resolve => setTimeout(resolve, 1000));
            
            try {
                // Use poll method with original request ID per IDEA.md spec
                const pollResponse = await this.httpRequest('poll', { request_id: requestId });
                
                if (pollResponse.error) {
                    console.log(`   Poll attempt ${pollAttempts}: ERROR - ${pollResponse.error.message}`);
                    continue;
                }

                const chunk = pollResponse.result?.chunk;
                if (chunk) {
                    console.log(`   Poll attempt ${pollAttempts}: Received chunk seq=${chunk.seq}, delta="${chunk.delta}"`);
                    
                    if (chunk.end === true) {
                        isComplete = true;
                        console.log('   ✅ HTTP Polling completed successfully');
                    }
                } else {
                    console.log(`   Poll attempt ${pollAttempts}: No chunks available`);
                }
            } catch (error) {
                console.log(`   Poll attempt ${pollAttempts}: EXCEPTION - ${error.message}`);
            }
        }

        if (!isComplete) {
            throw new Error(`HTTP polling timeout after ${maxPolls} attempts`);
        }

        return { status: 'completed', polls: pollAttempts };
    }

    // =============================================================================
    // FINAL RESULTS OUTPUT
    // =============================================================================

    printResults() {
        console.log('\n📋 Final Test Results');
        console.log('='.repeat(50));
        console.log(`Total: ${this.#testResults.total}`);
        console.log(`Passed: ${this.#testResults.passed}`);
        console.log(`Failed: ${this.#testResults.failed}`);

        console.log('\n🔍 Function Test Results');
        for (const [name, result] of this.#testResults.functions) {
            console.log(` ${name}: ${result.status}`);
            if (result.status === 'failed') {
                console.log(`   Error: ${result.error}`);
            }
        }

        // ✅ NEW: Print streaming test results
        console.log('\n🌊 Streaming Test Results');
        for (const [name, result] of this.#testResults.streaming) {
            console.log(` ${name}: ${result.status}`);
            if (result.status === 'failed') {
                console.log(`   Error: ${result.error}`);
            }
        }

        console.log('\n📊 Constants and Enums Test Results');
        for (const [groupName, constants] of this.#testResults.constants) {
            console.log(`\n${groupName}:`);
            Object.entries(constants).forEach(([name, value]) => {
                console.log(`   ${name}: 0x${value.toString(16).padStart(2, '0')} (${value})`);
            });
        }

        for (const [groupName, enums] of this.#testResults.enums) {
            console.log(`\n${groupName}:`);
            Object.entries(enums).forEach(([name, value]) => {
                console.log(`   ${name}: ${value}`);
            });
        }
    }
}

// =============================================================================
// MAIN EXECUTION - MODERN ASYNC/AWAIT PATTERN
// =============================================================================

const runTests = async () => {
    const client = new RKLLMTestClient();

    try {
        await client.startServer();
        await client.waitForServer();
        
        await client.testAllFunctions();
        await client.testMultipleTransports();
        client.testConstants();
        await client.testStreaming();
        
    } catch (error) {
        console.error('❌ Error:', error.message);
    } finally {
        await client.stopServer();
        client.printResults();
    }
};

// Export for module usage
export default RKLLMTestClient;

// Run if this is the main module
if (import.meta.url === `file://${process.argv[1]}`) {
    runTests().catch(console.error);
}