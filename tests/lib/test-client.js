const net = require('net');

class TestClient {
  constructor(socketPath = '/tmp/rkllm.sock') {
    this.socketPath = socketPath;
    this.socket = null;
    this.requestId = 1;
    this.isConnected = false;
  }

  async connect() {
    return new Promise((resolve, reject) => {
      this.socket = net.createConnection(this.socketPath);
      
      this.socket.on('connect', () => {
        console.log('🔗 Connected to RKLLM server');
        this.isConnected = true;
        resolve();
      });

      this.socket.on('error', (error) => {
        console.error('❌ Connection error:', error.message);
        this.isConnected = false;
        reject(error);
      });

      this.socket.on('close', () => {
        console.log('🔌 Connection closed');
        this.isConnected = false;
      });
    });
  }

  disconnect() {
    if (this.socket) {
      this.socket.end();
      this.socket = null;
      this.isConnected = false;
    }
  }

  /**
   * RAW JSON STREAMING - Shows the actual JSON-RPC responses being sent (COMPACT MODE)
   */
  async sendRawJsonStreamingRequest(method, params = []) {
    if (!this.isConnected || !this.socket) {
      throw new Error('Not connected to server');
    }

    const request = {
      jsonrpc: '2.0',
      method: method,
      params: params,
      id: this.requestId++
    };

    console.log(`📤 RAW JSON STREAMING REQUEST: ${method}`);
    
    return new Promise((resolve, reject) => {
      let tokens = [];
      let fullText = '';
      let finalPerf = null;
      let responseData = '';
      let timeout = null;
      let dataHandler = null;
      let chunkCount = 0;

      // Clean up function
      const cleanup = () => {
        if (timeout) {
          clearTimeout(timeout);
          timeout = null;
        }
        if (dataHandler && this.socket) {
          this.socket.removeListener('data', dataHandler);
        }
      };

      // COMPACT JSON DATA HANDLER - Shows minimal info about chunks
      dataHandler = (data) => {
        responseData += data.toString();
        
        // Process each complete line
        const lines = responseData.split('\n');
        responseData = lines.pop() || ''; // Keep incomplete line for next chunk
        
        for (const line of lines) {
          if (!line.trim()) continue;
          
          try {
            const response = JSON.parse(line.trim());
            
            // Check if this is our streaming response
            if (response.id === request.id && response.result) {
              chunkCount++;
              
              // COMPACT: Only show brief summary every 10 chunks
              if (chunkCount % 10 === 0 || chunkCount <= 3) {
                console.log(`      📦 Chunk ${chunkCount}: "${response.result.text}" (token_id: ${response.result.token_id})`);
              }
              
              // Show final completion state
              if (response.result._callback_state === 2) { // RKLLM_RUN_FINISH
                finalPerf = response.result.perf;
                console.log(`\n      ✅ STREAMING COMPLETE! Received ${chunkCount} JSON-RPC responses`);
                cleanup();
                resolve({
                  tokens: tokens,
                  fullText: fullText,
                  finalPerf: finalPerf
                });
                return;
              }
              
              // Extract and accumulate text for summary
              if (response.result.text !== null && response.result.text !== undefined) {
                tokens.push({
                  text: response.result.text,
                  token_id: response.result.token_id,
                  callback_state: response.result._callback_state
                });
                fullText += response.result.text;
              }
            }
          } catch (e) {
            // Invalid JSON line, continue
          }
        }
      };

      // Set up handlers
      this.socket.on('data', dataHandler);

      // Set longer timeout for streaming
      timeout = setTimeout(() => {
        cleanup();
        if (tokens.length > 0) {
          // We got some tokens, return partial result
          console.log(`\n⏰ TIMEOUT after ${chunkCount} JSON responses`);
          resolve({
            tokens: tokens,
            fullText: fullText,
            finalPerf: finalPerf
          });
        } else {
          reject(new Error('Raw JSON streaming timeout - no responses received'));
        }
      }, 15000); // 15 second timeout

      // Send request and start compact JSON display
      try {
        console.log('      🎬 LIVE JSON STREAMING (compact mode - each chunk verified as complete JSON-RPC):');
        this.socket.write(JSON.stringify(request) + '\n');
      } catch (error) {
        cleanup();
        reject(new Error(`Failed to send raw JSON streaming request: ${error.message}`));
      }
    });
  }

  /**
   * REAL-TIME STREAMING - Shows tokens instantly as they appear
   */
  async sendRealTimeStreamingRequest(method, params = []) {
    if (!this.isConnected || !this.socket) {
      throw new Error('Not connected to server');
    }

    const request = {
      jsonrpc: '2.0',
      method: method,
      params: params,
      id: this.requestId++
    };

    console.log(`📤 REAL-TIME REQUEST: ${method}`);
    
    return new Promise((resolve, reject) => {
      let tokens = [];
      let fullText = '';
      let finalPerf = null;
      let responseData = '';
      let timeout = null;
      let dataHandler = null;
      let tokenCount = 0;

      // Clean up function
      const cleanup = () => {
        if (timeout) {
          clearTimeout(timeout);
          timeout = null;
        }
        if (dataHandler && this.socket) {
          this.socket.removeListener('data', dataHandler);
        }
      };

      // REAL-TIME DATA HANDLER - Shows tokens instantly
      dataHandler = (data) => {
        responseData += data.toString();
        
        // Process each complete line
        const lines = responseData.split('\n');
        responseData = lines.pop() || ''; // Keep incomplete line for next chunk
        
        for (const line of lines) {
          if (!line.trim()) continue;
          
          try {
            const response = JSON.parse(line.trim());
            
            // Check if this is our streaming response
            if (response.id === request.id && response.result) {
              // Show final completion state
              if (response.result._callback_state === 2) { // RKLLM_RUN_FINISH
                finalPerf = response.result.perf;
                console.log(`\n      ✅ STREAMING COMPLETE! Generated ${tokenCount} tokens`);
                if (finalPerf) {
                  console.log(`      📊 Performance: ${finalPerf.generate_time_ms}ms for ${finalPerf.generate_tokens} tokens`);
                  console.log(`      💾 Memory: ${finalPerf.memory_usage_mb}MB`);
                }
                cleanup();
                resolve({
                  tokens: tokens,
                  fullText: fullText,
                  finalPerf: finalPerf
                });
                return;
              }
              
              // INSTANT TOKEN DISPLAY - Show each token as it arrives
              if (response.result.text !== null && response.result.text !== undefined) {
                tokenCount++;
                
                // Display token instantly
                process.stdout.write(response.result.text);
                
                // Show token details occasionally for verification
                if (tokenCount % 10 === 0) {
                  console.log(`\n      [Token ${tokenCount}: ID=${response.result.token_id}]`);
                }
                
                tokens.push({
                  text: response.result.text,
                  token_id: response.result.token_id,
                  callback_state: response.result._callback_state
                });
                fullText += response.result.text;
              }
            }
          } catch (e) {
            // Invalid JSON line, continue
          }
        }
      };

      // Set up handlers
      this.socket.on('data', dataHandler);

      // Set longer timeout for streaming
      timeout = setTimeout(() => {
        cleanup();
        if (tokens.length > 0) {
          // We got some tokens, return partial result
          console.log(`\n      ⏰ TIMEOUT after ${tokens.length} tokens`);
          resolve({
            tokens: tokens,
            fullText: fullText,
            finalPerf: finalPerf
          });
        } else {
          reject(new Error('Real-time streaming timeout - no tokens received'));
        }
      }, 20000); // 20 second timeout

      // Send request and start real-time display
      try {
        console.log('      🎬 LIVE STREAMING (tokens will appear instantly):');
        console.log('      ▶️  ');
        this.socket.write(JSON.stringify(request) + '\n');
      } catch (error) {
        cleanup();
        reject(new Error(`Failed to send real-time streaming request: ${error.message}`));
      }
    });
  }

  /**
   * Send request and wait for complete response with proper cleanup
   */
  async sendRequest(method, params = []) {
    if (!this.isConnected || !this.socket) {
      throw new Error('Not connected to server');
    }

    const request = {
      jsonrpc: '2.0',
      method: method,
      params: params,
      id: this.requestId++
    };

    console.log('📤 REQUEST:', JSON.stringify(request, null, 2));

    return new Promise((resolve, reject) => {
      let responseData = '';
      let timeout = null;
      let dataHandler = null;

      // Clean up function
      const cleanup = () => {
        if (timeout) {
          clearTimeout(timeout);
          timeout = null;
        }
        if (dataHandler && this.socket) {
          this.socket.removeListener('data', dataHandler);
        }
      };

      // Data handler - handle both line-based and complete JSON responses
      dataHandler = (data) => {
        responseData += data.toString();
        
        // Try to parse the entire accumulated data as JSON first
        try {
          const response = JSON.parse(responseData.trim());
          if (response.id === request.id) {
            cleanup();
            const trimmedResponse = this.trimLongArrays(response);
            console.log('📥 RESPONSE:', JSON.stringify(trimmedResponse, null, 2));
            resolve(response);
            return;
          }
        } catch (e) {
          // Not complete JSON yet, try line-by-line parsing
        }
        
        // Try to parse each line as a separate JSON response
        const lines = responseData.split('\n');
        
        for (let i = 0; i < lines.length; i++) {
          const line = lines[i].trim();
          if (!line) continue;
          
          try {
            const response = JSON.parse(line);
            
            // Check if this is the response to our request
            if (response.id === request.id) {
              cleanup();
              const trimmedResponse = this.trimLongArrays(response);
              resolve(response);
              return;
            }
            // Otherwise it might be a streaming response, ignore it for now
          } catch (e) {
            // Invalid JSON line, continue
            continue;
          }
        }
      };

      // Set up handlers
      this.socket.on('data', dataHandler);

      // Set timeout with method-specific timeouts
      const timeoutMs = method.includes('logits') ? 8000 : 15000; // Shorter timeout for logits mode
      timeout = setTimeout(() => {
        cleanup();
        reject(new Error(`Request timeout (${timeoutMs}ms) for method: ${method}`));
      }, timeoutMs);

      // Send request
      try {
        this.socket.write(JSON.stringify(request) + '\n');
      } catch (error) {
        cleanup();
        reject(new Error(`Failed to send request: ${error.message}`));
      }
    });
  }

  /**
   * Send request and wait for complete response with proper cleanup - SILENT VERSION
   */
  async sendRequestSilent(method, params = []) {
    if (!this.isConnected || !this.socket) {
      throw new Error('Not connected to server');
    }

    const request = {
      jsonrpc: '2.0',
      method: method,
      params: params,
      id: this.requestId++
    };

    // No console.log here - silent operation

    return new Promise((resolve, reject) => {
      let responseData = '';
      let timeout = null;
      let dataHandler = null;

      // Clean up function
      const cleanup = () => {
        if (timeout) {
          clearTimeout(timeout);
          timeout = null;
        }
        if (dataHandler && this.socket) {
          this.socket.removeListener('data', dataHandler);
        }
      };

      // Data handler - handle both line-based and complete JSON responses
      dataHandler = (data) => {
        responseData += data.toString();
        
        // Try to parse the entire accumulated data as JSON first
        try {
          const response = JSON.parse(responseData.trim());
          if (response.id === request.id) {
            cleanup();
            // No console.log here - silent operation
            resolve(response);
            return;
          }
        } catch (e) {
          // Not complete JSON yet, try line-by-line parsing
        }
        
        // Try to parse each line as a separate JSON response
        const lines = responseData.split('\n');
        
        for (let i = 0; i < lines.length; i++) {
          const line = lines[i].trim();
          if (!line) continue;
          
          try {
            const response = JSON.parse(line);
            
            // Check if this is the response to our request
            if (response.id === request.id) {
              cleanup();
              // No console.log here - silent operation
              resolve(response);
              return;
            }
            // Otherwise it might be a streaming response, ignore it for now
          } catch (e) {
            // Invalid JSON line, continue
            continue;
          }
        }
      };

      // Set up handlers
      this.socket.on('data', dataHandler);

      // Set timeout with method-specific timeouts
      const timeoutMs = method.includes('logits') ? 8000 : 15000; // Shorter timeout for logits mode
      timeout = setTimeout(() => {
        cleanup();
        reject(new Error(`Request timeout (${timeoutMs}ms) for method: ${method}`));
      }, timeoutMs);

      // Send request
      try {
        this.socket.write(JSON.stringify(request) + '\n');
      } catch (error) {
        cleanup();
        reject(new Error(`Failed to send request: ${error.message}`));
      }
    });
  }

  /**
   * Send streaming request and capture all tokens as they arrive
   */
  async sendStreamingRequest(method, params = []) {
    if (!this.isConnected || !this.socket) {
      throw new Error('Not connected to server');
    }

    const request = {
      jsonrpc: '2.0',
      method: method,
      params: params,
      id: this.requestId++
    };

    return new Promise((resolve, reject) => {
      let tokens = [];
      let fullText = '';
      let finalPerf = null;
      let responseData = '';
      let timeout = null;
      let dataHandler = null;
      let isComplete = false;

      // Clean up function
      const cleanup = () => {
        if (timeout) {
          clearTimeout(timeout);
          timeout = null;
        }
        if (dataHandler && this.socket) {
          this.socket.removeListener('data', dataHandler);
        }
      };

      // Data handler for streaming responses
      dataHandler = (data) => {
        responseData += data.toString();
        
        // Process each complete line
        const lines = responseData.split('\n');
        responseData = lines.pop() || ''; // Keep incomplete line for next chunk
        
        for (const line of lines) {
          if (!line.trim()) continue;
          
          try {
            const response = JSON.parse(line.trim());
            
            // Check if this is our streaming response
            if (response.id === request.id && response.result) {
              // Check for final completion state
              if (response.result._callback_state === 2) { // RKLLM_RUN_FINISH
                finalPerf = response.result.perf;
                isComplete = true;
                cleanup();
                resolve({
                  tokens: tokens,
                  fullText: fullText,
                  finalPerf: finalPerf
                });
                return;
              }
              
              // Add token to collection
              if (response.result.text !== null && response.result.text !== undefined) {
                tokens.push({
                  text: response.result.text,
                  token_id: response.result.token_id,
                  callback_state: response.result._callback_state
                });
                fullText += response.result.text;
              }
            }
          } catch (e) {
            // Invalid JSON line, continue
          }
        }
      };

      // Set up handlers
      this.socket.on('data', dataHandler);

      // Set longer timeout for streaming
      timeout = setTimeout(() => {
        cleanup();
        if (tokens.length > 0) {
          // We got some tokens, return partial result
          resolve({
            tokens: tokens,
            fullText: fullText,
            finalPerf: finalPerf
          });
        } else {
          reject(new Error('Streaming request timeout - no tokens received'));
        }
      }, 20000); // 20 second timeout for streaming

      // Send request
      try {
        this.socket.write(JSON.stringify(request) + '\n');
      } catch (error) {
        cleanup();
        reject(new Error(`Failed to send streaming request: ${error.message}`));
      }
    });
  }

  /**
   * Send raw request data (for testing malformed JSON handling)
   */
  async sendRawRequest(rawData) {
    if (!this.isConnected || !this.socket) {
      throw new Error('Not connected to server');
    }

    return new Promise((resolve, reject) => {
      let responseData = '';
      let timeout = null;
      let dataHandler = null;

      // Clean up function
      const cleanup = () => {
        if (timeout) {
          clearTimeout(timeout);
          timeout = null;
        }
        if (dataHandler && this.socket) {
          this.socket.removeListener('data', dataHandler);
        }
      };

      // Data handler for raw responses
      dataHandler = (data) => {
        responseData += data.toString();
        
        // For malformed JSON, we might get an error response or connection close
        try {
          const response = JSON.parse(responseData.trim());
          cleanup();
          resolve(response);
        } catch (e) {
          // Could be incomplete JSON, wait for more data
          // If timeout occurs, we'll handle it there
        }
      };

      // Handle connection errors/close for malformed requests
      const errorHandler = (error) => {
        cleanup();
        reject(error);
      };

      const closeHandler = () => {
        cleanup();
        reject(new Error('Connection closed - server rejected malformed request'));
      };

      // Set up handlers
      this.socket.on('data', dataHandler);
      this.socket.on('error', errorHandler);
      this.socket.on('close', closeHandler);

      // Set shorter timeout for malformed requests
      timeout = setTimeout(() => {
        cleanup();
        // Remove extra handlers
        this.socket.removeListener('error', errorHandler);
        this.socket.removeListener('close', closeHandler);
        
        if (responseData.length > 0) {
          // Got some response, but couldn't parse it
          resolve({ rawResponse: responseData });
        } else {
          reject(new Error('Raw request timeout - no response'));
        }
      }, 3000); // 3 second timeout for raw requests

      // Send raw request
      try {
        this.socket.write(rawData + '\n');
      } catch (error) {
        cleanup();
        this.socket.removeListener('error', errorHandler);
        this.socket.removeListener('close', closeHandler);
        reject(new Error(`Failed to send raw request: ${error.message}`));
      }
    });
  }

  /**
   * Test a specific RKLLM function with proper sequential execution
   */
  async testFunction(method, params, expected = {}) {
    console.log(`\n🧪 TESTING: ${method}`);
    console.log('=' .repeat(50));

    try {
      const response = await this.sendRequest(method, params);
      
      // Wait a moment between requests to ensure server is ready
      await new Promise(resolve => setTimeout(resolve, 500));
      
      if (response.error) {
        console.log('⚠️  EXPECTED ERROR:', response.error.message);
        return {
          success: false,
          error: response.error,
          response: response
        };
      } else if (response.result !== undefined) {
        console.log('✅ SUCCESS:', response.result);
        return {
          success: true,
          result: response.result,
          response: response
        };
      } else {
        console.log('❌ UNEXPECTED RESPONSE FORMAT');
        return {
          success: false,
          error: { message: 'Unexpected response format' },
          response: response
        };
      }
    } catch (error) {
      console.log('❌ TEST FAILED:', error.message);
      return {
        success: false,
        error: { message: error.message },
        response: null
      };
    }
  }

  /**
   * Trim long arrays in response for readable console output
   */
  trimLongArrays(obj, maxItems = 5) {
    if (Array.isArray(obj)) {
      if (obj.length > maxItems) {
        return [...obj.slice(0, maxItems), `... ${obj.length - maxItems} more items`];
      }
      return obj;
    } else if (obj && typeof obj === 'object') {
      const trimmed = {};
      for (const [key, value] of Object.entries(obj)) {
        trimmed[key] = this.trimLongArrays(value, maxItems);
      }
      return trimmed;
    }
    return obj;
  }

  /**
   * Ensure model is destroyed between tests for clean state
   */
  async ensureCleanState() {
    try {
      await this.sendRequest('rkllm.destroy', [null]);
      // Wait for cleanup to complete
      await new Promise(resolve => setTimeout(resolve, 1000));
    } catch (error) {
      // Ignore cleanup errors
    }
  }
}

module.exports = TestClient;