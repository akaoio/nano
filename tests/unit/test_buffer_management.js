// Official test for HTTP buffer management
import { RKLLMTestClient } from '../client.js';
import { describe, it, expect, beforeEach, afterEach } from 'mocha';

describe('HTTP Buffer Management', () => {
  let client;
  
  beforeEach(async () => {
    client = new RKLLMTestClient();
    await client.startServer();
    await client.waitForServer();
  });
  
  afterEach(async () => {
    await client.stopServer();
  });
  
  it('should initialize HTTP buffer manager', async () => {
    const response = await client.httpRequest('http_buffer_manager_status', {});
    expect(response.error).to.be.undefined;
    expect(response.result.initialized).to.be.true;
    expect(response.result.active_buffers).to.be.a('number');
  });
  
  it('should create and retrieve buffer chunks', async () => {
    const response = await client.httpRequest('test_buffer_operations', {
      operation: 'create_and_retrieve',
      request_id: 'test_456',
      chunks: ['chunk1', 'chunk2', 'chunk3']
    });
    
    expect(response.error).to.be.undefined;
    expect(response.result.created).to.be.true;
    expect(response.result.retrieved_chunks).to.have.lengthOf(3);
  });
});