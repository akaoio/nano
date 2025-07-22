# Phase 1: Core Streaming Infrastructure
**Duration**: 2-3 weeks  
**Priority**: P0 (Blocking)  
**Status**: Not Started

## Overview
Implement the missing streaming infrastructure that connects RKLLM callbacks to transport layers. This phase focuses on the critical missing piece that prevents real-time streaming from working.

## Phase 1 Test Implementation

### 1. Add Streaming Tests to Official Test Suite

#### 1.1 Create Core Streaming Test Module
**File**: `tests/unit/test_streaming_core.js`

```javascript
// Official test for streaming infrastructure
import { RKLLMTestClient } from '../client.js';
import { describe, it, expect, beforeEach, afterEach } from 'mocha';

describe('Core Streaming Infrastructure', () => {
  let client;
  
  beforeEach(async () => {
    client = new RKLLMTestClient();
    await client.startServer();
    await client.waitForServer();
  });
  
  afterEach(async () => {
    await client.stopServer();
  });
  
  it('should initialize stream manager successfully', async () => {
    const response = await client.httpRequest('stream_manager_status', {});
    expect(response.error).to.be.undefined;
    expect(response.result.initialized).to.be.true;
    expect(response.result.active_sessions).to.equal(0);
  });
  
  it('should create streaming session for rkllm_run_async', async () => {
    const response = await client.httpRequest('rkllm_run_async', {
      input_type: 0,
      prompt_input: 'Test streaming',
      stream: true
    });
    
    expect(response.error).to.be.undefined;
    expect(response.result.session_id).to.be.a('string');
    expect(response.result.status).to.equal('streaming_started');
  });
  
  it('should handle streaming callbacks properly', async () => {
    // This test ensures RKLLM callbacks integrate with MCP
    const response = await client.httpRequest('test_streaming_callback', {
      mock_tokens: ['Hello', ' world', '!'],
      delay_ms: 100
    });
    
    expect(response.error).to.be.undefined;
    expect(response.result.callback_registered).to.be.true;
  });
});
```

#### 1.2 Update package.json Test Scripts

```json
{
  "scripts": {
    "test": "npm run test:unit && npm run test:integration",
    "test:unit": "mocha tests/unit/**/*.js --require tests/setup.js",
    "test:integration": "mocha tests/integration/**/*.js --require tests/setup.js",
    "test:streaming": "mocha tests/unit/test_streaming_core.js --require tests/setup.js"
  }
}
```

## Implementation Steps

### Step 1: Implement Stream Manager Core
**Files**: `src/lib/core/stream_manager.c`, `src/lib/core/stream_manager.h`

- [ ] **1.1** Create `stream_session_t` structure with proper fields
  ```c
  typedef struct {
      char session_id[64];
      char request_id[32];
      transport_type_t transport_type;
      void* transport_handle;
      rkllm_stream_context_t* rkllm_context;
      pthread_mutex_t session_mutex;
      bool active;
      uint32_t sequence_number;
      uint64_t created_timestamp;
      uint64_t last_activity;
  } stream_session_t;
  ```

- [ ] **1.2** Implement `stream_manager_t` with thread-safe operations
  ```c
  typedef struct {
      stream_session_t* sessions;
      size_t session_count;
      size_t max_sessions;
      pthread_mutex_t manager_mutex;
      pthread_t cleanup_thread;
      bool initialized;
      bool running;
  } stream_manager_t;
  ```

- [ ] **1.3** Core functions implementation:
  - [ ] `stream_manager_init(stream_manager_t* manager)`
  - [ ] `stream_manager_shutdown(stream_manager_t* manager)`
  - [ ] `stream_manager_create_session(const char* request_id, transport_type_t type)`
  - [ ] `stream_manager_get_session(const char* session_id)`
  - [ ] `stream_manager_destroy_session(const char* session_id)`
  - [ ] `stream_manager_cleanup_expired_sessions(void)`

- [ ] **1.4** Add session timeout and cleanup mechanism
  - [ ] Automatic session cleanup after 30 seconds of inactivity
  - [ ] Background thread for periodic cleanup
  - [ ] Proper resource deallocation

### Step 2: Fix RKLLM Callback Integration
**Files**: `src/lib/core/rkllm_proxy.c`

- [ ] **2.1** Replace placeholder streaming callback with real implementation
  ```c
  static void rkllm_streaming_callback_real(RKLLMResult* result, void* userdata, LLMCallState state) {
      stream_session_t* session = (stream_session_t*)userdata;
      
      if (!session || !session->active) {
          return;
      }
      
      // Create MCP streaming chunk
      mcp_stream_chunk_t chunk = {0};
      strncpy(chunk.request_id, session->request_id, sizeof(chunk.request_id) - 1);
      strncpy(chunk.method, "rkllm_stream", sizeof(chunk.method) - 1);
      chunk.seq = session->sequence_number++;
      chunk.end = (state == RKLLM_RUN_FINISH || state == RKLLM_RUN_ERROR);
      
      // Copy result data safely
      if (result && result->text) {
          size_t copy_len = strnlen(result->text, sizeof(chunk.delta) - 1);
          memcpy(chunk.delta, result->text, copy_len);
          chunk.delta[copy_len] = '\0';
      }
      
      // Handle errors
      if (state == RKLLM_RUN_ERROR) {
          chunk.error_message = "RKLLM inference error";
      }
      
      // Forward to transport layer
      transport_send_stream_chunk(session->transport_handle, &chunk);
      
      // Update session activity
      session->last_activity = get_timestamp_ms();
      
      // Clean up session if finished
      if (chunk.end) {
          session->active = false;
      }
  }
  ```

- [ ] **2.2** Fix `rkllm_run_async` implementation to use real streaming
  ```c
  static json_object* handle_rkllm_run_async(json_object* params, const char* request_id) {
      // Create streaming session
      char session_id[64];
      generate_session_id(session_id, sizeof(session_id));
      
      stream_session_t* session = stream_manager_create_session(request_id, get_current_transport_type());
      if (!session) {
          return create_error_response(-32603, "Failed to create streaming session");
      }
      
      // Set up RKLLM parameters with real callback
      RKLLMParam rkllm_params = {0};
      // ... populate from JSON params ...
      
      // Set the real streaming callback
      rkllm_params.callback = rkllm_streaming_callback_real;
      rkllm_params.userdata = session;
      
      // Start RKLLM async inference
      int result = rkllm_run_async(g_rkllm_handle, &rkllm_params);
      if (result != 0) {
          stream_manager_destroy_session(session->session_id);
          return create_error_response(-32603, "RKLLM run_async failed");
      }
      
      // Return success response with session info
      json_object* response = json_object_new_object();
      json_object_object_add(response, "session_id", json_object_new_string(session->session_id));
      json_object_object_add(response, "status", json_object_new_string("streaming_started"));
      
      return response;
  }
  ```

### Step 3: Integrate Stream Manager with MCP Adapter
**Files**: `src/lib/protocol/adapter.c`

- [ ] **3.1** Replace TODO comments with real stream manager initialization
  ```c
  int mcp_adapter_init(mcp_adapter_t* adapter) {
      if (!adapter) return MCP_ADAPTER_ERROR_INVALID_JSON;
      
      memset(adapter, 0, sizeof(mcp_adapter_t));
      strncpy(adapter->protocol_version, "2025-03-26", sizeof(adapter->protocol_version) - 1);
      adapter->utf8_validation_enabled = true;
      adapter->message_batching_enabled = true;
      
      // Initialize stream manager (REPLACE TODO)
      if (stream_manager_init(&adapter->stream_manager) != 0) {
          return MCP_ADAPTER_ERROR_STREAM_ERROR;
      }
      
      // Initialize HTTP buffer manager for polling
      if (http_buffer_manager_init(&adapter->http_buffers) != 0) {
          stream_manager_shutdown(&adapter->stream_manager);
          return MCP_ADAPTER_ERROR_STREAM_ERROR;
      }
      
      // Initialize IO operations (RKLLM proxy)
      if (io_operations_init() != 0) {
          stream_manager_shutdown(&adapter->stream_manager);
          http_buffer_manager_shutdown(&adapter->http_buffers);
          return MCP_ADAPTER_ERROR_INVALID_JSON;
      }
      
      adapter->initialized = true;
      return MCP_ADAPTER_OK;
  }
  ```

- [ ] **3.2** Fix shutdown function to clean up stream manager
  ```c
  void mcp_adapter_shutdown(mcp_adapter_t* adapter) {
      if (!adapter || !adapter->initialized) return;
      
      // Shutdown stream manager (REPLACE TODO)
      stream_manager_shutdown(&adapter->stream_manager);
      
      // Shutdown HTTP buffer manager
      http_buffer_manager_shutdown(&adapter->http_buffers);
      
      // Shutdown IO operations
      io_operations_shutdown();
      
      adapter->initialized = false;
  }
  ```

### Step 4: Add Stream Status Endpoint for Testing
**Files**: `src/lib/core/operations.c`

- [ ] **4.1** Add `stream_manager_status` method for test validation
  ```c
  static json_object* handle_stream_manager_status(json_object* params, const char* request_id) {
      (void)params; (void)request_id; // Suppress unused warnings
      
      json_object* response = json_object_new_object();
      
      // Get stream manager instance
      stream_manager_t* manager = get_global_stream_manager();
      if (!manager) {
          json_object_object_add(response, "initialized", json_object_new_boolean(false));
          json_object_object_add(response, "error", json_object_new_string("Stream manager not initialized"));
          return response;
      }
      
      // Return status information
      json_object_object_add(response, "initialized", json_object_new_boolean(manager->initialized));
      json_object_object_add(response, "active_sessions", json_object_new_int(manager->session_count));
      json_object_object_add(response, "max_sessions", json_object_new_int(manager->max_sessions));
      
      return response;
  }
  ```

- [ ] **4.2** Register the method in operations table
  ```c
  // Add to g_operations array
  { "stream_manager_status", handle_stream_manager_status },
  ```

### Step 5: Create Test Streaming Callback Method
**Files**: `src/lib/core/operations.c`

- [ ] **5.1** Add test method for callback validation
  ```c
  static json_object* handle_test_streaming_callback(json_object* params, const char* request_id) {
      // This method tests the streaming callback mechanism with mock data
      
      // Extract test parameters
      json_object* mock_tokens_obj;
      json_object* delay_ms_obj;
      
      if (!json_object_object_get_ex(params, "mock_tokens", &mock_tokens_obj) ||
          !json_object_object_get_ex(params, "delay_ms", &delay_ms_obj)) {
          return create_error_response(-32602, "Missing required parameters: mock_tokens, delay_ms");
      }
      
      // Create a test streaming session
      stream_session_t* session = stream_manager_create_session(request_id, TRANSPORT_TYPE_HTTP);
      if (!session) {
          return create_error_response(-32603, "Failed to create test streaming session");
      }
      
      // Simulate RKLLM callback with mock tokens
      int token_count = json_object_array_length(mock_tokens_obj);
      int delay_ms = json_object_get_int(delay_ms_obj);
      
      for (int i = 0; i < token_count; i++) {
          json_object* token_obj = json_object_array_get_idx(mock_tokens_obj, i);
          const char* token = json_object_get_string(token_obj);
          
          // Create mock RKLLM result
          RKLLMResult mock_result = {0};
          mock_result.text = (char*)token;
          
          // Determine state
          LLMCallState state = (i == token_count - 1) ? RKLLM_RUN_FINISH : RKLLM_RUN_NORMAL;
          
          // Call the streaming callback
          rkllm_streaming_callback_real(&mock_result, session, state);
          
          // Add delay between tokens
          if (delay_ms > 0) {
              usleep(delay_ms * 1000);
          }
      }
      
      json_object* response = json_object_new_object();
      json_object_object_add(response, "callback_registered", json_object_new_boolean(true));
      json_object_object_add(response, "tokens_processed", json_object_new_int(token_count));
      json_object_object_add(response, "session_id", json_object_new_string(session->session_id));
      
      return response;
  }
  ```

## Testing Validation

### Phase 1 Success Criteria

Run `npm test` - all these must pass:

- [ ] **Test 1**: Stream manager initializes successfully
- [ ] **Test 2**: Streaming sessions can be created for `rkllm_run_async`
- [ ] **Test 3**: Streaming callbacks are properly registered and functional
- [ ] **Test 4**: Session cleanup works correctly
- [ ] **Test 5**: Multiple concurrent streaming sessions are supported

### Integration Points

- [ ] **Integration 1**: Stream manager integrates with all transport types
- [ ] **Integration 2**: RKLLM callbacks properly forward to MCP layer  
- [ ] **Integration 3**: Memory management is leak-free
- [ ] **Integration 4**: Error handling covers all failure scenarios

## Files Modified in Phase 1

### New Files
- [ ] `src/lib/core/stream_manager.h`
- [ ] `src/lib/core/stream_manager.c`
- [ ] `tests/unit/test_streaming_core.js`

### Modified Files  
- [ ] `src/lib/protocol/adapter.c` (Replace 2 TODOs)
- [ ] `src/lib/core/operations.c` (Replace 3 TODOs)
- [ ] `src/lib/core/rkllm_proxy.c` (Replace 4 TODOs)
- [ ] `package.json` (Update test scripts)

## Risk Mitigation

### High Risk Items
- **Memory leaks** in session management → Comprehensive cleanup testing
- **Thread safety** issues → Use proper mutex locking
- **RKLLM callback crashes** → Defensive programming with null checks

### Testing Strategy
- Unit tests for each stream manager function
- Integration tests for RKLLM callback flow
- Memory leak testing with valgrind
- Concurrent session stress testing

## Success Metrics

After Phase 1 completion:
- [ ] `npm test` passes 100%
- [ ] No memory leaks detected
- [ ] Stream sessions create/destroy properly
- [ ] RKLLM callbacks flow to transport layer
- [ ] All TODOs in core streaming components removed

**Next Phase**: Transport Layer Connection Handling