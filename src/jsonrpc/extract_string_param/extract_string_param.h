#ifndef EXTRACT_STRING_PARAM_H
#define EXTRACT_STRING_PARAM_H

#include <json-c/json.h>

/**
 * Extract string parameter from JSON object
 * @param json_obj JSON object to extract from
 * @param key Key name to extract
 * @param default_value Default value if key not found
 * @return Allocated string (caller must free) or NULL
 */
char* extract_string_param(json_object* json_obj, const char* key, const char* default_value);

#endif