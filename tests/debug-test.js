#!/usr/bin/env node

/**
 * Debug Test - Isolate connection and response issues
 */

const net = require('net');
const { spawn } = require('child_process');
const path = require('path');

async function debugTest() {
  console.log('🔍 DEBUG TEST - Isolating Issues');
  console.log('=' .repeat(50));

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

  // Test 1: Simple request with manual response handling
  console.log('\n🧪 TEST 1: Manual createDefaultParam');
  console.log('=' .repeat(40));
  
  const request = {
    jsonrpc: '2.0',
    method: 'rkllm.createDefaultParam',
    params: [],
    id: 1
  };
  
  console.log('📤 Sending:', JSON.stringify(request));
  socket.write(JSON.stringify(request) + '\n');
  
  // Listen for any data
  let responseData = '';
  let dataCount = 0;
  
  const dataHandler = (data) => {
    dataCount++;
    const chunk = data.toString();
    responseData += chunk;
    
    console.log(`📥 Data chunk ${dataCount}:`, JSON.stringify(chunk));
    
    // Try to parse as JSON
    const lines = responseData.split('\n');
    for (const line of lines) {
      if (line.trim()) {
        try {
          const parsed = JSON.parse(line);
          console.log('✅ Parsed JSON:', JSON.stringify(parsed, null, 2));
          
          if (parsed.id === request.id) {
            console.log('✅ Response received for our request!');
            socket.removeListener('data', dataHandler);
            testComplete();
            return;
          }
        } catch (e) {
          console.log('⚠️  Could not parse as JSON:', line);
        }
      }
    }
  };

  socket.on('data', dataHandler);
  
  // Timeout after 10 seconds
  setTimeout(() => {
    console.log('⏰ Timeout reached');
    socket.removeListener('data', dataHandler);
    testComplete();
  }, 10000);

  function testComplete() {
    console.log('\n📊 Debug Results:');
    console.log(`Total data chunks received: ${dataCount}`);
    console.log(`Raw response data: ${JSON.stringify(responseData)}`);
    
    // Cleanup
    socket.end();
    server.kill();
    
    console.log('\n🏁 Debug test complete');
  }
}

debugTest().catch(error => {
  console.error('❌ Debug test failed:', error);
  process.exit(1);
});