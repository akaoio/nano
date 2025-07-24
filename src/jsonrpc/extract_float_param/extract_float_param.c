#include "extract_float_param.h"

float extract_float_param(json_object* json_obj, const char* key, float default_value) {
    if (!json_obj || !key) return default_value;
    
    json_object* value_obj;
    if (json_object_object_get_ex(json_obj, key, &value_obj)) {
        return (float)json_object_get_double(value_obj);
    }
    
    return default_value;
}