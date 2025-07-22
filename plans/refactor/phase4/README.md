# Phase 4: Error Handling & Memory Management
**Duration**: 1-2 weeks  
**Priority**: P1 (Production Readiness)  
**Status**: Not Started

## Overview
Implement comprehensive error handling, memory leak prevention, and robust error recovery mechanisms. This phase ensures the system is production-ready with proper error mapping and resource management.

## Phase 4 Test Implementation

### 1. Add Error Handling Tests to Official Test Suite

#### 1.1 Create Error Handling Test Module
**File**: `tests/integration/test_error_handling.js`

```javascript
// Official test for error handling and recovery
import { RKLLMTestClient } from '../client.js';
import { describe, it, expect, beforeEach, afterEach } from 'mocha';

describe('Error Handling & Recovery', () => {
  let client;
  
  beforeEach(async () => {
    client = new RKLLMTestClient();
    await client.startServer();
    await client.waitForServer();
  });
  
  afterEach(async () => {
    await client.stopServer();
  });
  
  it('should map RKLLM error codes to JSON-RPC errors', async () => {
    const response = await client.httpRequest('test_rkllm_error_mapping', {
      simulate_error: 'RKLLM_INVALID_PARAM',
      expected_json_rpc_code: -32602
    });
    
    expect(response.error).to.be.undefined;
    expect(response.result.error_mapped_correctly).to.be.true;
    expect(response.result.json_rpc_code).to.equal(-32602);
  });
  
  it('should handle invalid JSON requests gracefully', async () => {
    const response = await client.httpRequest('test_invalid_json', {
      invalid_json: '{"jsonrpc":"2.0","method":"invalid"'
    });
    
    expect(response.error).to.not.be.undefined;
    expect(response.error.code).to.equal(-32700); // Parse error
  });
  
  it('should recover from transport connection failures', async () => {
    const response = await client.httpRequest('test_transport_recovery', {
      transport: 'tcp',
      failure_type: 'connection_lost'
    });
    
    expect(response.error).to.be.undefined;
    expect(response.result.recovery_successful).to.be.true;
    expect(response.result.reconnection_time_ms).to.be.lessThan(5000);
  });
  
  it('should detect and prevent memory leaks', async () => {
    const response = await client.httpRequest('test_memory_leak_detection', {
      iterations: 100,
      operation: 'rkllm_run_stress_test'
    });
    
    expect(response.error).to.be.undefined;
    expect(response.result.memory_leaks_detected).to.equal(0);
    expect(response.result.final_memory_usage).to.be.lessThan(response.result.initial_memory_usage * 1.1);
  });
  
  it('should handle concurrent error scenarios', async () => {
    // Simulate multiple clients with various error conditions
    const promises = Array(10).fill().map((_, i) => 
      client.httpRequest('test_concurrent_errors', {
        client_id: i,
        error_scenario: i % 3 // Rotate through different error types
      })
    );
    
    const results = await Promise.all(promises);
    results.forEach((result, i) => {
      expect(result.result.handled_gracefully).to.be.true;
      expect(result.result.client_id).to.equal(i);
    });
  });
});
```

#### 1.2 Create Memory Management Test Module
**File**: `tests/unit/test_memory_management.js`

```javascript
// Official test for memory management
import { RKLLMTestClient } from '../client.js';
import { describe, it, expect, beforeEach, afterEach } from 'mocha';

describe('Memory Management', () => {
  let client;
  
  beforeEach(async () => {
    client = new RKLLMTestClient();
    await client.startServer();
    await client.waitForServer();
  });
  
  afterEach(async () => {
    await client.stopServer();
  });
  
  it('should track application memory allocations', async () => {
    const response = await client.httpRequest('memory_tracker_status', {});
    expect(response.error).to.be.undefined;
    expect(response.result.tracker_initialized).to.be.true;
    expect(response.result.tracked_allocations).to.be.a('number');
  });
  
  it('should cleanup all allocations on shutdown', async () => {
    const response = await client.httpRequest('test_memory_cleanup', {
      allocate_test_buffers: 50,
      force_cleanup: true
    });
    
    expect(response.error).to.be.undefined;
    expect(response.result.allocations_before_cleanup).to.be.greaterThan(0);
    expect(response.result.allocations_after_cleanup).to.equal(0);
  });
  
  it('should handle array parameter memory correctly', async () => {
    const response = await client.httpRequest('test_array_memory_handling', {
      test_arrays: {
        float_array: [1.0, 2.0, 3.0, 4.0, 5.0],
        int_array: [1, 2, 3, 4, 5],
        string_array: ["test1", "test2", "test3"]
      }
    });
    
    expect(response.error).to.be.undefined;
    expect(response.result.arrays_processed).to.equal(3);
    expect(response.result.memory_freed_correctly).to.be.true;
  });
});
```

## Implementation Steps

### Step 1: Implement RKLLM Error Code Mapping
**Files**: `src/lib/core/error_mapping.c`, `src/lib/core/error_mapping.h`

- [ ] **1.1** Create RKLLM to JSON-RPC error mapping
  ```c
  typedef struct {
      int rkllm_code;
      int json_rpc_code;
      const char* message;
  } error_mapping_t;
  
  static const error_mapping_t g_error_mappings[] = {
      { RKLLM_SUCCESS, 0, "Success" },
      { RKLLM_INVALID_PARAM, -32602, "Invalid method parameter(s)" },
      { RKLLM_MODEL_NOT_FOUND, -32603, "Model file not found" },
      { RKLLM_MEMORY_ERROR, -32603, "Memory allocation failed" },
      { RKLLM_INFERENCE_ERROR, -32603, "Inference execution failed" },
      { RKLLM_DEVICE_ERROR, -32603, "NPU device error" },
      { RKLLM_TIMEOUT_ERROR, -32603, "Operation timeout" },
      { RKLLM_CONTEXT_ERROR, -32603, "Context length exceeded" },
      { RKLLM_TOKEN_ERROR, -32602, "Invalid token input" },
      { RKLLM_CALLBACK_ERROR, -32603, "Callback execution failed" },
      { -1, -32603, "Unknown RKLLM error" } // Sentinel
  };
  
  int map_rkllm_error_to_json_rpc(int rkllm_code, int* json_rpc_code, const char** message) {
      for (int i = 0; g_error_mappings[i].rkllm_code != -1; i++) {
          if (g_error_mappings[i].rkllm_code == rkllm_code) {
              *json_rpc_code = g_error_mappings[i].json_rpc_code;
              *message = g_error_mappings[i].message;
              return 0;
          }
      }
      
      // Default mapping for unknown errors
      *json_rpc_code = -32603;
      *message = "Internal error";
      return -1;
  }
  ```

- [ ] **1.2** Implement error response creation
  ```c
  json_object* create_error_response_from_rkllm(int rkllm_code, const char* request_id) {
      int json_rpc_code;
      const char* message;
      
      map_rkllm_error_to_json_rpc(rkllm_code, &json_rpc_code, &message);
      
      json_object* response = json_object_new_object();
      json_object* error = json_object_new_object();
      
      json_object_object_add(response, "jsonrpc", json_object_new_string("2.0"));
      json_object_object_add(response, "id", 
                           request_id ? json_object_new_string(request_id) : NULL);
      
      json_object_object_add(error, "code", json_object_new_int(json_rpc_code));
      json_object_object_add(error, "message", json_object_new_string(message));
      json_object_object_add(error, "data", json_object_new_object());
      
      // Add RKLLM-specific error data
      json_object* data = json_object_object_get(error, "data");
      json_object_object_add(data, "rkllm_code", json_object_new_int(rkllm_code));
      json_object_object_add(data, "source", json_object_new_string("RKLLM"));
      
      json_object_object_add(response, "error", error);
      
      return response;
  }
  ```

### Step 2: Implement Application Memory Tracking
**Files**: `src/lib/core/memory_tracker.c`, `src/lib/core/memory_tracker.h`

- [ ] **2.1** Create memory allocation tracking system
  ```c
  typedef struct memory_allocation {
      void* ptr;
      size_t size;
      const char* function;
      const char* file;
      int line;
      uint64_t timestamp;
      bool is_array;
      struct memory_allocation* next;
  } memory_allocation_t;
  
  typedef struct {
      memory_allocation_t* allocations;
      size_t allocation_count;
      size_t total_allocated;
      pthread_mutex_t tracker_mutex;
      bool initialized;
      FILE* log_file;
  } memory_tracker_t;
  
  static memory_tracker_t g_memory_tracker = {0};
  
  int memory_tracker_init(void) {
      if (g_memory_tracker.initialized) return 0;
      
      if (pthread_mutex_init(&g_memory_tracker.tracker_mutex, NULL) != 0) {
          return -1;
      }
      
      g_memory_tracker.log_file = fopen("memory_tracker.log", "w");
      if (!g_memory_tracker.log_file) {
          pthread_mutex_destroy(&g_memory_tracker.tracker_mutex);
          return -1;
      }
      
      g_memory_tracker.initialized = true;
      return 0;
  }
  
  void memory_tracker_shutdown(void) {
      if (!g_memory_tracker.initialized) return;
      
      pthread_mutex_lock(&g_memory_tracker.tracker_mutex);
      
      // Free all tracked allocations and log leaks
      memory_allocation_t* current = g_memory_tracker.allocations;
      size_t leaks = 0;
      
      while (current) {
          memory_allocation_t* next = current->next;
          
          fprintf(g_memory_tracker.log_file, 
                  "LEAK: %p (%zu bytes) allocated in %s at %s:%d\n",
                  current->ptr, current->size, current->function, 
                  current->file, current->line);
          
          free(current->ptr);
          free(current);
          current = next;
          leaks++;
      }
      
      if (leaks > 0) {
          fprintf(g_memory_tracker.log_file, "TOTAL LEAKS: %zu allocations\n", leaks);
      }
      
      fclose(g_memory_tracker.log_file);
      pthread_mutex_unlock(&g_memory_tracker.tracker_mutex);
      pthread_mutex_destroy(&g_memory_tracker.tracker_mutex);
      
      g_memory_tracker.initialized = false;
  }
  ```

- [ ] **2.2** Implement tracked memory allocation functions
  ```c
  void* memory_tracker_malloc(size_t size, const char* function, const char* file, int line) {
      void* ptr = malloc(size);
      if (!ptr) return NULL;
      
      if (!g_memory_tracker.initialized) {
          return ptr; // Fallback if tracker not initialized
      }
      
      pthread_mutex_lock(&g_memory_tracker.tracker_mutex);
      
      memory_allocation_t* allocation = malloc(sizeof(memory_allocation_t));
      if (allocation) {
          allocation->ptr = ptr;
          allocation->size = size;
          allocation->function = function;
          allocation->file = file;
          allocation->line = line;
          allocation->timestamp = get_timestamp_ms();
          allocation->is_array = false;
          allocation->next = g_memory_tracker.allocations;
          
          g_memory_tracker.allocations = allocation;
          g_memory_tracker.allocation_count++;
          g_memory_tracker.total_allocated += size;
          
          fprintf(g_memory_tracker.log_file, 
                  "ALLOC: %p (%zu bytes) in %s at %s:%d\n",
                  ptr, size, function, file, line);
          fflush(g_memory_tracker.log_file);
      }
      
      pthread_mutex_unlock(&g_memory_tracker.tracker_mutex);
      return ptr;
  }
  
  void memory_tracker_free(void* ptr, const char* function, const char* file, int line) {
      if (!ptr) return;
      
      if (!g_memory_tracker.initialized) {
          free(ptr);
          return;
      }
      
      pthread_mutex_lock(&g_memory_tracker.tracker_mutex);
      
      // Find and remove allocation from tracking list
      memory_allocation_t** current = &g_memory_tracker.allocations;
      bool found = false;
      
      while (*current) {
          if ((*current)->ptr == ptr) {
              memory_allocation_t* to_remove = *current;
              *current = (*current)->next;
              
              g_memory_tracker.allocation_count--;
              g_memory_tracker.total_allocated -= to_remove->size;
              
              fprintf(g_memory_tracker.log_file, 
                      "FREE: %p (%zu bytes) freed in %s at %s:%d\n",
                      ptr, to_remove->size, function, file, line);
              fflush(g_memory_tracker.log_file);
              
              free(to_remove);
              found = true;
              break;
          }
          current = &(*current)->next;
      }
      
      if (!found) {
          fprintf(g_memory_tracker.log_file, 
                  "WARNING: Freeing untracked pointer %p in %s at %s:%d\n",
                  ptr, function, file, line);
      }
      
      pthread_mutex_unlock(&g_memory_tracker.tracker_mutex);
      free(ptr);
  }
  
  // Convenience macros for tracked allocation
  #define TRACKED_MALLOC(size) memory_tracker_malloc(size, __FUNCTION__, __FILE__, __LINE__)
  #define TRACKED_FREE(ptr) memory_tracker_free(ptr, __FUNCTION__, __FILE__, __LINE__)
  ```

### Step 3: Implement Transport Recovery Mechanisms
**Files**: `src/lib/transport/recovery.c`, `src/lib/transport/recovery.h`

- [ ] **3.1** Create transport recovery system
  ```c
  typedef struct {
      transport_type_t type;
      bool connection_lost;
      uint64_t last_failure_time;
      int failure_count;
      int max_retries;
      int retry_interval_ms;
      pthread_t recovery_thread;
      bool recovery_active;
  } transport_recovery_t;
  
  static transport_recovery_t g_transport_recovery[TRANSPORT_TYPE_COUNT] = {0};
  
  int transport_recovery_init(void) {
      for (int i = 0; i < TRANSPORT_TYPE_COUNT; i++) {
          g_transport_recovery[i].type = i;
          g_transport_recovery[i].max_retries = 5;
          g_transport_recovery[i].retry_interval_ms = 1000;
          g_transport_recovery[i].recovery_active = false;
      }
      return 0;
  }
  
  void transport_recovery_handle_failure(transport_type_t type) {
      if (type >= TRANSPORT_TYPE_COUNT) return;
      
      transport_recovery_t* recovery = &g_transport_recovery[type];
      recovery->connection_lost = true;
      recovery->last_failure_time = get_timestamp_ms();
      recovery->failure_count++;
      
      if (!recovery->recovery_active && recovery->failure_count <= recovery->max_retries) {
          recovery->recovery_active = true;
          pthread_create(&recovery->recovery_thread, NULL, transport_recovery_thread, recovery);
          pthread_detach(recovery->recovery_thread);
      }
  }
  
  static void* transport_recovery_thread(void* arg) {
      transport_recovery_t* recovery = (transport_recovery_t*)arg;
      
      while (recovery->connection_lost && recovery->failure_count <= recovery->max_retries) {
          usleep(recovery->retry_interval_ms * 1000);
          
          // Attempt to restart transport
          int result = transport_manager_restart_transport(recovery->type);
          if (result == 0) {
              recovery->connection_lost = false;
              recovery->failure_count = 0;
              printf("Transport %d recovery successful\n", recovery->type);
              break;
          } else {
              recovery->failure_count++;
              printf("Transport %d recovery attempt %d failed\n", recovery->type, recovery->failure_count);
          }
      }
      
      recovery->recovery_active = false;
      return NULL;
  }
  ```

### Step 4: Add Error Handling Test Methods
**Files**: `src/lib/core/operations.c`

- [ ] **4.1** Add error mapping test method
  ```c
  static json_object* handle_test_rkllm_error_mapping(json_object* params, const char* request_id) {
      json_object* simulate_error_obj;
      json_object* expected_code_obj;
      
      if (!json_object_object_get_ex(params, "simulate_error", &simulate_error_obj) ||
          !json_object_object_get_ex(params, "expected_json_rpc_code", &expected_code_obj)) {
          return create_error_response(-32602, "Missing required parameters");
      }
      
      const char* error_name = json_object_get_string(simulate_error_obj);
      int expected_code = json_object_get_int(expected_code_obj);
      
      // Map error name to RKLLM code
      int rkllm_code = 0;
      if (strcmp(error_name, "RKLLM_INVALID_PARAM") == 0) {
          rkllm_code = RKLLM_INVALID_PARAM;
      } else if (strcmp(error_name, "RKLLM_MODEL_NOT_FOUND") == 0) {
          rkllm_code = RKLLM_MODEL_NOT_FOUND;
      } else if (strcmp(error_name, "RKLLM_MEMORY_ERROR") == 0) {
          rkllm_code = RKLLM_MEMORY_ERROR;
      } else {
          return create_error_response(-32602, "Unknown error name");
      }
      
      // Test the mapping
      int json_rpc_code;
      const char* message;
      int result = map_rkllm_error_to_json_rpc(rkllm_code, &json_rpc_code, &message);
      
      json_object* response = json_object_new_object();
      json_object_object_add(response, "error_mapped_correctly", 
                           json_object_new_boolean(json_rpc_code == expected_code));
      json_object_object_add(response, "json_rpc_code", json_object_new_int(json_rpc_code));
      json_object_object_add(response, "message", json_object_new_string(message));
      json_object_object_add(response, "rkllm_code", json_object_new_int(rkllm_code));
      
      return response;
  }
  ```

- [ ] **4.2** Add memory tracking test methods
  ```c
  static json_object* handle_memory_tracker_status(json_object* params, const char* request_id) {
      (void)params; (void)request_id;
      
      json_object* response = json_object_new_object();
      
      if (!g_memory_tracker.initialized) {
          json_object_object_add(response, "tracker_initialized", json_object_new_boolean(false));
          json_object_object_add(response, "error", json_object_new_string("Memory tracker not initialized"));
          return response;
      }
      
      pthread_mutex_lock(&g_memory_tracker.tracker_mutex);
      json_object_object_add(response, "tracker_initialized", json_object_new_boolean(true));
      json_object_object_add(response, "tracked_allocations", json_object_new_int(g_memory_tracker.allocation_count));
      json_object_object_add(response, "total_allocated_bytes", json_object_new_int64(g_memory_tracker.total_allocated));
      pthread_mutex_unlock(&g_memory_tracker.tracker_mutex);
      
      return response;
  }
  
  static json_object* handle_test_memory_leak_detection(json_object* params, const char* request_id) {
      json_object* iterations_obj;
      json_object* operation_obj;
      
      if (!json_object_object_get_ex(params, "iterations", &iterations_obj) ||
          !json_object_object_get_ex(params, "operation", &operation_obj)) {
          return create_error_response(-32602, "Missing required parameters");
      }
      
      int iterations = json_object_get_int(iterations_obj);
      const char* operation = json_object_get_string(operation_obj);
      
      // Record initial memory usage
      size_t initial_allocations = g_memory_tracker.allocation_count;
      size_t initial_memory = g_memory_tracker.total_allocated;
      
      // Perform stress test
      for (int i = 0; i < iterations; i++) {
          if (strcmp(operation, "rkllm_run_stress_test") == 0) {
              // Allocate and free test buffers
              char* test_buffer = TRACKED_MALLOC(1024);
              if (test_buffer) {
                  memset(test_buffer, 0, 1024);
                  TRACKED_FREE(test_buffer);
              }
          }
      }
      
      // Check final memory usage
      size_t final_allocations = g_memory_tracker.allocation_count;
      size_t final_memory = g_memory_tracker.total_allocated;
      
      int memory_leaks = (final_allocations > initial_allocations) ? 
                        (final_allocations - initial_allocations) : 0;
      
      json_object* response = json_object_new_object();
      json_object_object_add(response, "iterations_completed", json_object_new_int(iterations));
      json_object_object_add(response, "initial_memory_usage", json_object_new_int64(initial_memory));
      json_object_object_add(response, "final_memory_usage", json_object_new_int64(final_memory));
      json_object_object_add(response, "memory_leaks_detected", json_object_new_int(memory_leaks));
      json_object_object_add(response, "test_passed", json_object_new_boolean(memory_leaks == 0));
      
      return response;
  }
  ```

## Testing Validation

### Phase 4 Success Criteria

Run `npm test` - all these must pass:

- [ ] **Test 1**: RKLLM error codes map correctly to JSON-RPC errors
- [ ] **Test 2**: Invalid JSON requests return proper parse errors
- [ ] **Test 3**: Transport connection failures trigger recovery mechanisms
- [ ] **Test 4**: Memory leak detection prevents application buffer leaks
- [ ] **Test 5**: Concurrent error scenarios are handled gracefully
- [ ] **Test 6**: Memory tracking works correctly for array parameters

## Files Modified in Phase 4

### New Files
- [ ] `src/lib/core/error_mapping.h`
- [ ] `src/lib/core/error_mapping.c`
- [ ] `src/lib/core/memory_tracker.h`
- [ ] `src/lib/core/memory_tracker.c`
- [ ] `src/lib/transport/recovery.h`
- [ ] `src/lib/transport/recovery.c`
- [ ] `tests/integration/test_error_handling.js`
- [ ] `tests/unit/test_memory_management.js`

### Modified Files
- [ ] `src/lib/core/operations.c` (Add error and memory test methods)
- [ ] `src/lib/core/rkllm_proxy.c` (Use tracked memory allocation)
- [ ] All transport files (Integrate recovery mechanisms)

## Success Metrics

After Phase 4 completion:
- [ ] Zero memory leaks detected in stress testing
- [ ] All RKLLM errors properly mapped to JSON-RPC
- [ ] Transport failures automatically trigger recovery
- [ ] Error handling covers all edge cases
- [ ] Production-ready error reporting and logging

**Next Phase**: Performance Optimization & Production Features