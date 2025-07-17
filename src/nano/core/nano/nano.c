#include "nano.h"
#include "../../../common/common.h"
#include "../../../common/constants.h"
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
        char error[256];
        snprintf(error, sizeof(error), "{\"code\":-32601,\"message\":\"Method not found: %s\"}", 
                request->method ? request->method : "null");
        mcp_message_create(response, MCP_RESPONSE, request->id, nullptr, error);
        return -1;
    }
    
    // Create RKLLM request using designated initializers
    rkllm_request_t rkllm_request = {
        .operation = operation,
        .handle_id = 0, // Will be extracted from params if needed
        .params_json = request->params,
        .params_size = request->params ? strlen(request->params) : 0
    };
    
    // Execute operation using designated initializers
    rkllm_result_t rkllm_result = {
        .result_data = nullptr,
        .result_size = 0
    };
    int ret = rkllm_proxy_execute(&rkllm_request, &rkllm_result);
    
    // Create response
    if (ret == 0) {
        mcp_message_create(response, MCP_RESPONSE, request->id, nullptr, 
                          rkllm_result.result_data ? rkllm_result.result_data : "null");
    } else {
        char error[8256];
        snprintf(error, sizeof(error), "{\"code\":%d,\"message\":\"%s\"}", ret, 
                rkllm_result.result_data ? rkllm_result.result_data : "Operation failed");
        mcp_message_create(response, MCP_RESPONSE, request->id, nullptr, error);
    }
    
    // Cleanup
    rkllm_proxy_free_result(&rkllm_result);
    
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
