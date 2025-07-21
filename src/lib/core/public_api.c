#include "mcp/server.h"
#include "server.h"
#include <stdlib.h>
#include <string.h>

// Bridge public API to internal implementation

int mcp_server_init(mcp_server_t* server, const mcp_server_config_t* config) {
    if (!server || !config) {
        return -1;
    }
    
    // Initialize the public server structure
    memset(server, 0, sizeof(mcp_server_t));
    
    // Allocate internal server data
    mcp_server_internal_t* internal = malloc(sizeof(mcp_server_internal_t));
    if (!internal) {
        return -1;
    }
    
    // Initialize internal server
    int result = mcp_server_internal_init(internal, config);
    if (result != 0) {
        free(internal);
        return result;
    }
    
    // Link internal data to public structure
    server->internal_data = internal;
    server->initialized = true;
    server->running = false;
    server->uptime_seconds = 0;
    
    return 0;
}

int mcp_server_start(mcp_server_t* server) {
    if (!server || !server->initialized || !server->internal_data) {
        return -1;
    }
    
    mcp_server_internal_t* internal = (mcp_server_internal_t*)server->internal_data;
    int result = mcp_server_internal_start(internal);
    
    if (result == 0) {
        server->running = true;
    }
    
    return result;
}

int mcp_server_run_event_loop(mcp_server_t* server) {
    if (!server || !server->running || !server->internal_data) {
        return -1;
    }
    
    mcp_server_internal_t* internal = (mcp_server_internal_t*)server->internal_data;
    return mcp_server_internal_run_event_loop(internal);
}

int mcp_server_stop(mcp_server_t* server) {
    if (!server || !server->internal_data) {
        return -1;
    }
    
    mcp_server_internal_t* internal = (mcp_server_internal_t*)server->internal_data;
    int result = mcp_server_internal_stop(internal);
    
    if (result == 0) {
        server->running = false;
    }
    
    return result;
}

void mcp_server_shutdown(mcp_server_t* server) {
    if (!server || !server->internal_data) {
        return;
    }
    
    mcp_server_internal_t* internal = (mcp_server_internal_t*)server->internal_data;
    mcp_server_internal_shutdown(internal);
    
    // Clean up
    free(internal);
    server->internal_data = NULL;
    server->initialized = false;
    server->running = false;
    server->uptime_seconds = 0;
}

const char* mcp_server_get_status(const mcp_server_t* server) {
    if (!server || !server->internal_data) {
        return "uninitialized";
    }
    
    mcp_server_internal_t* internal = (mcp_server_internal_t*)server->internal_data;
    return mcp_server_internal_get_status(internal);
}

void mcp_server_get_stats(const mcp_server_t* server, 
                         uint64_t* requests, uint64_t* responses, 
                         uint64_t* errors, uint64_t* uptime) {
    if (!server || !server->internal_data || !requests || !responses || !errors || !uptime) {
        if (requests) *requests = 0;
        if (responses) *responses = 0;
        if (errors) *errors = 0;
        if (uptime) *uptime = 0;
        return;
    }
    
    mcp_server_internal_t* internal = (mcp_server_internal_t*)server->internal_data;
    mcp_server_internal_get_stats(internal, requests, responses, errors, uptime);
    
    // Update public uptime
    *uptime = server->uptime_seconds;
}