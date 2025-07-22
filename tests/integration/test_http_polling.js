// Official test for HTTP polling streaming mechanism
import { RKLLMTestClient } from '../client.js';
import { describe, it, expect, beforeEach, afterEach } from 'mocha';

describe('HTTP Polling Stream Management', () => {
  let client;
  
  beforeEach(async () => {
    client = new RKLLMTestClient();
    await client.startServer();
    await client.waitForServer();
  });
  
  afterEach(async () => {
    await client.stopServer();
  });
  
  it('should implement poll method for HTTP streaming', async () => {
    // Start async request first
    const startResponse = await client.httpRequest('rkllm_run_async', {
      input_type: 0,
      prompt_input: 'Test HTTP polling',
      stream: true
    });
    
    expect(startResponse.error).to.be.undefined;
    const requestId = startResponse.id;
    
    // Poll for chunks
    const pollResponse = await client.httpRequest('poll', { request_id: requestId });
    expect(pollResponse.error).to.be.undefined;
    expect(pollResponse.result).to.have.property('chunk');
  });
  
  it('should buffer chunks properly for HTTP polling', async () => {
    const response = await client.httpRequest('test_http_buffer_manager', {
      request_id: 'test_123',
      mock_chunks: [
        { seq: 0, delta: 'Hello', end: false },
        { seq: 1, delta: ' world', end: false },
        { seq: 2, delta: '!', end: true }
      ]
    });
    
    expect(response.error).to.be.undefined;
    expect(response.result.chunks_buffered).to.equal(3);
    expect(response.result.buffer_size).to.be.greaterThan(0);
  });
  
  it('should clean up expired buffers automatically', async () => {
    const response = await client.httpRequest('test_buffer_cleanup', {
      create_expired_buffers: 3,
      timeout_seconds: 1
    });
    
    expect(response.error).to.be.undefined;
    expect(response.result.buffers_created).to.equal(3);
    expect(response.result.buffers_cleaned).to.equal(3);
  });
  
  it('should handle concurrent HTTP polling sessions', async () => {
    // Start 3 concurrent streaming sessions
    const sessions = [];
    for (let i = 0; i < 3; i++) {
      const response = await client.httpRequest('rkllm_run_async', {
        input_type: 0,
        prompt_input: `Test session ${i}`,
        stream: true
      });
      sessions.push(response.id);
    }
    
    // Poll each session
    for (const sessionId of sessions) {
      const pollResponse = await client.httpRequest('poll', { request_id: sessionId });
      expect(pollResponse.error).to.be.undefined;
    }
  });
});