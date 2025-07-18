#include "nano.h"
#include "../../../common/core.h"
#include "../../../io/core/io/io.h"
#include "../../../io/mapping/rkllm_proxy/rkllm_proxy.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>

nano_core_t g_nano = {
    .initialized = false,
    .running = false,
    .transport_count = 0
};

static void* transport_worker(void* arg);

int nano_init(void) {
    if (g_nano.initialized) return -1;
    
    // Initialize IO subsystem
    if (io_init() != 0) {
        return -1;
    }
    
    // Initialize RKLLM proxy
    if (rkllm_proxy_init() != 0) {
        return -1;
    }
    
    g_nano.initialized = true;
    g_nano.running = false;
    g_nano.transport_count = 0;
    
    return 0;
}

int nano_add_transport(mcp_transport_t* transport, void* config) {
    if (!g_nano.initialized || !transport || g_nano.transport_count >= 8) {
        return -1;
    }
    
    // Initialize transport
    if (transport->init(config) != 0) {
        return -1;
    }
    
    g_nano.transports[g_nano.transport_count++] = transport;
    return 0;
}

int nano_run(void) {
    if (!g_nano.initialized || g_nano.running) return -1;
    
    g_nano.running = true;
    
    // Create worker threads for each transport
    pthread_t workers[8];
    for (int i = 0; i < g_nano.transport_count; i++) {
        if (pthread_create(&workers[i], nullptr, transport_worker, g_nano.transports[i]) != 0) {
            g_nano.running = false;
            return -1;
        }
    }
    
    // Wait for all workers to complete
    for (int i = 0; i < g_nano.transport_count; i++) {
        pthread_join(workers[i], nullptr);
    }
    
    return 0;
}

void nano_stop(void) {
    g_nano.running = false;
}

void nano_shutdown(void) {
    if (!g_nano.initialized) return;
    
    g_nano.running = false;
    
    // Shutdown all transports
    for (int i = 0; i < g_nano.transport_count; i++) {
        if (g_nano.transports[i]) {
            g_nano.transports[i]->shutdown();
        }
    }
    
    // Shutdown subsystems
    rkllm_proxy_shutdown();
    io_shutdown();
    
    g_nano.initialized = false;
    g_nano.transport_count = 0;
}

int nano_process_message(const mcp_message_t* request, mcp_message_t* response) {
    if (!request || !response) return -1;
    
    // Get operation from method name
    rkllm_operation_t operation = rkllm_proxy_get_operation_by_name(request->method);
    if (operation == OP_MAX) {
        char error_msg[256];
        snprintf(error_msg, sizeof(error_msg), "Method not found: %s", 
                request->method ? request->method : "null");
        char* error_result = create_error_result(-32601, error_msg);
        mcp_message_create(response, MCP_RESPONSE, request->id, nullptr, error_result);
        mem_free(error_result);
        return -1;
    }
    
    // *** ARCHITECTURAL FIX: Use IO layer instead of direct RKLLM calls ***
    // Create JSON-RPC request for IO layer
    char json_request[4096];
    snprintf(json_request, sizeof(json_request),
             "{\"jsonrpc\":\"2.0\",\"id\":%u,\"method\":\"%s\",\"params\":%s}",
             request->id, request->method, request->params ? request->params : "{}");
    
    // Push request to IO layer
    int push_result = io_push_request(json_request);
    if (push_result != IO_OK) {
        char* error_result = create_error_result(-32603, "IO layer error");
        mcp_message_create(response, MCP_RESPONSE, request->id, nullptr, error_result);
        mem_free(error_result);
        return -1;
    }
    
    // Pop response from IO layer
    char json_response[4096];
    int pop_result = io_pop_response(json_response, sizeof(json_response));
    if (pop_result != IO_OK) {
        char* error_result = create_error_result(-32603, "IO layer timeout");
        mcp_message_create(response, MCP_RESPONSE, request->id, nullptr, error_result);
        mem_free(error_result);
        return -1;
    }
    
    // Parse JSON response and create MCP response
    mcp_message_create(response, MCP_RESPONSE, request->id, nullptr, json_response);
    
    return 0;
}

static void* transport_worker(void* arg) {
    mcp_transport_t* transport = (mcp_transport_t*)arg;
    
    while (g_nano.running) {
        mcp_message_t request = {
            .type = MCP_REQUEST,
            .id = 0,
            .method = nullptr,
            .params = nullptr,
            .params_len = 0
        };
        mcp_message_t response = {
            .type = MCP_RESPONSE,
            .id = 0,
            .method = nullptr,
            .params = nullptr,
            .params_len = 0
        };
        
        // Receive message with timeout
        if (transport->recv(&request, 1000) == 0) {
            // Process message
            if (nano_process_message(&request, &response) == 0) {
                // Send response
                transport->send(&response);
            }
            
            // Clean up
            mcp_message_destroy(&request);
            mcp_message_destroy(&response);
        }
    }
    
    return nullptr;
}
