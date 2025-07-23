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
            console.log('📥 RESPONSE:', JSON.stringify(response, null, 2));
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
              console.log('📥 RESPONSE:', JSON.stringify(response, null, 2));
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

      // Set timeout
      timeout = setTimeout(() => {
        cleanup();
        reject(new Error(`Request timeout for method: ${method}`));
      }, 15000); // Reduced timeout to 15 seconds

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