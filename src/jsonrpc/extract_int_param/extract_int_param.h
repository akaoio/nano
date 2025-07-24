#ifndef EXTRACT_INT_PARAM_H
#define EXTRACT_INT_PARAM_H

#include <json-c/json.h>

/**
 * Extract integer parameter from JSON object
 * @param json_obj JSON object to extract from
 * @param key Key name to extract
 * @param default_value Default value if key not found
 * @return Integer value
 */
int extract_int_param(json_object* json_obj, const char* key, int default_value);

#endif