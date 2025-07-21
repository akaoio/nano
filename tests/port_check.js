#!/usr/bin/env node

/**
 * Port availability checker for MCP server
 */

const net = require('net');
const { spawn } = require('child_process');
const path = require('path');

// Check if a port is available/in use
function checkPort(port, host = '127.0.0.1') {
    return new Promise((resolve) => {
        const server = net.createServer();
        
        server.listen(port, host, () => {
            server.close(() => {
                resolve({ port, available: true, error: null });
            });
        });
        
        server.on('error', (err) => {
            resolve({ port, available: false, error: err.code });
        });
    });
}

// Check if something is listening on a port
function checkListening(port, host = '127.0.0.1') {
    return new Promise((resolve) => {
        const socket = new net.Socket();
        const timeout = setTimeout(() => {
            socket.destroy();
            resolve({ port, listening: false, error: 'TIMEOUT' });
        }, 2000);
        
        socket.connect(port, host, () => {
            clearTimeout(timeout);
            socket.destroy();
            resolve({ port, listening: true, error: null });
        });
        
        socket.on('error', (err) => {
            clearTimeout(timeout);
            resolve({ port, listening: false, error: err.code });
        });
    });
}

async function main() {
    console.log('🔍 MCP Server Port Diagnostic');
    console.log('==============================');
    
    const ports = [8080, 8081, 8082, 8083];
    const transportNames = ['TCP', 'UDP', 'HTTP', 'WebSocket'];
    
    // Check port availability before starting server
    console.log('\n📊 Checking port availability before server start:');
    for (let i = 0; i < ports.length; i++) {
        const result = await checkPort(ports[i]);
        const status = result.available ? '✅ Available' : `❌ In use (${result.error})`;
        console.log(`   ${transportNames[i]} (${ports[i]}): ${status}`);
    }
    
    // Start the server
    console.log('\n🚀 Starting MCP Server...');
    const serverPath = path.join(__dirname, '..', 'build', 'mcp_server');
    const serverProcess = spawn(serverPath, [], {
        stdio: ['ignore', 'pipe', 'pipe']
    });
    
    let serverOutput = '';
    
    serverProcess.stdout.on('data', (data) => {
        serverOutput += data.toString();
        console.log(`[SERVER] ${data.toString().trim()}`);
    });
    
    serverProcess.stderr.on('data', (data) => {
        serverOutput += data.toString();
        console.log(`[SERVER] ${data.toString().trim()}`);
    });
    
    // Wait for server to start
    await new Promise(resolve => setTimeout(resolve, 5000));
    
    // Check if ports are actually listening
    console.log('\n📊 Checking if ports are listening after server start:');
    for (let i = 0; i < ports.length; i++) {
        const result = await checkListening(ports[i]);
        const status = result.listening ? '✅ Listening' : `❌ Not listening (${result.error})`;
        console.log(`   ${transportNames[i]} (${ports[i]}): ${status}`);
    }
    
    // Check with different host options
    console.log('\n📊 Checking localhost vs 0.0.0.0 binding:');
    for (let i = 0; i < ports.length; i++) {
        const localhostResult = await checkListening(ports[i], '127.0.0.1');
        const anyResult = await checkListening(ports[i], '0.0.0.0');
        
        console.log(`   ${transportNames[i]} (${ports[i]}):`);
        console.log(`     127.0.0.1: ${localhostResult.listening ? '✅' : '❌'}`);
        console.log(`     0.0.0.0: ${anyResult.listening ? '✅' : '❌'}`);
    }
    
    // Show any interesting server output
    console.log('\n📋 Server output analysis:');
    const lines = serverOutput.split('\n');
    const relevantLines = lines.filter(line => 
        line.includes('transport') || 
        line.includes('port') || 
        line.includes('bind') || 
        line.includes('listen') ||
        line.includes('error') ||
        line.includes('failed')
    );
    
    if (relevantLines.length > 0) {
        relevantLines.forEach(line => console.log(`   ${line.trim()}`));
    } else {
        console.log('   No transport-specific messages found');
    }
    
    // Stop server
    console.log('\n🛑 Stopping server...');
    serverProcess.kill('SIGTERM');
    
    setTimeout(() => {
        if (!serverProcess.killed) {
            serverProcess.kill('SIGKILL');
        }
        process.exit(0);
    }, 3000);
}

main().catch(console.error);