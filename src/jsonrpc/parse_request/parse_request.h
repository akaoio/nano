#ifndef PARSE_REQUEST_H
#define PARSE_REQUEST_H

#include <json-c/json.h>

/**
 * JSON-RPC request structure
 */
typedef struct {
    char* jsonrpc;        // Protocol version
    char* method;         // Method name
    json_object* params;  // Parameters
    json_object* id;      // Request ID
    int is_valid;         // Validation flag
} JSONRPCRequest;

/**
 * Parses JSON-RPC request from string
 * @param json_str JSON string to parse
 * @return JSONRPCRequest pointer or NULL on error
 */
JSONRPCRequest* parse_request(const char* json_str);

#endif