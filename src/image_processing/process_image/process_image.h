#ifndef PROCESS_IMAGE_H
#define PROCESS_IMAGE_H

#include <stdint.h>
#include <stddef.h>
#include "../../utils/image_enc.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    rknn_app_context_t encoder_ctx;
    int initialized;
} ImageProcessor;

// Initialize image processor with RKNN vision encoder model
int init_image_processor(ImageProcessor* processor, const char* model_path, int core_num);

// Process base64 encoded image and generate embeddings
int process_image_base64(ImageProcessor* processor, const char* base64_data, 
                        float* embeddings, size_t* embedding_size);

// Process raw image data and generate embeddings
int process_image_data(ImageProcessor* processor, uint8_t* image_data, 
                      int width, int height, int channels,
                      float* embeddings, size_t* embedding_size);

// Clean up image processor
int cleanup_image_processor(ImageProcessor* processor);

#ifdef __cplusplus
}
#endif

#endif // PROCESS_IMAGE_H