#ifndef FORMAT_RESPONSE_H
#define FORMAT_RESPONSE_H

#include <json-c/json.h>

/**
 * Formats JSON-RPC response
 * @param id Request ID (can be NULL)
 * @param result Result object 
 * @return JSON string or NULL on error (caller must free)
 */
char* format_response(json_object* id, json_object* result);

#endif