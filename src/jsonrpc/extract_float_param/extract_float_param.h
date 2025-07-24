#ifndef EXTRACT_FLOAT_PARAM_H
#define EXTRACT_FLOAT_PARAM_H

#include <json-c/json.h>

/**
 * Extract float parameter from JSON object
 * @param json_obj JSON object to extract from
 * @param key Key name to extract
 * @param default_value Default value if key not found
 * @return Float value
 */
float extract_float_param(json_object* json_obj, const char* key, float default_value);

#endif