#ifndef EXTRACT_OBJECT_PARAM_H
#define EXTRACT_OBJECT_PARAM_H

#include <json-c/json.h>

/**
 * Extract object parameter from JSON object
 * @param json_obj JSON object to extract from
 * @param key Key name to extract
 * @return JSON object (with increased ref count) or NULL
 */
json_object* extract_object_param(json_object* json_obj, const char* key);

#endif