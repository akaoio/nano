#include "extract_bool_param.h"

int extract_bool_param(json_object* json_obj, const char* key, int default_value) {
    if (!json_obj || !key) return default_value;
    
    json_object* value_obj;
    if (json_object_object_get_ex(json_obj, key, &value_obj)) {
        return json_object_get_boolean(value_obj);
    }
    
    return default_value;
}