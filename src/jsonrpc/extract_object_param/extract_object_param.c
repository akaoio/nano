#include "extract_object_param.h"

json_object* extract_object_param(json_object* json_obj, const char* key) {
    if (!json_obj || !key) return NULL;
    
    json_object* value_obj;
    if (json_object_object_get_ex(json_obj, key, &value_obj)) {
        if (json_object_is_type(value_obj, json_type_object)) {
            return json_object_get(value_obj);
        }
    }
    
    return NULL;
}