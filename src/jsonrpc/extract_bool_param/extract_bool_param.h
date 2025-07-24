#ifndef EXTRACT_BOOL_PARAM_H
#define EXTRACT_BOOL_PARAM_H

#include <json-c/json.h>

/**
 * Extract boolean parameter from JSON object
 * @param json_obj JSON object to extract from
 * @param key Key name to extract
 * @param default_value Default value if key not found
 * @return Boolean value (0 or 1)
 */
int extract_bool_param(json_object* json_obj, const char* key, int default_value);

#endif