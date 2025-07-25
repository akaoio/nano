#ifndef HANDLE_REQUEST_H
#define HANDLE_REQUEST_H

#include "../parse_request/parse_request.h"
#include "../../connection/create_connection/create_connection.h"

/**
 * Handles JSON-RPC request and sends response to connection
 * @param req Parsed JSON-RPC request
 * @param conn Connection to send response to
 * @return 0 on success, -1 on error
 */
int handle_request(JSONRPCRequest* req, Connection* conn);

/**
 * Helper function to send error responses
 * @param conn Connection to send error to
 * @param id Request ID (can be NULL)
 * @param code Error code
 * @param message Error message
 * @return 0 on success, -1 on error
 */
int send_error_response(Connection* conn, json_object* id, int code, const char* message);

#endif