const { spawn } = require('child_process');
const path = require('path');
const fs = require('fs');

/**
 * Server Manager - Handles RKLLM server lifecycle for testing
 * Prevents hanging by managing server startup/shutdown properly
 */
class ServerManager {
  constructor() {
    this.serverProcess = null;
    this.serverReady = false;
  }

  /**
   * Start RKLLM server for testing
   * @returns {Promise<boolean>} Success status
   */
  async startServer() {
    return new Promise((resolve, reject) => {
      console.log('🚀 Starting RKLLM server...');
      
      // Build server executable path
      const serverPath = path.join(__dirname, '../../build/server');
      
      // Check if server executable exists
      if (!fs.existsSync(serverPath)) {
        console.error('❌ Server executable not found at:', serverPath);
        console.error('   Please run: cmake --build build');
        return reject(new Error('Server executable not found'));
      }

      // Start server process
      this.serverProcess = spawn(serverPath, [], {
        stdio: ['pipe', 'pipe', 'pipe'],
        env: {
          ...process.env,
          LD_LIBRARY_PATH: path.join(__dirname, '../../src/external/rkllm')
        }
      });

      // Handle server output
      this.serverProcess.stdout.on('data', (data) => {
        const output = data.toString();
        if (output.includes('Server started successfully')) {
          console.log('✅ RKLLM server started successfully');
          this.serverReady = true;
          resolve(true);
        }
      });

      this.serverProcess.stderr.on('data', (data) => {
        console.log('🔍 Server stderr:', data.toString().trim());
      });

      this.serverProcess.on('error', (error) => {
        console.error('❌ Failed to start server:', error);
        reject(error);
      });

      this.serverProcess.on('exit', (code) => {
        console.log(`🔄 Server exited with code: ${code}`);
        this.serverReady = false;
      });

      // Timeout after 10 seconds
      setTimeout(() => {
        if (!this.serverReady) {
          console.error('❌ Server startup timeout');
          this.stopServer();
          reject(new Error('Server startup timeout'));
        }
      }, 10000);
    });
  }

  /**
   * Stop RKLLM server
   */
  stopServer() {
    if (this.serverProcess) {
      console.log('🛑 Stopping RKLLM server...');
      this.serverProcess.kill('SIGTERM');
      this.serverProcess = null;
      this.serverReady = false;
    }
  }

  /**
   * Wait for server to be ready
   */
  async waitForReady(timeout = 5000) {
    const start = Date.now();
    while (!this.serverReady && (Date.now() - start) < timeout) {
      await new Promise(resolve => setTimeout(resolve, 100));
    }
    return this.serverReady;
  }
}

module.exports = ServerManager;