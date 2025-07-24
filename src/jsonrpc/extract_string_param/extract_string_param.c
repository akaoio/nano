#include "extract_string_param.h"
#include <stdlib.h>
#include <string.h>

char* extract_string_param(json_object* json_obj, const char* key, const char* default_value) {
    if (!json_obj || !key) return NULL;
    
    json_object* value_obj;
    if (json_object_object_get_ex(json_obj, key, &value_obj)) {
        const char* str_val = json_object_get_string(value_obj);
        if (str_val) {
            return strdup(str_val);
        }
    }
    
    if (default_value) {
        return strdup(default_value);
    }
    
    return NULL;
}