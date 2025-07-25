#ifndef CALL_PROCESS_IMAGE_H
#define CALL_PROCESS_IMAGE_H

#include <json-c/json.h>

// Process image and return embeddings as JSON
json_object* call_process_image(json_object* params);

// Initialize image processor
json_object* call_init_image_processor(json_object* params);

// Cleanup image processor
json_object* call_cleanup_image_processor(void);

#endif // CALL_PROCESS_IMAGE_H