# Phase 2: Transport Layer Implementation
**Duration**: 2-3 weeks  
**Priority**: P0 (Blocking)  
**Status**: Not Started

## Overview
Complete the transport layer implementations (TCP, UDP, WebSocket) which currently have skeleton code but no actual networking functionality. This phase makes all 5 transports fully functional for real client connections.

## Phase 2 Test Implementation

### 1. Add Transport Tests to Official Test Suite

#### 1.1 Create Transport Connection Test Module
**File**: `tests/integration/test_transport_connections.js`

```javascript
// Official test for transport layer functionality
import { RKLLMTestClient } from '../client.js';
import { describe, it, expect, beforeEach, afterEach } from 'mocha';
import net from 'net';
import dgram from 'dgram';
import WebSocket from 'ws';

describe('Transport Layer Connections', () => {
  let client;
  
  beforeEach(async () => {
    client = new RKLLMTestClient();
    await client.startServer();
    await client.waitForServer();
  });
  
  afterEach(async () => {
    await client.stopServer();
  });
  
  it('should handle TCP connections and requests', async () => {
    const response = await client.tcpRequest('rkllm_list_functions', {});
    expect(response.error).to.be.undefined;
    expect(response.result).to.be.an('array');
    expect(response.result.length).to.be.greaterThan(0);
  });
  
  it('should handle UDP requests', async () => {
    const response = await client.udpRequest('rkllm_createDefaultParam', {});
    expect(response.error).to.be.undefined;
    expect(response.result).to.be.an('object');
  });
  
  it('should handle WebSocket connections', async () => {
    const response = await client.wsRequest('rkllm_get_constants', {});
    expect(response.error).to.be.undefined;
    expect(response.result).to.be.an('object');
  });
  
  it('should handle multiple concurrent connections per transport', async () => {
    // Test 5 concurrent TCP connections
    const tcpPromises = Array(5).fill().map((_, i) => 
      client.tcpRequest('rkllm_list_functions', { test_id: i })
    );
    
    const results = await Promise.all(tcpPromises);
    results.forEach((result, i) => {
      expect(result.error).to.be.undefined;
      expect(result.result).to.be.an('array');
    });
  });
  
  it('should handle connection drops gracefully', async () => {
    // This test verifies transport error handling
    const response = await client.httpRequest('test_connection_drop', {
      transport: 'tcp',
      action: 'simulate_drop'
    });
    
    expect(response.error).to.be.undefined;
    expect(response.result.handled_gracefully).to.be.true;
  });
});
```

#### 1.2 Create Transport Streaming Test Module  
**File**: `tests/integration/test_transport_streaming.js`

```javascript
// Official test for transport-specific streaming
import { RKLLMTestClient } from '../client.js';
import { describe, it, expect, beforeEach, afterEach } from 'mocha';

describe('Transport Streaming Implementation', () => {
  let client;
  
  beforeEach(async () => {
    client = new RKLLMTestClient();
    await client.startServer();
    await client.waitForServer();
  });
  
  afterEach(async () => {
    await client.stopServer();
  });
  
  it('should stream via TCP in real-time', async () => {
    await client.testRealTimeStreaming('tcp', {
      input_type: 0,
      prompt_input: 'Count to 3',
      stream: true
    });
  });
  
  it('should stream via WebSocket in real-time', async () => {
    await client.testRealTimeStreaming('ws', {
      input_type: 0,
      prompt_input: 'Count to 3',
      stream: true
    });
  });
  
  it('should handle HTTP polling streaming', async () => {
    const result = await client.testHttpPollingStreaming({
      input_type: 0,
      prompt_input: 'Count to 3',
      stream: true
    });
    
    expect(result.status).to.equal('completed');
    expect(result.polls).to.be.greaterThan(0);
  });
});
```

## Implementation Steps

### Step 1: Complete TCP Transport Implementation
**Files**: `src/lib/transport/tcp.c`, `src/lib/transport/tcp.h`

- [ ] **1.1** Implement TCP server initialization
  ```c
  typedef struct {
      int server_socket;
      int client_sockets[MAX_TCP_CLIENTS];
      pthread_t server_thread;
      pthread_t client_threads[MAX_TCP_CLIENTS];
      bool running;
      char host[64];
      int port;
      size_t client_count;
      pthread_mutex_t clients_mutex;
  } tcp_transport_state_t;
  
  static tcp_transport_state_t g_tcp_state = {0};
  ```

- [ ] **1.2** Implement TCP server thread
  ```c
  static void* tcp_server_thread(void* arg) {
      tcp_transport_t* transport = (tcp_transport_t*)arg;
      
      // Create server socket
      g_tcp_state.server_socket = socket(AF_INET, SOCK_STREAM, 0);
      if (g_tcp_state.server_socket < 0) {
          printf("TCP: Failed to create server socket\n");
          return NULL;
      }
      
      // Set socket options
      int opt = 1;
      setsockopt(g_tcp_state.server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
      
      // Bind and listen
      struct sockaddr_in server_addr = {0};
      server_addr.sin_family = AF_INET;
      server_addr.sin_addr.s_addr = INADDR_ANY;
      server_addr.sin_port = htons(g_tcp_state.port);
      
      if (bind(g_tcp_state.server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
          printf("TCP: Failed to bind to port %d\n", g_tcp_state.port);
          return NULL;
      }
      
      if (listen(g_tcp_state.server_socket, MAX_TCP_CLIENTS) < 0) {
          printf("TCP: Failed to listen\n");
          return NULL;
      }
      
      printf("TCP: Server listening on %s:%d\n", g_tcp_state.host, g_tcp_state.port);
      
      // Accept connections
      while (g_tcp_state.running) {
          struct sockaddr_in client_addr;
          socklen_t client_len = sizeof(client_addr);
          
          int client_socket = accept(g_tcp_state.server_socket, (struct sockaddr*)&client_addr, &client_len);
          if (client_socket < 0) {
              if (g_tcp_state.running) {
                  printf("TCP: Failed to accept connection\n");
              }
              continue;
          }
          
          // Find available client slot
          pthread_mutex_lock(&g_tcp_state.clients_mutex);
          int client_index = -1;
          for (int i = 0; i < MAX_TCP_CLIENTS; i++) {
              if (g_tcp_state.client_sockets[i] == 0) {
                  client_index = i;
                  g_tcp_state.client_sockets[i] = client_socket;
                  g_tcp_state.client_count++;
                  break;
              }
          }
          pthread_mutex_unlock(&g_tcp_state.clients_mutex);
          
          if (client_index >= 0) {
              // Create client handler thread
              tcp_client_data_t* client_data = malloc(sizeof(tcp_client_data_t));
              client_data->socket = client_socket;
              client_data->index = client_index;
              client_data->transport = transport;
              
              pthread_create(&g_tcp_state.client_threads[client_index], NULL, tcp_client_handler, client_data);
              pthread_detach(g_tcp_state.client_threads[client_index]);
          } else {
              printf("TCP: Maximum clients reached, dropping connection\n");
              close(client_socket);
          }
      }
      
      return NULL;
  }
  ```

- [ ] **1.3** Implement TCP client handler
  ```c
  typedef struct {
      int socket;
      int index;
      tcp_transport_t* transport;
  } tcp_client_data_t;
  
  static void* tcp_client_handler(void* arg) {
      tcp_client_data_t* client_data = (tcp_client_data_t*)arg;
      int client_socket = client_data->socket;
      int client_index = client_data->index;
      
      char buffer[TCP_BUFFER_SIZE];
      char response_buffer[TCP_RESPONSE_SIZE];
      
      while (g_tcp_state.running) {
          // Read request from client
          ssize_t bytes_received = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
          if (bytes_received <= 0) {
              break; // Client disconnected or error
          }
          
          buffer[bytes_received] = '\0';
          
          // Process request through MCP adapter
          size_t response_size = sizeof(response_buffer);
          int result = mcp_adapter_process_request(buffer, response_buffer, &response_size);
          
          if (result == MCP_ADAPTER_OK) {
              // Send response back to client
              ssize_t bytes_sent = send(client_socket, response_buffer, response_size, 0);
              if (bytes_sent < 0) {
                  printf("TCP: Failed to send response to client %d\n", client_index);
                  break;
              }
          } else {
              // Send error response
              const char* error_response = "{\"jsonrpc\":\"2.0\",\"error\":{\"code\":-32603,\"message\":\"Internal error\"},\"id\":null}\n";
              send(client_socket, error_response, strlen(error_response), 0);
          }
      }
      
      // Cleanup client connection
      close(client_socket);
      
      pthread_mutex_lock(&g_tcp_state.clients_mutex);
      g_tcp_state.client_sockets[client_index] = 0;
      g_tcp_state.client_count--;
      pthread_mutex_unlock(&g_tcp_state.clients_mutex);
      
      free(client_data);
      return NULL;
  }
  ```

- [ ] **1.4** Implement TCP streaming support
  ```c
  static int tcp_send_stream_chunk(transport_base_t* base, const mcp_stream_chunk_t* chunk) {
      tcp_transport_t* transport = (tcp_transport_t*)base;
      
      // Format chunk as JSON-RPC response
      char chunk_json[4096];
      int result = mcp_adapter_format_stream_chunk(chunk, chunk_json, sizeof(chunk_json));
      if (result != MCP_ADAPTER_OK) {
          return -1;
      }
      
      // Send to all connected clients (broadcast for now)
      // In production, should send only to the client that initiated the stream
      pthread_mutex_lock(&g_tcp_state.clients_mutex);
      for (int i = 0; i < MAX_TCP_CLIENTS; i++) {
          if (g_tcp_state.client_sockets[i] > 0) {
              ssize_t bytes_sent = send(g_tcp_state.client_sockets[i], chunk_json, strlen(chunk_json), 0);
              if (bytes_sent < 0) {
                  printf("TCP: Failed to send stream chunk to client %d\n", i);
              }
          }
      }
      pthread_mutex_unlock(&g_tcp_state.clients_mutex);
      
      return 0;
  }
  ```

### Step 2: Complete UDP Transport Implementation
**Files**: `src/lib/transport/udp.c`, `src/lib/transport/udp.h`

- [ ] **2.1** Implement UDP server initialization
  ```c
  typedef struct {
      int server_socket;
      pthread_t server_thread;
      bool running;
      char host[64];
      int port;
      struct sockaddr_in clients[MAX_UDP_CLIENTS];
      size_t client_count;
      pthread_mutex_t clients_mutex;
  } udp_transport_state_t;
  
  static udp_transport_state_t g_udp_state = {0};
  ```

- [ ] **2.2** Implement UDP server thread
  ```c
  static void* udp_server_thread(void* arg) {
      udp_transport_t* transport = (udp_transport_t*)arg;
      
      // Create UDP socket
      g_udp_state.server_socket = socket(AF_INET, SOCK_DGRAM, 0);
      if (g_udp_state.server_socket < 0) {
          printf("UDP: Failed to create server socket\n");
          return NULL;
      }
      
      // Bind socket
      struct sockaddr_in server_addr = {0};
      server_addr.sin_family = AF_INET;
      server_addr.sin_addr.s_addr = INADDR_ANY;
      server_addr.sin_port = htons(g_udp_state.port);
      
      if (bind(g_udp_state.server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
          printf("UDP: Failed to bind to port %d\n", g_udp_state.port);
          return NULL;
      }
      
      printf("UDP: Server listening on %s:%d\n", g_udp_state.host, g_udp_state.port);
      
      char buffer[UDP_BUFFER_SIZE];
      char response_buffer[UDP_RESPONSE_SIZE];
      
      while (g_udp_state.running) {
          struct sockaddr_in client_addr;
          socklen_t client_len = sizeof(client_addr);
          
          // Receive packet from client
          ssize_t bytes_received = recvfrom(g_udp_state.server_socket, buffer, sizeof(buffer) - 1, 0,
                                          (struct sockaddr*)&client_addr, &client_len);
          if (bytes_received <= 0) {
              if (g_udp_state.running) {
                  printf("UDP: Failed to receive packet\n");
              }
              continue;
          }
          
          buffer[bytes_received] = '\0';
          
          // Process request through MCP adapter
          size_t response_size = sizeof(response_buffer);
          int result = mcp_adapter_process_request(buffer, response_buffer, &response_size);
          
          if (result == MCP_ADAPTER_OK) {
              // Send response back to client
              ssize_t bytes_sent = sendto(g_udp_state.server_socket, response_buffer, response_size, 0,
                                        (struct sockaddr*)&client_addr, client_len);
              if (bytes_sent < 0) {
                  printf("UDP: Failed to send response\n");
              }
          } else {
              // Send error response
              const char* error_response = "{\"jsonrpc\":\"2.0\",\"error\":{\"code\":-32603,\"message\":\"Internal error\"},\"id\":null}\n";
              sendto(g_udp_state.server_socket, error_response, strlen(error_response), 0,
                     (struct sockaddr*)&client_addr, client_len);
          }
          
          // Register client for streaming (if not already registered)
          udp_register_client(&client_addr, client_len);
      }
      
      return NULL;
  }
  ```

- [ ] **2.3** Implement UDP client registration for streaming
  ```c
  static void udp_register_client(struct sockaddr_in* client_addr, socklen_t client_len) {
      pthread_mutex_lock(&g_udp_state.clients_mutex);
      
      // Check if client already registered
      for (size_t i = 0; i < g_udp_state.client_count; i++) {
          if (memcmp(&g_udp_state.clients[i], client_addr, sizeof(struct sockaddr_in)) == 0) {
              pthread_mutex_unlock(&g_udp_state.clients_mutex);
              return; // Already registered
          }
      }
      
      // Add new client if space available
      if (g_udp_state.client_count < MAX_UDP_CLIENTS) {
          memcpy(&g_udp_state.clients[g_udp_state.client_count], client_addr, sizeof(struct sockaddr_in));
          g_udp_state.client_count++;
      }
      
      pthread_mutex_unlock(&g_udp_state.clients_mutex);
  }
  ```

### Step 3: Complete WebSocket Transport Implementation
**Files**: `src/lib/transport/websocket.c`, `src/lib/transport/websocket.h`

- [ ] **3.1** Implement WebSocket server initialization using wsServer library
  ```c
  typedef struct {
      ws_server_t* server;
      ws_connection_t* connections[MAX_WS_CONNECTIONS];
      size_t connection_count;
      pthread_mutex_t connections_mutex;
      bool running;
      char host[64];
      int port;
  } websocket_transport_state_t;
  
  static websocket_transport_state_t g_ws_state = {0};
  ```

- [ ] **3.2** Implement WebSocket message handlers
  ```c
  static void on_ws_connect(ws_connection_t* connection) {
      printf("WebSocket: Client connected\n");
      
      pthread_mutex_lock(&g_ws_state.connections_mutex);
      
      // Find available connection slot
      for (size_t i = 0; i < MAX_WS_CONNECTIONS; i++) {
          if (g_ws_state.connections[i] == NULL) {
              g_ws_state.connections[i] = connection;
              g_ws_state.connection_count++;
              break;
          }
      }
      
      pthread_mutex_unlock(&g_ws_state.connections_mutex);
  }
  
  static void on_ws_disconnect(ws_connection_t* connection) {
      printf("WebSocket: Client disconnected\n");
      
      pthread_mutex_lock(&g_ws_state.connections_mutex);
      
      // Remove connection from active list
      for (size_t i = 0; i < MAX_WS_CONNECTIONS; i++) {
          if (g_ws_state.connections[i] == connection) {
              g_ws_state.connections[i] = NULL;
              g_ws_state.connection_count--;
              break;
          }
      }
      
      pthread_mutex_unlock(&g_ws_state.connections_mutex);
  }
  
  static void on_ws_message(ws_connection_t* connection, const char* message, size_t length) {
      char response_buffer[WS_RESPONSE_SIZE];
      size_t response_size = sizeof(response_buffer);
      
      // Process request through MCP adapter
      int result = mcp_adapter_process_request(message, response_buffer, &response_size);
      
      if (result == MCP_ADAPTER_OK) {
          // Send response back to client
          ws_send_text(connection, response_buffer, response_size);
      } else {
          // Send error response
          const char* error_response = "{\"jsonrpc\":\"2.0\",\"error\":{\"code\":-32603,\"message\":\"Internal error\"},\"id\":null}";
          ws_send_text(connection, error_response, strlen(error_response));
      }
  }
  ```

- [ ] **3.3** Implement WebSocket streaming support
  ```c
  static int websocket_send_stream_chunk(transport_base_t* base, const mcp_stream_chunk_t* chunk) {
      websocket_transport_t* transport = (websocket_transport_t*)base;
      
      // Format chunk as JSON-RPC response
      char chunk_json[4096];
      int result = mcp_adapter_format_stream_chunk(chunk, chunk_json, sizeof(chunk_json));
      if (result != MCP_ADAPTER_OK) {
          return -1;
      }
      
      // Send to all connected WebSocket clients
      pthread_mutex_lock(&g_ws_state.connections_mutex);
      for (size_t i = 0; i < MAX_WS_CONNECTIONS; i++) {
          if (g_ws_state.connections[i] != NULL) {
              ws_send_text(g_ws_state.connections[i], chunk_json, strlen(chunk_json));
          }
      }
      pthread_mutex_unlock(&g_ws_state.connections_mutex);
      
      return 0;
  }
  ```

### Step 4: Implement Connection Drop Testing
**Files**: `src/lib/core/operations.c`

- [ ] **4.1** Add test method for connection drop simulation
  ```c
  static json_object* handle_test_connection_drop(json_object* params, const char* request_id) {
      json_object* transport_obj;
      json_object* action_obj;
      
      if (!json_object_object_get_ex(params, "transport", &transport_obj) ||
          !json_object_object_get_ex(params, "action", &action_obj)) {
          return create_error_response(-32602, "Missing required parameters: transport, action");
      }
      
      const char* transport = json_object_get_string(transport_obj);
      const char* action = json_object_get_string(action_obj);
      
      if (strcmp(action, "simulate_drop") != 0) {
          return create_error_response(-32602, "Invalid action, expected 'simulate_drop'");
      }
      
      bool handled_gracefully = false;
      
      // Simulate connection drop for specified transport
      if (strcmp(transport, "tcp") == 0) {
          handled_gracefully = tcp_test_connection_drop();
      } else if (strcmp(transport, "udp") == 0) {
          handled_gracefully = udp_test_connection_drop();
      } else if (strcmp(transport, "websocket") == 0) {
          handled_gracefully = websocket_test_connection_drop();
      } else {
          return create_error_response(-32602, "Invalid transport type");
      }
      
      json_object* response = json_object_new_object();
      json_object_object_add(response, "handled_gracefully", json_object_new_boolean(handled_gracefully));
      json_object_object_add(response, "transport", json_object_new_string(transport));
      json_object_object_add(response, "action", json_object_new_string(action));
      
      return response;
  }
  ```

### Step 5: Fix Transport Base Interface
**Files**: `src/lib/transport/manager.c`

- [ ] **5.1** Implement proper transport streaming integration
  ```c
  int transport_manager_send_stream_chunk(transport_manager_t* manager, const mcp_stream_chunk_t* chunk) {
      if (!manager || !chunk || !manager->transport) {
          return TRANSPORT_MANAGER_ERROR_INVALID_TRANSPORT;
      }
      
      // Call transport-specific streaming method
      if (manager->transport->send_stream_chunk) {
          return manager->transport->send_stream_chunk(manager->transport, chunk);
      } else {
          printf("Transport %s does not support streaming\n", manager->transport->name);
          return TRANSPORT_MANAGER_ERROR_NOT_SUPPORTED;
      }
  }
  ```

- [ ] **5.2** Add connection status monitoring
  ```c
  int transport_manager_get_connection_count(transport_manager_t* manager) {
      if (!manager || !manager->transport) {
          return -1;
      }
      
      // Call transport-specific connection count method
      if (manager->transport->get_connection_count) {
          return manager->transport->get_connection_count(manager->transport);
      }
      
      return 0; // Default for transports that don't track connections
  }
  ```

## Testing Validation

### Phase 2 Success Criteria

Run `npm test` - all these must pass:

- [ ] **Test 1**: TCP connections can be established and handle requests
- [ ] **Test 2**: UDP packets are processed correctly  
- [ ] **Test 3**: WebSocket connections work with handshake and messaging
- [ ] **Test 4**: Multiple concurrent connections work per transport
- [ ] **Test 5**: Connection drops are handled gracefully
- [ ] **Test 6**: Real-time streaming works for TCP and WebSocket
- [ ] **Test 7**: HTTP polling streaming continues to work

## Files Modified in Phase 2

### New Files
- [ ] `tests/integration/test_transport_connections.js`
- [ ] `tests/integration/test_transport_streaming.js`

### Modified Files
- [ ] `src/lib/transport/tcp.c` (Complete implementation)
- [ ] `src/lib/transport/udp.c` (Complete implementation)  
- [ ] `src/lib/transport/websocket.c` (Complete implementation)
- [ ] `src/lib/transport/manager.c` (Add streaming integration)
- [ ] `src/lib/core/operations.c` (Add connection drop testing)

## Success Metrics

After Phase 2 completion:
- [ ] All 5 transports accept real network connections
- [ ] Concurrent connections supported (TCP: 50+, UDP: 100+, WS: 20+)
- [ ] Real-time streaming works for TCP, UDP, WebSocket
- [ ] Connection error handling works properly
- [ ] No network resource leaks

**Next Phase**: HTTP Transport Polling & Buffer Management