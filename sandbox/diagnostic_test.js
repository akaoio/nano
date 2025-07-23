#!/usr/bin/env node

/**
 * FOCUSED DIAGNOSTIC: LoRa and Logits Issues
 * This test will help identify and fix both issues
 */

const net = require('net');
const { spawn } = require('child_process');

class DiagnosticClient {
    constructor() {
        this.socket = null;
        this.requestId = 1;
        this.pendingRequests = new Map();
    }

    async connect() {
        return new Promise((resolve, reject) => {
            this.socket = net.createConnection('/tmp/rkllm.sock');
            
            this.socket.on('connect', () => {
                console.log('✅ Connected to server');
                resolve();
            });

            this.socket.on('error', reject);
            this.socket.on('data', (data) => this.handleResponse(data.toString()));
        });
    }

    handleResponse(data) {
        try {
            const response = JSON.parse(data);
            console.log('📥 Response:', JSON.stringify(response, null, 2));
            
            if (response.id && this.pendingRequests.has(response.id)) {
                const { resolve } = this.pendingRequests.get(response.id);
                this.pendingRequests.delete(response.id);
                resolve(response);
            }
        } catch (error) {
            console.error('❌ Parse error:', error.message);
        }
    }

    async sendRequest(method, params, timeout = 30000) {
        return new Promise((resolve, reject) => {
            const id = this.requestId++;
            const request = { jsonrpc: "2.0", id, method, params };

            const timeoutId = setTimeout(() => {
                this.pendingRequests.delete(id);
                reject(new Error(`Timeout after ${timeout}ms`));
            }, timeout);

            this.pendingRequests.set(id, { 
                resolve: (response) => {
                    clearTimeout(timeoutId);
                    resolve(response);
                }
            });

            console.log('📤 Request:', JSON.stringify(request));
            this.socket.write(JSON.stringify(request));
        });
    }

    disconnect() {
        if (this.socket) this.socket.end();
    }
}

async function diagnosticTest() {
    console.log('🔍 DIAGNOSTIC TEST: LoRa and Logits Issues');
    console.log('==========================================\n');

    let server = null;
    const client = new DiagnosticClient();

    try {
        // Start server
        console.log('🚀 Starting server...');
        server = spawn('./rkllm_uds_server', [], {
            cwd: '/home/x/Projects/nano/build',
            env: { ...process.env, LD_LIBRARY_PATH: '../src/external/rkllm' }
        });

        // Wait for startup
        await new Promise(resolve => setTimeout(resolve, 2000));
        await client.connect();

        // === PHASE 1: Test Basic Functionality ===
        console.log('\n📋 PHASE 1: Testing basic functionality');
        
        const initResponse = await client.sendRequest('rkllm.init', [{
            model_path: "/home/x/Projects/nano/models/qwen3/model.rkllm",
            max_context_len: 256,
            max_new_tokens: 20
        }]);

        if (initResponse.error) {
            throw new Error(`Init failed: ${initResponse.error.message}`);
        }
        console.log('✅ Basic model initialization works');

        // Test basic inference
        const basicResponse = await client.sendRequest('rkllm.run', [
            null,
            {
                role: "user",
                input_type: 0,
                prompt_input: "Hello"
            },
            {
                mode: 0,
                keep_history: 0
            },
            null
        ]);

        if (basicResponse.error) {
            throw new Error(`Basic inference failed: ${basicResponse.error.message}`);
        }
        console.log('✅ Basic inference works');

        // === PHASE 2: Test LoRa ===
        console.log('\n🧬 PHASE 2: Testing LoRa functionality');
        
        // First, switch to LoRa model
        await client.sendRequest('rkllm.destroy', [null]);
        await new Promise(resolve => setTimeout(resolve, 1000));

        const loraInitResponse = await client.sendRequest('rkllm.init', [{
            model_path: "/home/x/Projects/nano/models/lora/model.rkllm",
            max_context_len: 256,
            max_new_tokens: 20
        }]);

        if (loraInitResponse.error) {
            console.log('❌ LoRa model init failed:', loraInitResponse.error.message);
        } else {
            console.log('✅ LoRa base model loaded');

            // Try to load LoRa adapter
            const loraLoadResponse = await client.sendRequest('rkllm.load_lora', [
                null,
                {
                    lora_adapter_path: "/home/x/Projects/nano/models/lora/lora.rkllm",
                    lora_adapter_name: "test_adapter",
                    scale: 1.0
                }
            ]);

            if (loraLoadResponse.error) {
                console.log('❌ LoRa adapter load failed:', loraLoadResponse.error.message);
            } else {
                console.log('✅ LoRa adapter loaded successfully');

                // Test inference with LoRa
                const loraInferResponse = await client.sendRequest('rkllm.run', [
                    null,
                    {
                        role: "user",
                        input_type: 0,
                        prompt_input: "Hello"
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

                if (loraInferResponse.error) {
                    console.log('❌ LoRa inference failed:', loraInferResponse.error.message);
                } else {
                    console.log('✅ LoRa inference works!');
                }
            }
        }

        // === PHASE 3: Test Logits Mode ===
        console.log('\n🎯 PHASE 3: Testing logits mode (with timeout protection)');
        
        // Switch back to normal model for logits test
        await client.sendRequest('rkllm.destroy', [null]);
        await new Promise(resolve => setTimeout(resolve, 1000));

        const logitsInitResponse = await client.sendRequest('rkllm.init', [{
            model_path: "/home/x/Projects/nano/models/qwen3/model.rkllm",
            max_context_len: 256,
            max_new_tokens: 20
        }]);

        if (logitsInitResponse.error) {
            console.log('❌ Logits model init failed:', logitsInitResponse.error.message);
        } else {
            console.log('✅ Model reloaded for logits test');

            try {
                const logitsResponse = await client.sendRequest('rkllm.run', [
                    null,
                    {
                        role: "user",
                        input_type: 0,
                        prompt_input: "Hi"
                    },
                    {
                        mode: 2, // RKLLM_INFER_GET_LOGITS
                        keep_history: 0
                    },
                    null
                ], 10000); // 10 second timeout

                if (logitsResponse.error) {
                    console.log('❌ Logits mode failed:', logitsResponse.error.message);
                } else {
                    console.log('✅ Logits mode works!');
                    console.log('Logits data available:', !!logitsResponse.result.logits);
                }

            } catch (timeoutError) {
                console.log('⏰ Logits mode TIMEOUT - this confirms the hanging issue');
                console.log('   The RKLLM library hangs when extracting logits');
            }
        }

        console.log('\n📊 DIAGNOSTIC SUMMARY:');
        console.log('======================');
        console.log('This test will show which components work and which need fixing.');

    } catch (error) {
        console.error('💥 Diagnostic failed:', error.message);
    } finally {
        client.disconnect();
        if (server) {
            server.kill();
        }
    }
}

// Run diagnostic
diagnosticTest();