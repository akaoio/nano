# Phase 3: HTTP Transport Polling & Buffer Management
**Duration**: 1-2 weeks  
**Priority**: P0 (Critical for HTTP streaming)  
**Status**: Not Started

## Overview
Fix the HTTP transport polling mechanism and implement the "NOT IMPLEMENTED" buffer management system. This phase makes HTTP streaming work through proper polling and chunk buffering as specified in IDEA.md.

## Phase 3 Test Implementation

### 1. Add HTTP Polling Tests to Official Test Suite

#### 1.1 Create HTTP Polling Test Module
**File**: `tests/integration/test_http_polling.js`

```javascript
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
```

#### 1.2 Create Buffer Management Test Module
**File**: `tests/unit/test_buffer_management.js`

```javascript
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
```

## Implementation Steps

### Step 1: Implement HTTP Buffer Manager
**Files**: `src/lib/protocol/http_buffer_manager.c`, `src/lib/protocol/http_buffer_manager.h`

- [ ] **1.1** Create HTTP buffer structures
  ```c
  typedef struct {
      char request_id[64];
      char* chunks;
      size_t chunks_size;
      size_t chunks_capacity;
      uint32_t chunk_count;
      uint64_t created_timestamp;
      uint64_t last_access;
      bool completed;
  } http_buffer_t;
  
  typedef struct {
      http_buffer_t* buffers;
      size_t buffer_count;
      size_t max_buffers;
      pthread_mutex_t manager_mutex;
      pthread_t cleanup_thread;
      bool initialized;
      bool running;
      uint32_t cleanup_interval_seconds;
      uint32_t buffer_timeout_seconds;
  } http_buffer_manager_t;
  ```

- [ ] **1.2** Implement core buffer operations
  ```c
  int http_buffer_manager_init(http_buffer_manager_t* manager) {
      if (!manager) return -1;
      
      memset(manager, 0, sizeof(http_buffer_manager_t));
      
      // Allocate buffer array
      manager->max_buffers = HTTP_MAX_BUFFERS;
      manager->buffers = calloc(manager->max_buffers, sizeof(http_buffer_t));
      if (!manager->buffers) {
          return -1;
      }
      
      // Initialize mutex
      if (pthread_mutex_init(&manager->manager_mutex, NULL) != 0) {
          free(manager->buffers);
          return -1;
      }
      
      // Start cleanup thread
      manager->cleanup_interval_seconds = 30;
      manager->buffer_timeout_seconds = 60;
      manager->running = true;
      
      if (pthread_create(&manager->cleanup_thread, NULL, http_buffer_cleanup_thread, manager) != 0) {
          pthread_mutex_destroy(&manager->manager_mutex);
          free(manager->buffers);
          return -1;
      }
      
      manager->initialized = true;
      return 0;
  }
  
  void http_buffer_manager_shutdown(http_buffer_manager_t* manager) {
      if (!manager || !manager->initialized) return;
      
      manager->running = false;
      
      // Wait for cleanup thread to finish
      pthread_join(manager->cleanup_thread, NULL);
      
      // Free all buffers
      pthread_mutex_lock(&manager->manager_mutex);
      for (size_t i = 0; i < manager->buffer_count; i++) {
          if (manager->buffers[i].chunks) {
              free(manager->buffers[i].chunks);
          }
      }
      free(manager->buffers);
      pthread_mutex_unlock(&manager->manager_mutex);
      
      pthread_mutex_destroy(&manager->manager_mutex);
      manager->initialized = false;
  }
  ```

- [ ] **1.3** Implement buffer creation and chunk accumulation
  ```c
  http_buffer_t* http_buffer_manager_create_buffer(http_buffer_manager_t* manager, const char* request_id) {
      if (!manager || !request_id) return NULL;
      
      pthread_mutex_lock(&manager->manager_mutex);
      
      // Check if buffer already exists
      for (size_t i = 0; i < manager->buffer_count; i++) {
          if (strcmp(manager->buffers[i].request_id, request_id) == 0) {
              pthread_mutex_unlock(&manager->manager_mutex);
              return &manager->buffers[i]; // Return existing buffer
          }
      }
      
      // Create new buffer if space available
      if (manager->buffer_count >= manager->max_buffers) {
          pthread_mutex_unlock(&manager->manager_mutex);
          return NULL; // No space available
      }
      
      http_buffer_t* buffer = &manager->buffers[manager->buffer_count];
      memset(buffer, 0, sizeof(http_buffer_t));
      
      strncpy(buffer->request_id, request_id, sizeof(buffer->request_id) - 1);
      buffer->chunks_capacity = HTTP_INITIAL_CHUNK_CAPACITY;
      buffer->chunks = malloc(buffer->chunks_capacity);
      buffer->created_timestamp = get_timestamp_ms();
      buffer->last_access = buffer->created_timestamp;
      
      if (!buffer->chunks) {
          pthread_mutex_unlock(&manager->manager_mutex);
          return NULL;
      }
      
      buffer->chunks[0] = '\0';
      manager->buffer_count++;
      
      pthread_mutex_unlock(&manager->manager_mutex);
      return buffer;
  }
  
  int http_buffer_manager_add_chunk(http_buffer_manager_t* manager, const char* request_id, const mcp_stream_chunk_t* chunk) {
      if (!manager || !request_id || !chunk) return -1;
      
      pthread_mutex_lock(&manager->manager_mutex);
      
      // Find buffer
      http_buffer_t* buffer = NULL;
      for (size_t i = 0; i < manager->buffer_count; i++) {
          if (strcmp(manager->buffers[i].request_id, request_id) == 0) {
              buffer = &manager->buffers[i];
              break;
          }
      }
      
      if (!buffer) {
          pthread_mutex_unlock(&manager->manager_mutex);
          return -1; // Buffer not found
      }
      
      // Format chunk as JSON
      char chunk_json[1024];
      snprintf(chunk_json, sizeof(chunk_json),
               "{\"seq\":%u,\"delta\":\"%s\",\"end\":%s}",
               chunk->seq, chunk->delta, chunk->end ? "true" : "false");
      
      size_t chunk_len = strlen(chunk_json);
      size_t current_len = strlen(buffer->chunks);
      
      // Expand buffer if needed
      if (current_len + chunk_len + 2 > buffer->chunks_capacity) {
          size_t new_capacity = buffer->chunks_capacity * 2;
          char* new_chunks = realloc(buffer->chunks, new_capacity);
          if (!new_chunks) {
              pthread_mutex_unlock(&manager->manager_mutex);
              return -1;
          }
          buffer->chunks = new_chunks;
          buffer->chunks_capacity = new_capacity;
      }
      
      // Add chunk to buffer (concatenate with separator)
      if (current_len > 0) {
          strcat(buffer->chunks, ",");
      }
      strcat(buffer->chunks, chunk_json);
      
      buffer->chunk_count++;
      buffer->last_access = get_timestamp_ms();
      
      if (chunk->end) {
          buffer->completed = true;
      }
      
      pthread_mutex_unlock(&manager->manager_mutex);
      return 0;
  }
  ```

- [ ] **1.4** Implement buffer retrieval and cleanup
  ```c
  int http_buffer_manager_get_chunks(http_buffer_manager_t* manager, const char* request_id, char* output, size_t output_size, bool clear_after_read) {
      if (!manager || !request_id || !output) return -1;
      
      pthread_mutex_lock(&manager->manager_mutex);
      
      // Find buffer
      http_buffer_t* buffer = NULL;
      size_t buffer_index = 0;
      for (size_t i = 0; i < manager->buffer_count; i++) {
          if (strcmp(manager->buffers[i].request_id, request_id) == 0) {
              buffer = &manager->buffers[i];
              buffer_index = i;
              break;
          }
      }
      
      if (!buffer) {
          pthread_mutex_unlock(&manager->manager_mutex);
          return -1; // Buffer not found
      }
      
      // Copy chunks to output
      size_t chunks_len = strlen(buffer->chunks);
      if (chunks_len >= output_size) {
          pthread_mutex_unlock(&manager->manager_mutex);
          return -1; // Output buffer too small
      }
      
      strcpy(output, buffer->chunks);
      buffer->last_access = get_timestamp_ms();
      
      // Clear buffer if requested or if completed
      if (clear_after_read || buffer->completed) {
          // Remove buffer from array
          free(buffer->chunks);
          
          // Move last buffer to this position
          if (buffer_index < manager->buffer_count - 1) {
              memcpy(buffer, &manager->buffers[manager->buffer_count - 1], sizeof(http_buffer_t));
          }
          manager->buffer_count--;
      }
      
      pthread_mutex_unlock(&manager->manager_mutex);
      return 0;
  }
  
  static void* http_buffer_cleanup_thread(void* arg) {
      http_buffer_manager_t* manager = (http_buffer_manager_t*)arg;
      
      while (manager->running) {
          sleep(manager->cleanup_interval_seconds);
          
          uint64_t current_time = get_timestamp_ms();
          uint64_t timeout_ms = manager->buffer_timeout_seconds * 1000;
          
          pthread_mutex_lock(&manager->manager_mutex);
          
          // Clean up expired buffers
          for (size_t i = 0; i < manager->buffer_count; ) {
              if (current_time - manager->buffers[i].last_access > timeout_ms) {
                  // Free expired buffer
                  free(manager->buffers[i].chunks);
                  
                  // Move last buffer to this position
                  if (i < manager->buffer_count - 1) {
                      memcpy(&manager->buffers[i], &manager->buffers[manager->buffer_count - 1], sizeof(http_buffer_t));
                  }
                  manager->buffer_count--;
              } else {
                  i++;
              }
          }
          
          pthread_mutex_unlock(&manager->manager_mutex);
      }
      
      return NULL;
  }
  ```

### Step 2: Fix MCP Protocol Poll Method
**Files**: `src/lib/protocol/mcp_protocol.c`

- [ ] **2.1** Replace "NOT IMPLEMENTED" poll method with real implementation
  ```c
  int mcp_handle_stream_poll_request(const char* request_id, char* response, size_t response_size) {
      // Get global HTTP buffer manager
      http_buffer_manager_t* manager = get_global_http_buffer_manager();
      if (!manager) {
          return mcp_format_error_response(-32603, "HTTP buffer manager not initialized", response, response_size);
      }
      
      // Get buffered chunks for this request_id
      char chunks_buffer[HTTP_MAX_CHUNK_SIZE];
      int result = http_buffer_manager_get_chunks(manager, request_id, chunks_buffer, sizeof(chunks_buffer), true);
      
      if (result != 0) {
          return mcp_format_error_response(-32602, "No streaming session found or buffer error", response, response_size);
      }
      
      // Format response with chunks
      if (strlen(chunks_buffer) == 0) {
          // No chunks available yet
          snprintf(response, response_size,
                   "{\"jsonrpc\":\"2.0\",\"id\":\"%s\",\"result\":{\"chunk\":null}}", 
                   request_id);
      } else {
          // Return accumulated chunks
          snprintf(response, response_size,
                   "{\"jsonrpc\":\"2.0\",\"id\":\"%s\",\"result\":{\"chunk\":{\"chunks\":[%s]}}}",
                   request_id, chunks_buffer);
      }
      
      return 0;
  }
  ```

### Step 3: Integrate HTTP Buffer Manager with MCP Adapter
**Files**: `src/lib/protocol/adapter.c`

- [ ] **3.1** Add HTTP buffer manager to MCP adapter
  ```c
  // Add to mcp_adapter_t structure
  typedef struct {
      char protocol_version[32];
      bool utf8_validation_enabled;
      bool message_batching_enabled;
      bool initialized;
      stream_manager_t stream_manager;
      http_buffer_manager_t http_buffers;  // ADD THIS LINE
  } mcp_adapter_t;
  ```

- [ ] **3.2** Initialize HTTP buffer manager in adapter initialization
  ```c
  int mcp_adapter_init(mcp_adapter_t* adapter) {
      // ...existing code...
      
      // Initialize HTTP buffer manager (REPLACE EXISTING TODO)
      if (http_buffer_manager_init(&adapter->http_buffers) != 0) {
          stream_manager_shutdown(&adapter->stream_manager);
          return MCP_ADAPTER_ERROR_STREAM_ERROR;
      }
      
      // ...existing code...
  }
  
  void mcp_adapter_shutdown(mcp_adapter_t* adapter) {
      // ...existing code...
      
      // Shutdown HTTP buffer manager (REPLACE EXISTING TODO)
      http_buffer_manager_shutdown(&adapter->http_buffers);
      
      // ...existing code...
  }
  ```

### Step 4: Add HTTP Buffer Manager Test Methods
**Files**: `src/lib/core/operations.c`

- [ ] **4.1** Add HTTP buffer manager status method
  ```c
  static json_object* handle_http_buffer_manager_status(json_object* params, const char* request_id) {
      (void)params; (void)request_id;
      
      json_object* response = json_object_new_object();
      
      http_buffer_manager_t* manager = get_global_http_buffer_manager();
      if (!manager) {
          json_object_object_add(response, "initialized", json_object_new_boolean(false));
          json_object_object_add(response, "error", json_object_new_string("HTTP buffer manager not initialized"));
          return response;
      }
      
      json_object_object_add(response, "initialized", json_object_new_boolean(manager->initialized));
      json_object_object_add(response, "active_buffers", json_object_new_int(manager->buffer_count));
      json_object_object_add(response, "max_buffers", json_object_new_int(manager->max_buffers));
      json_object_object_add(response, "cleanup_interval", json_object_new_int(manager->cleanup_interval_seconds));
      
      return response;
  }
  ```

- [ ] **4.2** Add HTTP buffer testing methods
  ```c
  static json_object* handle_test_http_buffer_manager(json_object* params, const char* request_id) {
      json_object* request_id_obj;
      json_object* mock_chunks_obj;
      
      if (!json_object_object_get_ex(params, "request_id", &request_id_obj) ||
          !json_object_object_get_ex(params, "mock_chunks", &mock_chunks_obj)) {
          return create_error_response(-32602, "Missing required parameters: request_id, mock_chunks");
      }
      
      const char* test_request_id = json_object_get_string(request_id_obj);
      int chunk_count = json_object_array_length(mock_chunks_obj);
      
      http_buffer_manager_t* manager = get_global_http_buffer_manager();
      if (!manager) {
          return create_error_response(-32603, "HTTP buffer manager not initialized");
      }
      
      // Create buffer for test request
      http_buffer_t* buffer = http_buffer_manager_create_buffer(manager, test_request_id);
      if (!buffer) {
          return create_error_response(-32603, "Failed to create HTTP buffer");
      }
      
      // Add mock chunks to buffer
      for (int i = 0; i < chunk_count; i++) {
          json_object* chunk_obj = json_object_array_get_idx(mock_chunks_obj, i);
          json_object* seq_obj, * delta_obj, * end_obj;
          
          if (json_object_object_get_ex(chunk_obj, "seq", &seq_obj) &&
              json_object_object_get_ex(chunk_obj, "delta", &delta_obj) &&
              json_object_object_get_ex(chunk_obj, "end", &end_obj)) {
              
              mcp_stream_chunk_t chunk = {0};
              chunk.seq = json_object_get_int(seq_obj);
              strncpy(chunk.delta, json_object_get_string(delta_obj), sizeof(chunk.delta) - 1);
              chunk.end = json_object_get_boolean(end_obj);
              
              http_buffer_manager_add_chunk(manager, test_request_id, &chunk);
          }
      }
      
      json_object* response = json_object_new_object();
      json_object_object_add(response, "chunks_buffered", json_object_new_int(chunk_count));
      json_object_object_add(response, "buffer_size", json_object_new_int(strlen(buffer->chunks)));
      json_object_object_add(response, "request_id", json_object_new_string(test_request_id));
      
      return response;
  }
  
  static json_object* handle_test_buffer_cleanup(json_object* params, const char* request_id) {
      json_object* create_expired_obj;
      json_object* timeout_obj;
      
      if (!json_object_object_get_ex(params, "create_expired_buffers", &create_expired_obj) ||
          !json_object_object_get_ex(params, "timeout_seconds", &timeout_obj)) {
          return create_error_response(-32602, "Missing required parameters: create_expired_buffers, timeout_seconds");
      }
      
      int buffers_to_create = json_object_get_int(create_expired_obj);
      int timeout_seconds = json_object_get_int(timeout_obj);
      
      http_buffer_manager_t* manager = get_global_http_buffer_manager();
      if (!manager) {
          return create_error_response(-32603, "HTTP buffer manager not initialized");
      }
      
      // Create expired buffers
      for (int i = 0; i < buffers_to_create; i++) {
          char test_id[64];
          snprintf(test_id, sizeof(test_id), "expired_test_%d", i);
          
          http_buffer_t* buffer = http_buffer_manager_create_buffer(manager, test_id);
          if (buffer) {
              // Manually set old timestamp to simulate expiration
              buffer->last_access = get_timestamp_ms() - (timeout_seconds + 1) * 1000;
          }
      }
      
      int initial_count = manager->buffer_count;
      
      // Wait for cleanup
      sleep(timeout_seconds + 1);
      
      int final_count = manager->buffer_count;
      int cleaned = initial_count - final_count;
      
      json_object* response = json_object_new_object();
      json_object_object_add(response, "buffers_created", json_object_new_int(buffers_to_create));
      json_object_object_add(response, "buffers_cleaned", json_object_new_int(cleaned));
      json_object_object_add(response, "initial_count", json_object_new_int(initial_count));
      json_object_object_add(response, "final_count", json_object_new_int(final_count));
      
      return response;
  }
  ```

## Testing Validation

### Phase 3 Success Criteria

Run `npm test` - all these must pass:

- [ ] **Test 1**: Poll method works for HTTP streaming (no more "NOT IMPLEMENTED")
- [ ] **Test 2**: HTTP buffer manager initializes and manages buffers
- [ ] **Test 3**: Chunks are properly buffered and retrieved via polling
- [ ] **Test 4**: Expired buffers are automatically cleaned up
- [ ] **Test 5**: Concurrent HTTP polling sessions work correctly
- [ ] **Test 6**: HTTP polling integrates with real RKLLM streaming

## Files Modified in Phase 3

### New Files
- [ ] `src/lib/protocol/http_buffer_manager.h`
- [ ] `src/lib/protocol/http_buffer_manager.c`
- [ ] `tests/integration/test_http_polling.js`
- [ ] `tests/unit/test_buffer_management.js`

### Modified Files
- [ ] `src/lib/protocol/mcp_protocol.c` (Fix "NOT IMPLEMENTED" poll method)
- [ ] `src/lib/protocol/adapter.c` (Integrate HTTP buffer manager)
- [ ] `src/lib/core/operations.c` (Add HTTP buffer test methods)

## Success Metrics

After Phase 3 completion:
- [ ] HTTP streaming works via polling mechanism
- [ ] No "NOT IMPLEMENTED" errors in protocol layer
- [ ] Buffer management prevents memory leaks
- [ ] Concurrent HTTP sessions supported
- [ ] All streaming tests pass for HTTP transport

**Next Phase**: Error Handling & Memory Management