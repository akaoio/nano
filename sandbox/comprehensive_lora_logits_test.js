#!/usr/bin/env node

const net = require('net');
const fs = require('fs');

class RKLLMClient {
    constructor(socketPath = '/tmp/rkllm.sock') {
        this.socketPath = socketPath;
        this.socket = null;
        this.connected = false;
        this.pendingRequests = new Map();
        this.requestId = 1;
    }

    connect() {
        return new Promise((resolve, reject) => {
            this.socket = net.createConnection(this.socketPath);
            
            this.socket.on('connect', () => {
                this.connected = true;
                console.log('✓ Connected to RKLLM server');
                resolve();
            });

            this.socket.on('data', (data) => {
                const responses = data.toString().trim().split('\n');
                responses.forEach(responseStr => {
                    if (responseStr.trim()) {
                        try {
                            const response = JSON.parse(responseStr);
                            this.handleResponse(response);
                        } catch (e) {
                            console.log('Raw response:', responseStr);
                        }
                    }
                });
            });

            this.socket.on('error', (err) => {
                console.error('Socket error:', err.message);
                reject(err);
            });

            this.socket.on('close', () => {
                this.connected = false;
                console.log('Disconnected from server');
            });
        });
    }

    handleResponse(response) {
        if (response.id && this.pendingRequests.has(response.id)) {
            const { resolve, reject } = this.pendingRequests.get(response.id);
            this.pendingRequests.delete(response.id);
            
            if (response.error) {
                reject(new Error(`Server error: ${response.error.message || JSON.stringify(response.error)}`));
            } else {
                resolve(response.result);
            }
        } else {
            // Handle streaming responses or unmatched responses
            console.log('Server response:', JSON.stringify(response, null, 2));
        }
    }

    request(method, params, timeout = 60000) {
        return new Promise((resolve, reject) => {
            if (!this.connected) {
                return reject(new Error('Not connected to server'));
            }

            const id = this.requestId++;
            const request = {
                jsonrpc: "2.0",
                id,
                method,
                params
            };

            // Set up timeout
            const timeoutId = setTimeout(() => {
                if (this.pendingRequests.has(id)) {
                    this.pendingRequests.delete(id);
                    reject(new Error(`Request timed out after ${timeout}ms`));
                }
            }, timeout);

            this.pendingRequests.set(id, {
                resolve: (result) => {
                    clearTimeout(timeoutId);
                    resolve(result);
                },
                reject: (error) => {
                    clearTimeout(timeoutId);
                    reject(error);
                }
            });

            this.socket.write(JSON.stringify(request) + '\n');
        });
    }

    disconnect() {
        if (this.socket) {
            this.socket.end();
        }
    }
}

async function testLoRaWorkflow() {
    const client = new RKLLMClient();
    
    try {
        console.log('=== LoRa Workflow Test ===\n');
        
        // Check if files exist
        const modelPath = '/home/x/Projects/nano/models/lora/model.rkllm';
        const loraPath = '/home/x/Projects/nano/models/lora/lora.rkllm';
        
        console.log('Checking required files...');
        if (!fs.existsSync(modelPath)) {
            throw new Error(`Base model not found: ${modelPath}`);
        }
        if (!fs.existsSync(loraPath)) {
            throw new Error(`LoRa adapter not found: ${loraPath}`);
        }
        console.log('✓ All required files found\n');

        await client.connect();

        // Step 1: Initialize model
        console.log('1. Initializing base model...');
        const initResult = await client.request('rkllm.init', [
            null,
            {
                model_path: modelPath,
                max_context_len: 512,
                max_new_tokens: 256,
                top_k: 1,
                top_p: 0.9,
                temperature: 0.8,
                repeat_penalty: 1.1,
                frequency_penalty: 0.0,
                presence_penalty: 0.0
            },
            null
        ], 90000); // 90 second timeout for model loading
        
        console.log('✓ Model initialized:', initResult);

        // Step 2: Load LoRa adapter
        console.log('\n2. Loading LoRa adapter...');
        const loraResult = await client.request('rkllm.load_lora', [
            null,
            {
                lora_adapter_path: loraPath,
                lora_adapter_name: "test_adapter",
                scale: 1.0
            }
        ]);
        
        console.log('✓ LoRa adapter loaded:', loraResult);

        // Step 3: Test inference with LoRa
        console.log('\n3. Testing inference with LoRa...');
        const inferenceResult = await client.request('rkllm.run', [
            null,
            {
                role: "user",
                input_type: 0,
                prompt_input: "Hello, how are you today?"
            },
            {
                mode: 0,
                lora_params: {
                    lora_adapter_name: "test_adapter"
                },
                keep_history: 0
            },
            null
        ], 30000);
        
        console.log('✓ LoRa inference result:', inferenceResult);
        
        return true;
    } catch (error) {
        console.error('✗ LoRa test failed:', error.message);
        return false;
    } finally {
        client.disconnect();
    }
}

async function testLogitsExtraction() {
    const client = new RKLLMClient();
    
    try {
        console.log('\n=== Logits Extraction Test ===\n');
        
        await client.connect();

        // Test logits extraction (note: this may timeout on some models)
        console.log('Testing logits extraction...');
        const logitsResult = await client.request('rkllm.run', [
            null,
            {
                role: "user",
                input_type: 0,
                prompt_input: "The capital of France is"
            },
            {
                mode: 2, // Logits mode
                keep_history: 0
            },
            null
        ], 15000); // Shorter timeout for logits test
        
        console.log('✓ Logits extraction result:');
        if (logitsResult.logits && Array.isArray(logitsResult.logits)) {
            console.log(`   - Text: "${logitsResult.text}"`);
            console.log(`   - Logits array length: ${logitsResult.logits.length}`);
            console.log(`   - Sample logits: [${logitsResult.logits.slice(0, 5).join(', ')}...]`);
            console.log(`   - Vocab size: ${logitsResult.vocab_size || 'unknown'}`);
        } else {
            console.log('   - Raw result:', logitsResult);
        }
        
        return true;
    } catch (error) {
        console.error('✗ Logits test failed:', error.message);
        if (error.message.includes('timed out')) {
            console.log('   Note: Logits mode may not be supported by this model');
        }
        return false;
    } finally {
        client.disconnect();
    }
}

async function testCombinedLoRaLogits() {
    const client = new RKLLMClient();
    
    try {
        console.log('\n=== Combined LoRa + Logits Test ===\n');
        
        await client.connect();

        // Test combining LoRa with logits extraction
        console.log('Testing LoRa + logits combination...');
        const combinedResult = await client.request('rkllm.run', [
            null,
            {
                role: "user",
                input_type: 0,
                prompt_input: "Explain artificial intelligence"
            },
            {
                mode: 2, // Logits mode
                lora_params: {
                    lora_adapter_name: "test_adapter"
                },
                keep_history: 0
            },
            null
        ], 15000);
        
        console.log('✓ Combined LoRa + logits result:');
        console.log(`   - Text: "${combinedResult.text}"`);
        if (combinedResult.logits) {
            console.log(`   - Logits available: ${combinedResult.logits.length} values`);
        }
        
        return true;
    } catch (error) {
        console.error('✗ Combined test failed:', error.message);
        return false;
    } finally {
        client.disconnect();
    }
}

async function runComprehensiveTest() {
    console.log('🚀 Starting Comprehensive LoRa and Logits Test\n');
    
    const results = {
        lora: false,
        logits: false,
        combined: false
    };
    
    // Test LoRa workflow
    results.lora = await testLoRaWorkflow();
    
    // Test logits extraction (only if LoRa worked)
    if (results.lora) {
        results.logits = await testLogitsExtraction();
        
        // Test combined functionality (only if both worked)
        if (results.logits) {
            results.combined = await testCombinedLoRaLogits();
        }
    }
    
    // Summary
    console.log('\n=== Test Summary ===');
    console.log(`LoRa functionality: ${results.lora ? '✓ PASS' : '✗ FAIL'}`);
    console.log(`Logits extraction: ${results.logits ? '✓ PASS' : '✗ FAIL'}`);
    console.log(`Combined LoRa+Logits: ${results.combined ? '✓ PASS' : '✗ FAIL'}`);
    
    if (results.lora && results.logits && results.combined) {
        console.log('\n🎉 All tests passed! LoRa and logits are working correctly.');
    } else if (results.lora && !results.logits) {
        console.log('\n⚠️  LoRa works, but logits mode may not be supported by this model.');
    } else if (!results.lora) {
        console.log('\n❌ LoRa functionality failed. Check model files and server status.');
    }
    
    return results;
}

// Run the test if this script is executed directly
if (require.main === module) {
    runComprehensiveTest().catch(console.error);
}

module.exports = {
    RKLLMClient,
    testLoRaWorkflow,
    testLogitsExtraction,
    testCombinedLoRaLogits,
    runComprehensiveTest
};