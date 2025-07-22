#pragma once

// Temporary WebSocket stub to avoid compilation errors
// TODO: Replace with proper WebSocket library implementation

typedef int ws_cli_conn_t;

struct ws_server {
    const char* host;
    int port;
    int thread_loop;
    int timeout_ms;
    struct {
        void (*onopen)(ws_cli_conn_t);
        void (*onclose)(ws_cli_conn_t);
        void (*onmessage)(ws_cli_conn_t, const unsigned char*, uint64_t, int);
    } evs;
};

// Stub functions
static inline int ws_socket(struct ws_server* server) { return -1; }
static inline const char* ws_getaddress(ws_cli_conn_t conn) { return "stub"; }
static inline const char* ws_getport(ws_cli_conn_t conn) { return "0"; }
static inline int ws_sendframe_txt(ws_cli_conn_t conn, const char* data) { return -1; }
static inline void ws_close_client(ws_cli_conn_t conn) { }