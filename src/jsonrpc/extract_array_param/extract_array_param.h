#ifndef EXTRACT_ARRAY_PARAM_H
#define EXTRACT_ARRAY_PARAM_H

#include <json-c/json.h>

/**
 * Extract array parameter from JSON object
 * @param json_obj JSON object to extract from
 * @param key Key name to extract
 * @return JSON array (with increased ref count) or NULL
 */
json_object* extract_array_param(json_object* json_obj, const char* key);

#endif