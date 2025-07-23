const net = require('net');

class StreamingTestClient {
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
   * Send request and collect ALL streaming responses until completion
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

    console.log('📤 REQUEST:', JSON.stringify(request, null, 2));

    return new Promise((resolve, reject) => {
      let responseData = '';
      let timeout = null;
      let dataHandler = null;
      let allResponses = [];
      let finalResult = null;

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

      // Data handler - collect all streaming responses
      dataHandler = (data) => {
        responseData += data.toString();
        
        // Process complete JSON lines
        const lines = responseData.split('\n');
        
        for (let i = 0; i < lines.length - 1; i++) {
          const line = lines[i].trim();
          if (!line) continue;
          
          try {
            const response = JSON.parse(line);
            
            // Check if this is response to our request
            if (response.id === request.id) {
              console.log('📥 STREAMING RESPONSE:', JSON.stringify(response, null, 2));
              allResponses.push(response);
              
              // Check if this is the final response
              if (response.result && response.result._callback_state === 2) { // RKLLM_RUN_FINISH
                console.log('🏁 Final streaming response received');
                
                // Combine all streaming responses into final result
                finalResult = {
                  success: true,
                  allResponses: allResponses,
                  combinedText: allResponses.map(r => r.result?.text || '').join(''),
                  tokenCount: allResponses.length,
                  finalResponse: response
                };
                
                cleanup();
                resolve(finalResult);
                return;
              }
              
              // For non-streaming responses (no callback state), return immediately
              if (response.result && response.result._callback_state === undefined) {
                finalResult = {
                  success: true,
                  response: response,
                  result: response.result
                };
                cleanup();
                resolve(finalResult);
                return;
              }
              
              // For error responses
              if (response.error) {
                finalResult = {
                  success: false,
                  error: response.error,
                  response: response
                };
                cleanup();
                resolve(finalResult);
                return;
              }
            }
          } catch (e) {
            // Invalid JSON line, continue
            continue;
          }
        }
        
        // Keep the last potentially incomplete line
        responseData = lines[lines.length - 1] || '';
      };

      // Set up handlers
      this.socket.on('data', dataHandler);

      // Set timeout (longer for streaming responses)
      timeout = setTimeout(() => {
        cleanup();
        
        if (allResponses.length > 0) {
          // We got some responses but no final state - return what we have
          console.log('⏰ Timeout but got partial streaming responses');
          resolve({
            success: true,
            allResponses: allResponses,
            combinedText: allResponses.map(r => r.result?.text || '').join(''),
            tokenCount: allResponses.length,
            incomplete: true
          });
        } else {
          reject(new Error(`Request timeout for method: ${method}`));
        }
      }, 30000); // 30 second timeout for streaming

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
   * Test a function with proper streaming support
   */
  async testStreamingFunction(method, params) {
    console.log(`\n🧪 STREAMING TEST: ${method}`);
    console.log('=' .repeat(60));

    try {
      const result = await this.sendStreamingRequest(method, params);
      
      // Wait between requests
      await new Promise(resolve => setTimeout(resolve, 1000));
      
      if (result.success) {
        if (result.allResponses) {
          console.log('✅ STREAMING SUCCESS:');
          console.log(`   Combined text: "${result.combinedText}"`);
          console.log(`   Token count: ${result.tokenCount}`);
          console.log(`   Complete: ${!result.incomplete}`);
          
          return {
            success: true,
            streamingResult: result,
            text: result.combinedText,
            tokenCount: result.tokenCount
          };
        } else {
          console.log('✅ NON-STREAMING SUCCESS:', result.result);
          return {
            success: true,
            result: result.result,
            response: result.response
          };
        }
      } else {
        console.log('⚠️  ERROR:', result.error.message);
        return {
          success: false,
          error: result.error,
          response: result.response
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
   * Test basic non-streaming function
   */
  async testBasicFunction(method, params) {
    return this.testStreamingFunction(method, params);
  }
}

module.exports = StreamingTestClient;