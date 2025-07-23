#!/usr/bin/env node

/**
 * Debug streaming responses to understand callback behavior
 */

const net = require('net');
const { spawn } = require('child_process');
const path = require('path');

async function debugStreaming() {
  console.log('🔍 DEBUG STREAMING - Understanding Callback Behavior');
  console.log('=' .repeat(60));

  // Start server
  console.log('🚀 Starting server...');
  const serverPath = path.join(__dirname, '../build/rkllm_uds_server');
  const server = spawn(serverPath, [], {
    stdio: ['pipe', 'pipe', 'pipe'],
    env: {
      ...process.env,
      LD_LIBRARY_PATH: path.join(__dirname, '../src/external/rkllm')
    }
  });

  // Wait for server to start
  await new Promise(resolve => setTimeout(resolve, 2000));
  console.log('✅ Server started');

  // Connect to server
  console.log('🔗 Connecting...');
  const socket = net.createConnection('/tmp/rkllm.sock');
  
  await new Promise((resolve, reject) => {
    socket.on('connect', () => {
      console.log('✅ Connected');
      resolve();
    });
    socket.on('error', reject);
  });

  // Step 1: Initialize model
  console.log('\n🧪 STEP 1: Initialize Model');
  console.log('=' .repeat(40));
  
  const initRequest = {
    jsonrpc: '2.0',
    method: 'rkllm.init',
    params: [
      null,
      {
        model_path: '/home/x/Projects/nano/models/qwen3/model.rkllm',
        max_context_len: 512,
        max_new_tokens: 50, // More tokens to see streaming
        top_k: 40,
        n_keep: 0,
        top_p: 0.9,
        temperature: 0.8,
        repeat_penalty: 1.1,
        frequency_penalty: 0,
        presence_penalty: 0,
        mirostat: 0,
        mirostat_tau: 5,
        mirostat_eta: 0.1,
        skip_special_token: false,
        is_async: false,
        img_start: null,
        img_end: null,
        img_content: null,
        extend_param: {
          base_domain_id: 0,
          embed_flash: 0,
          enabled_cpus_num: 4,
          enabled_cpus_mask: 15,
          n_batch: 1,
          use_cross_attn: 0,
          reserved: null
        }
      },
      null
    ],
    id: 1
  };
  
  console.log('📤 Sending init request...');
  socket.write(JSON.stringify(initRequest) + '\n');
  
  // Wait for init response
  let initData = '';
  await new Promise((resolve) => {
    const handler = (data) => {
      initData += data.toString();
      try {
        const response = JSON.parse(initData.trim());
        if (response.id === 1) {
          console.log('✅ Init response:', JSON.stringify(response, null, 2));
          socket.removeListener('data', handler);
          resolve();
        }
      } catch (e) {
        // Continue accumulating
      }
    };
    socket.on('data', handler);
  });
  
  // Wait for model to be ready
  await new Promise(resolve => setTimeout(resolve, 3000));

  // Step 2: Test inference with streaming observation
  console.log('\n🧪 STEP 2: Test Inference with Streaming Observation');
  console.log('=' .repeat(50));
  
  const runRequest = {
    jsonrpc: '2.0',
    method: 'rkllm.run',
    params: [
      null,
      {
        role: 'user',
        enable_thinking: false,
        input_type: 0,
        prompt_input: 'Write a short story about a robot learning to paint. Make it at least 3 sentences long.'
      },
      {
        mode: 0,
        lora_params: null,
        prompt_cache_params: null,
        keep_history: 0
      },
      null
    ],
    id: 2
  };
  
  console.log('📤 Sending run request for longer text generation...');
  console.log('🔍 Watching for ALL streaming responses...');
  
  let streamingData = '';
  let responseCount = 0;
  let allResponses = [];
  
  const streamHandler = (data) => {
    const chunk = data.toString();
    streamingData += chunk;
    
    console.log(`📥 Raw chunk ${++responseCount}:`, JSON.stringify(chunk));
    
    // Try to parse each complete JSON line
    const lines = streamingData.split('\n');
    
    for (let i = 0; i < lines.length - 1; i++) {
      const line = lines[i].trim();
      if (!line) continue;
      
      try {
        const response = JSON.parse(line);
        
        if (response.id === 2) {
          console.log(`✅ Parsed response ${allResponses.length + 1}:`, JSON.stringify(response, null, 2));
          allResponses.push(response);
          
          // Check if this is the final response
          if (response.result && response.result._callback_state === 2) { // RKLLM_RUN_FINISH
            console.log('🏁 Final response detected');
            socket.removeListener('data', streamHandler);
            analyzeResults();
          }
        }
      } catch (e) {
        console.log('⚠️  Could not parse line:', line);
      }
    }
    
    // Keep the last incomplete line
    streamingData = lines[lines.length - 1] || '';
  };

  socket.on('data', streamHandler);
  socket.write(JSON.stringify(runRequest) + '\n');
  
  // Timeout after 30 seconds
  setTimeout(() => {
    console.log('⏰ Analysis timeout reached');
    socket.removeListener('data', streamHandler);
    analyzeResults();
  }, 30000);

  function analyzeResults() {
    console.log('\n📊 STREAMING ANALYSIS RESULTS:');
    console.log('=' .repeat(50));
    
    console.log(`Total responses received: ${allResponses.length}`);
    console.log(`Total raw chunks: ${responseCount}`);
    
    if (allResponses.length > 0) {
      console.log('\n🔍 Response Analysis:');
      
      allResponses.forEach((response, index) => {
        const result = response.result;
        if (result) {
          console.log(`\nResponse ${index + 1}:`);
          console.log(`  Text: "${result.text || 'N/A'}"`);
          console.log(`  Token ID: ${result.token_id || 'N/A'}`);
          console.log(`  Callback State: ${result._callback_state} (${getStateString(result._callback_state)})`);
          console.log(`  Performance: prefill=${result.perf?.prefill_tokens || 0}, generate=${result.perf?.generate_tokens || 0}`);
        }
      });
      
      // Check if we got multiple tokens or just one
      const texts = allResponses.map(r => r.result?.text).filter(t => t);
      const uniqueTexts = [...new Set(texts)];
      
      console.log('\n🎯 KEY FINDINGS:');
      console.log(`- Unique text pieces: ${uniqueTexts.length}`);
      console.log(`- Text pieces: [${uniqueTexts.map(t => `"${t}"`).join(', ')}]`);
      
      if (uniqueTexts.length === 1) {
        console.log('❌ PROBLEM: Only received 1 unique text piece - not proper streaming!');
      } else {
        console.log('✅ SUCCESS: Received multiple text pieces - proper streaming working!');
      }
      
    } else {
      console.log('❌ No responses received!');
    }
    
    // Cleanup
    socket.end();
    server.kill();
    
    console.log('\n🏁 Debug analysis complete');
  }
  
  function getStateString(state) {
    const states = {
      0: 'RKLLM_RUN_NORMAL',
      1: 'RKLLM_RUN_WAITING',
      2: 'RKLLM_RUN_FINISH',
      3: 'RKLLM_RUN_ERROR'
    };
    return states[state] || 'UNKNOWN';
  }
}

debugStreaming().catch(error => {
  console.error('❌ Debug streaming failed:', error);
  process.exit(1);
});