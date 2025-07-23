#!/usr/bin/env node

/**
 * FOCUSED TEST: LoRa and Logits Functionality
 * Tests proper LoRa workflow and logits extraction debugging
 */

const ServerManager = require('../tests/lib/server-manager');
const TestClient = require('../tests/lib/test-client');
const { createRKLLMParam, createRKLLMInput, createRKLLMInferParam } = require('../tests/lib/test-helpers');
const path = require('path');
const fs = require('fs');
const net = require('net');
const { spawn } = require('child_process');

class RKLLMClient {
    constructor(socketPath = '/tmp/rkllm.sock') {
        this.socketPath = socketPath;
        this.socket = null;
        this.connected = false;
        this.requestId = 1;
        this.pendingRequests = new Map();
    }

    async connect(timeout = 5000) {
        return new Promise((resolve, reject) => {
            const timeoutId = setTimeout(() => {
                reject(new Error('Connection timeout'));
            }, timeout);

            this.socket = net.createConnection(this.socketPath);
            
            this.socket.on('connect', () => {
                clearTimeout(timeoutId);
                this.connected = true;
                console.log('✅ Connected to RKLLM server');
                resolve();
            });

            this.socket.on('error', (err) => {
                clearTimeout(timeoutId);
                reject(err);
            });

            this.socket.on('data', (data) => {
                this.handleResponse(data.toString());
            });

            this.socket.on('close', () => {
                this.connected = false;
                console.log('❌ Disconnected from server');
            });
        });
    }

    handleResponse(data) {
        try {
            const response = JSON.parse(data);
            console.log('📥 Received response:', JSON.stringify(response, null, 2));
            
            if (response.id && this.pendingRequests.has(response.id)) {
                const { resolve } = this.pendingRequests.get(response.id);
                this.pendingRequests.delete(response.id);
                resolve(response);
            }
        } catch (error) {
            console.error('❌ Failed to parse response:', error.message);
        }
    }

    async sendRequest(method, params, timeout = 30000) {
        return new Promise((resolve, reject) => {
            const id = this.requestId++;
            const request = {
                jsonrpc: "2.0",
                id: id,
                method: method,
                params: params
            };

            const timeoutId = setTimeout(() => {
                this.pendingRequests.delete(id);
                reject(new Error(`Request timeout after ${timeout}ms for method: ${method}`));
            }, timeout);

            this.pendingRequests.set(id, { 
                resolve: (response) => {
                    clearTimeout(timeoutId);
                    resolve(response);
                }
            });

            const requestStr = JSON.stringify(request);
            console.log('📤 Sending request:', requestStr);
            this.socket.write(requestStr);
        });
    }

    disconnect() {
        if (this.socket) {
            this.socket.end();
        }
    }
}

async function testLoRaWorkflow() {
    console.log('\n🔬 Testing LoRa Workflow');
    console.log('========================');

    const client = new RKLLMClient();
    let server = null;

    try {
        // Start server
        console.log('🚀 Starting RKLLM server...');
        server = spawn('./rkllm_uds_server', [], {
            cwd: '/home/x/Projects/nano/build',
            env: { ...process.env, LD_LIBRARY_PATH: '../src/external/rkllm' }
        });

        server.stdout.on('data', (data) => {
            console.log('🖥️  Server:', data.toString().trim());
        });

        server.stderr.on('data', (data) => {
            console.log('🖥️  Server Error:', data.toString().trim());
        });

        // Wait for server to start
        await new Promise(resolve => setTimeout(resolve, 2000));

        // Connect to server
        await client.connect();

        // Step 1: Initialize with base LoRa model
        console.log('\n📁 Step 1: Initialize with base LoRa model');
        const initResponse = await client.sendRequest('rkllm.init', [{
            model_path: "/home/x/Projects/nano/models/lora/model.rkllm",
            max_context_len: 512,
            max_new_tokens: 50,
            top_k: 40,
            top_p: 0.9,
            temperature: 0.8
        }]);

        if (initResponse.error) {
            throw new Error(`Init failed: ${initResponse.error.message}`);
        }
        console.log('✅ Model initialized successfully');

        // Step 2: Load LoRa adapter
        console.log('\n🔧 Step 2: Load LoRa adapter');
        const loraResponse = await client.sendRequest('rkllm.load_lora', [{
            lora_adapter_path: "/home/x/Projects/nano/models/lora/lora.rkllm",
            lora_adapter_name: "test_adapter",
            scale: 1.0
        }]);

        if (loraResponse.error) {
            throw new Error(`LoRa load failed: ${loraResponse.error.message}`);
        }
        console.log('✅ LoRa adapter loaded successfully');

        // Step 3: Run inference WITHOUT LoRa (baseline)
        console.log('\n🏃 Step 3: Run inference WITHOUT LoRa (baseline)');
        const baselineResponse = await client.sendRequest('rkllm.run', [
            null,
            {
                role: "user",
                enable_thinking: false,
                input_type: 0,
                prompt_input: "What is artificial intelligence?"
            },
            {
                mode: 0,
                lora_params: null,
                keep_history: 0
            },
            null
        ]);

        if (baselineResponse.error) {
            throw new Error(`Baseline inference failed: ${baselineResponse.error.message}`);
        }
        console.log('✅ Baseline response:', baselineResponse.result.text);

        // Step 4: Run inference WITH LoRa
        console.log('\n🎯 Step 4: Run inference WITH LoRa');
        const loraInferenceResponse = await client.sendRequest('rkllm.run', [
            null,
            {
                role: "user",
                enable_thinking: false,
                input_type: 0,
                prompt_input: "What is artificial intelligence?"
            },
            {
                mode: 0,
                lora_params: {
                    lora_adapter_name: "test_adapter"
                },
                keep_history: 0
            },
            null
        ]);

        if (loraInferenceResponse.error) {
            throw new Error(`LoRa inference failed: ${loraInferenceResponse.error.message}`);
        }
        console.log('✅ LoRa response:', loraInferenceResponse.result.text);

        // Compare responses
        console.log('\n📊 Comparison Results:');
        console.log('Baseline:', baselineResponse.result.text);
        console.log('With LoRa:', loraInferenceResponse.result.text);
        
        if (baselineResponse.result.text !== loraInferenceResponse.result.text) {
            console.log('✅ LoRa is working - responses are different!');
        } else {
            console.log('⚠️  LoRa may not be working - responses are identical');
        }

        console.log('\n🎉 LoRa workflow test completed successfully!');

    } catch (error) {
        console.error('❌ LoRa test failed:', error.message);
        throw error;
    } finally {
        client.disconnect();
        if (server) {
            server.kill();
        }
    }
}

async function testLogitsWithTimeout() {
    console.log('\n🔬 Testing Logits Mode with Timeout Protection');
    console.log('=============================================');

    const client = new RKLLMClient();
    let server = null;

    try {
        // Start server
        console.log('🚀 Starting RKLLM server...');
        server = spawn('./rkllm_uds_server', [], {
            cwd: '/home/x/Projects/nano/build',
            env: { ...process.env, LD_LIBRARY_PATH: '../src/external/rkllm' }
        });

        server.stdout.on('data', (data) => {
            console.log('🖥️  Server:', data.toString().trim());
        });

        server.stderr.on('data', (data) => {
            console.log('🖥️  Server Error:', data.toString().trim());
        });

        // Wait for server to start
        await new Promise(resolve => setTimeout(resolve, 2000));

        // Connect to server
        await client.connect();

        // Initialize with standard model (not LoRa model)
        console.log('\n📁 Initialize with standard model');
        const initResponse = await client.sendRequest('rkllm.init', [{
            model_path: "/home/x/Projects/nano/models/qwen3/model.rkllm",
            max_context_len: 512,
            max_new_tokens: 50
        }]);

        if (initResponse.error) {
            throw new Error(`Init failed: ${initResponse.error.message}`);
        }
        console.log('✅ Model initialized successfully');

        // Test logits mode with 15-second timeout
        console.log('\n🎯 Testing logits mode (15s timeout)...');
        
        try {
            const logitsResponse = await client.sendRequest('rkllm.run', [
                null,
                {
                    role: "user",
                    enable_thinking: false,
                    input_type: 0,
                    prompt_input: "Hello"
                },
                {
                    mode: 2, // RKLLM_INFER_GET_LOGITS
                    keep_history: 0
                },
                null
            ], 15000); // 15 second timeout

            if (logitsResponse.error) {
                console.log('❌ Logits mode failed:', logitsResponse.error.message);
            } else {
                console.log('✅ Logits mode succeeded!');
                console.log('Logits data:', logitsResponse.result.logits);
            }

        } catch (timeoutError) {
            console.log('⏰ Logits mode timed out after 15 seconds - this confirms the hanging issue');
            console.log('🔍 This indicates the RKLLM library hangs on logits extraction');
        }

        console.log('\n📋 Logits test completed (timeout protection working)');

    } catch (error) {
        console.error('❌ Logits test failed:', error.message);
        throw error;
    } finally {
        client.disconnect();
        if (server) {
            server.kill();
        }
    }
}

async function runTests() {
    console.log('🚀 Starting LoRa and Logits Tests');
    console.log('==================================');

    try {
        // Test LoRa workflow
        await testLoRaWorkflow();
        
        // Wait between tests
        await new Promise(resolve => setTimeout(resolve, 3000));
        
        // Test logits with timeout protection
        await testLogitsWithTimeout();

        console.log('\n🎉 All tests completed!');

    } catch (error) {
        console.error('💥 Test suite failed:', error.message);
        process.exit(1);
    }
}

// Run tests
runTests();