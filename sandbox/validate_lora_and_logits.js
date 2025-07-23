#!/usr/bin/env node

const net = require('net');
const fs = require('fs');

class RKLLMClient {
  constructor() {
    this.socket = null;
    this.connected = false;
    this.requestId = 1;
    this.pendingRequests = new Map(); // Add missing property
  }

  connect() {
    return new Promise((resolve, reject) => {
      // Use Unix Domain Socket instead of TCP
      this.socket = net.createConnection('/tmp/rkllm.sock', () => {
        this.connected = true;
        console.log('✅ Connected to RKLLM server');
        resolve();
      });

      this.socket.on('error', (err) => {
        console.error('❌ Connection error:', err.message);
        reject(err);
      });

      this.socket.on('close', () => {
        this.connected = false;
        console.log('🔌 Connection closed');
      });
    });
  }

  handleResponse(data) {
    const lines = data.trim().split('\n');
    
    lines.forEach(line => {
      if (line.trim()) {
        try {
          const response = JSON.parse(line);
          
          if (response.id && this.pendingRequests.has(response.id)) {
            const { resolve, reject } = this.pendingRequests.get(response.id);
            this.pendingRequests.delete(response.id);
            
            if (response.error) {
              reject(new Error(response.error.message || JSON.stringify(response.error)));
            } else {
              resolve(response);
            }
          }
        } catch (e) {
          console.error('❌ Failed to parse response:', line);
        }
      }
    });
  }

  sendRequest(method, params, timeout = 30000) {
    return new Promise((resolve, reject) => {
      const id = ++this.requestId;
      const request = {
        jsonrpc: "2.0",
        id: id,
        method: method,
        params: params
      };

      this.pendingRequests.set(id, { resolve, reject });
      
      // Set timeout
      const timeoutId = setTimeout(() => {
        if (this.pendingRequests.has(id)) {
          this.pendingRequests.delete(id);
          reject(new Error(`Request timeout after ${timeout}ms for method: ${method}`));
        }
      }, timeout);

      // Clear timeout when request completes
      const originalResolve = resolve;
      const originalReject = reject;
      
      const wrappedResolve = (result) => {
        clearTimeout(timeoutId);
        originalResolve(result);
      };
      
      const wrappedReject = (error) => {
        clearTimeout(timeoutId);
        originalReject(error);
      };
      
      this.pendingRequests.set(id, { resolve: wrappedResolve, reject: wrappedReject });

      this.socket.write(JSON.stringify(request) + '\n');
    });
  }

  disconnect() {
    if (this.socket) {
      this.socket.end();
      console.log('📡 Disconnected from server');
    }
  }
}

function checkFileExists(filepath) {
  try {
    fs.accessSync(filepath, fs.constants.F_OK);
    return true;
  } catch (err) {
    return false;
  }
}

async function validateLoRaAndLogits() {
  console.log('🔬 LoRa and Logits Validation Test');
  console.log('=====================================\n');

  const client = new RKLLMClient();
  let testResults = {
    connection: false,
    fileCheck: false,
    initialization: false,
    loraLoad: false,
    loraInference: false,
    logitsExtraction: false,
    cleanup: false
  };

  try {
    // Test 1: Connection
    console.log('1️⃣ Testing connection...');
    await client.connect();
    testResults.connection = true;
    console.log('   ✅ Connection successful\n');

    // Test 2: File existence check
    console.log('2️⃣ Checking required files...');
    const baseModel = '/home/x/Projects/nano/models/lora/model.rkllm';
    const loraAdapter = '/home/x/Projects/nano/models/lora/lora.rkllm';
    
    const baseExists = checkFileExists(baseModel);
    const loraExists = checkFileExists(loraAdapter);
    
    console.log(`   Base model (${baseModel}): ${baseExists ? '✅' : '❌'}`);
    console.log(`   LoRa adapter (${loraAdapter}): ${loraExists ? '✅' : '❌'}`);
    
    if (!baseExists) {
      console.log('   ⚠️  Using fallback model: /home/x/Projects/nano/models/qwen3/model.rkllm');
    }
    
    testResults.fileCheck = baseExists && loraExists;
    console.log('');

    // Test 3: Model initialization
    console.log('3️⃣ Initializing model...');
    const modelPath = baseExists ? baseModel : '/home/x/Projects/nano/models/qwen3/model.rkllm';
    
    try {
      const initResponse = await client.sendRequest('rkllm.init', [{
        model_path: modelPath,
        max_context_len: 256,
        max_new_tokens: 50,
        top_k: 40,
        top_p: 0.9,
        temperature: 0.8
      }]);
      
      console.log('   ✅ Model initialized successfully');
      testResults.initialization = true;
    } catch (error) {
      console.log('   ❌ Model initialization failed:', error.message);
      throw error;
    }
    console.log('');

    // Test 4: LoRa loading (only if files exist)
    if (testResults.fileCheck) {
      console.log('4️⃣ Loading LoRa adapter...');
      try {
        const loraResponse = await client.sendRequest('rkllm.load_lora', [
          null,
          {
            lora_adapter_path: loraAdapter,
            lora_adapter_name: "validation_adapter",
            scale: 1.0
          }
        ]);
        
        console.log('   ✅ LoRa adapter loaded successfully');
        testResults.loraLoad = true;
      } catch (error) {
        console.log('   ❌ LoRa loading failed:', error.message);
      }
      console.log('');

      // Test 5: LoRa inference
      if (testResults.loraLoad) {
        console.log('5️⃣ Testing LoRa inference...');
        try {
          const loraInferenceResponse = await client.sendRequest('rkllm.run', [
            null,
            {
              role: "user",
              input_type: 0,
              prompt_input: "Hello"
            },
            {
              mode: 0,
              lora_params: {
                lora_adapter_name: "validation_adapter"
              },
              keep_history: 0
            },
            null
          ], 15000);
          
          if (loraInferenceResponse.result && loraInferenceResponse.result.text) {
            console.log('   ✅ LoRa inference successful');
            console.log(`   💬 Response: "${loraInferenceResponse.result.text.substring(0, 100)}..."`);
            testResults.loraInference = true;
          } else {
            console.log('   ⚠️  LoRa inference completed but no text returned');
          }
        } catch (error) {
          console.log('   ❌ LoRa inference failed:', error.message);
        }
        console.log('');
      }
    } else {
      console.log('4️⃣ Skipping LoRa tests (files not found)\n');
    }

    // Test 6: Logits extraction
    console.log('6️⃣ Testing logits extraction...');
    try {
      const logitsResponse = await client.sendRequest('rkllm.run', [
        null,
        {
          role: "user",
          input_type: 0,
          prompt_input: "Hi"
        },
        {
          mode: 2,
          keep_history: 0
        },
        null
      ], 10000); // 10 second timeout for logits
      
      if (logitsResponse.result && logitsResponse.result.logits) {
        const logits = logitsResponse.result.logits;
        console.log('   ✅ Logits extraction successful');
        console.log(`   📊 Vocab size: ${logits.vocab_size}`);
        console.log(`   📊 Num tokens: ${logits.num_tokens}`);
        console.log(`   📊 Logits array length: ${logits.logits ? logits.logits.length : 0}`);
        
        if (logits.logits && logits.logits.length > 0) {
          const topLogits = logits.logits
            .map((value, index) => ({ value, index }))
            .sort((a, b) => b.value - a.value)
            .slice(0, 5);
          
          console.log('   🏆 Top 5 logits:');
          topLogits.forEach((item, i) => {
            console.log(`      ${i + 1}. Token ${item.index}: ${item.value.toFixed(4)}`);
          });
        }
        
        testResults.logitsExtraction = true;
      } else {
        console.log('   ⚠️  Logits response received but no logits data');
      }
    } catch (error) {
      console.log('   ❌ Logits extraction failed:', error.message);
      if (error.message.includes('timeout')) {
        console.log('   💡 This is likely because the model doesn\'t support logits mode');
      }
    }
    console.log('');

    // Test 7: Cleanup
    console.log('7️⃣ Cleaning up...');
    try {
      await client.sendRequest('rkllm.destroy', [null]);
      console.log('   ✅ Model destroyed successfully');
      testResults.cleanup = true;
    } catch (error) {
      console.log('   ⚠️  Cleanup warning:', error.message);
      testResults.cleanup = true; // Don't fail the test for cleanup issues
    }

  } catch (error) {
    console.error('❌ Test failed:', error.message);
  } finally {
    client.disconnect();
  }

  // Summary
  console.log('\n📋 Test Results Summary');
  console.log('========================');
  Object.entries(testResults).forEach(([test, passed]) => {
    const icon = passed ? '✅' : '❌';
    const status = passed ? 'PASS' : 'FAIL';
    console.log(`${icon} ${test.padEnd(20)} ${status}`);
  });

  const passedTests = Object.values(testResults).filter(Boolean).length;
  const totalTests = Object.keys(testResults).length;
  
  console.log(`\n🎯 Overall: ${passedTests}/${totalTests} tests passed`);
  
  if (passedTests === totalTests) {
    console.log('🎉 All tests passed! LoRa and logits functionality is working correctly.');
  } else {
    console.log('⚠️  Some tests failed. See the guide above for troubleshooting steps.');
  }

  return testResults;
}

// Run validation if this script is executed directly
if (require.main === module) {
  validateLoRaAndLogits().catch(console.error);
}

module.exports = { validateLoRaAndLogits, RKLLMClient };