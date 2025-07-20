#ifndef COMMON_ERRORS_H
#define COMMON_ERRORS_H

/**
 * @file errors.h
 * @brief Common error codes and error handling utilities
 */

#ifdef __cplusplus
extern "C" {
#endif

// Generic return codes
#define MCP_OK                0
#define MCP_ERROR            -1
#define MCP_INVALID_PARAM    -2
#define MCP_NOT_INITIALIZED  -3
#define MCP_ALREADY_INIT     -4
#define MCP_OUT_OF_MEMORY    -5
#define MCP_TIMEOUT          -6
#define MCP_NOT_FOUND        -7
#define MCP_INVALID_STATE    -8

// Transport specific errors
#define MCP_TRANSPORT_ERROR       -100
#define MCP_TRANSPORT_DISCONNECTED -101
#define MCP_TRANSPORT_TIMEOUT     -102
#define MCP_TRANSPORT_INVALID_DATA -103

// Protocol specific errors  
#define MCP_PROTOCOL_ERROR        -200
#define MCP_PROTOCOL_PARSE_ERROR  -201
#define MCP_PROTOCOL_INVALID_JSON -202
#define MCP_PROTOCOL_INVALID_RPC  -203

// Stream specific errors
#define MCP_STREAM_ERROR         -300
#define MCP_STREAM_NOT_FOUND     -301
#define MCP_STREAM_EXPIRED       -302
#define MCP_STREAM_INVALID_STATE -303
#define MCP_STREAM_BUFFER_FULL   -304

// System errors
#define MCP_SYSTEM_ERROR      -400
#define MCP_FILE_ERROR        -401
#define MCP_NETWORK_ERROR     -402
#define MCP_RESOURCE_ERROR    -403

/**
 * @brief Convert error code to human-readable string
 * @param error_code Error code
 * @return Error description string (do not free)
 */
const char* mcp_strerror(int error_code);

/**
 * @brief Check if error code indicates success
 * @param error_code Error code to check
 * @return true if success, false if error
 */
static inline bool mcp_is_success(int error_code) {
    return error_code >= 0;
}

/**
 * @brief Check if error code indicates failure
 * @param error_code Error code to check
 * @return true if error, false if success
 */
static inline bool mcp_is_error(int error_code) {
    return error_code < 0;
}

#ifdef __cplusplus
}
#endif

#endif // COMMON_ERRORS_H