#include "parse_request.h"
#include <stdlib.h>
#include <string.h>

JSONRPCRequest* parse_request(const char* json_str) {
    if (!json_str) {
        return NULL;
    }
    
    json_object* root = json_tokener_parse(json_str);
    if (!root) {
        return NULL;
    }
    
    JSONRPCRequest* req = malloc(sizeof(JSONRPCRequest));
    if (!req) {
        json_object_put(root);
        return NULL;
    }
    
    memset(req, 0, sizeof(JSONRPCRequest));
    req->is_valid = 0;
    
    // Extract jsonrpc version
    json_object* jsonrpc_obj;
    if (json_object_object_get_ex(root, "jsonrpc", &jsonrpc_obj)) {
        const char* version = json_object_get_string(jsonrpc_obj);
        if (version && strcmp(version, "2.0") == 0) {
            req->jsonrpc = strdup(version);
            if (!req->jsonrpc) {
                return -1; // Memory allocation failed
            }
        }
    }
    
    // Extract method
    json_object* method_obj;
    if (json_object_object_get_ex(root, "method", &method_obj)) {
        const char* method = json_object_get_string(method_obj);
        if (method) {
            req->method = strdup(method);
            if (!req->method) {
                if (req->jsonrpc) free((void*)req->jsonrpc);
                return -1; // Memory allocation failed
            }
        }
    }
    
    // Extract params
    json_object* params_obj;
    if (json_object_object_get_ex(root, "params", &params_obj)) {
        req->params = json_object_get(params_obj); // Increment reference
    }
    
    // Extract id
    json_object* id_obj;
    if (json_object_object_get_ex(root, "id", &id_obj)) {
        req->id = json_object_get(id_obj); // Increment reference
    }
    
    // Validate required fields
    if (req->jsonrpc && req->method) {
        req->is_valid = 1;
    }
    
    json_object_put(root);
    return req;
}