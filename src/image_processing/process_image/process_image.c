#include "process_image.h"
#include "../../utils/base64_decode.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define IMAGE_HEIGHT 392
#define IMAGE_WIDTH 392
#define IMAGE_TOKEN_NUM 196
#define EMBED_SIZE 1536

// Forward declaration
int process_image_data(ImageProcessor* processor, uint8_t* image_data, 
                      int width, int height, int channels,
                      float* embeddings, size_t* embedding_size);

// Simple image resizing function (nearest neighbor)
static void resize_image(uint8_t* src, int src_w, int src_h, int channels,
                        uint8_t* dst, int dst_w, int dst_h) {
    float x_ratio = (float)src_w / dst_w;
    float y_ratio = (float)src_h / dst_h;
    
    for (int y = 0; y < dst_h; y++) {
        for (int x = 0; x < dst_w; x++) {
            int src_x = (int)(x * x_ratio);
            int src_y = (int)(y * y_ratio);
            
            for (int c = 0; c < channels; c++) {
                dst[(y * dst_w + x) * channels + c] = 
                    src[(src_y * src_w + src_x) * channels + c];
            }
        }
    }
}

// Expand image to square with padding
static void expand_to_square(uint8_t* src, int width, int height, int channels,
                           uint8_t** dst, int* new_size) {
    int size = (width > height) ? width : height;
    *new_size = size;
    *dst = (uint8_t*)malloc(size * size * channels);
    
    // Fill with gray background (127.5)
    memset(*dst, 127, size * size * channels);
    
    // Calculate padding
    int x_offset = (size - width) / 2;
    int y_offset = (size - height) / 2;
    
    // Copy original image to center
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            for (int c = 0; c < channels; c++) {
                (*dst)[((y + y_offset) * size + (x + x_offset)) * channels + c] = 
                    src[(y * width + x) * channels + c];
            }
        }
    }
}

int init_image_processor(ImageProcessor* processor, const char* model_path, int core_num) {
    if (!processor || !model_path) {
        return -1;
    }
    
    memset(processor, 0, sizeof(ImageProcessor));
    
    // Note: This demo version doesn't support core_num parameter  
    (void)core_num; // Suppress unused parameter warning
    int ret = init_imgenc(model_path, &processor->encoder_ctx);
    if (ret != 0) {
        printf("Failed to initialize image encoder: %d\n", ret);
        return ret;
    }
    
    processor->initialized = 1;
    return 0;
}

int process_image_base64(ImageProcessor* processor, const char* base64_data, 
                        float* embeddings, size_t* embedding_size) {
    if (!processor || !processor->initialized || !base64_data || !embeddings) {
        return -1;
    }
    
    // Decode base64 to binary data
    unsigned char* decoded_data;
    size_t decoded_size;
    int decode_ret = base64_decode(base64_data, &decoded_data, &decoded_size);
    if (decode_ret != 0 || !decoded_data) {
        printf("Failed to decode base64 image data\n");
        return -1;
    }
    
    // For now, assume this is raw RGB data - in practice you'd use a proper image decoder
    // This is a simplified implementation
    int width = 224;  // Default assumption
    int height = 224;
    int channels = 3;
    
    int ret = process_image_data(processor, decoded_data, width, height, channels, 
                                embeddings, embedding_size);
    
    free(decoded_data);
    return ret;
}

int process_image_data(ImageProcessor* processor, uint8_t* image_data, 
                      int width, int height, int channels,
                      float* embeddings, size_t* embedding_size) {
    if (!processor || !processor->initialized || !image_data || !embeddings) {
        return -1;
    }
    
    // Expand to square
    uint8_t* square_data;
    int square_size;
    expand_to_square(image_data, width, height, channels, &square_data, &square_size);
    
    // Resize to target size
    uint8_t* resized_data = (uint8_t*)malloc(IMAGE_WIDTH * IMAGE_HEIGHT * channels);
    resize_image(square_data, square_size, square_size, channels,
                resized_data, IMAGE_WIDTH, IMAGE_HEIGHT);
    
    // Run image encoder
    float img_vec[IMAGE_TOKEN_NUM * EMBED_SIZE];
    int ret = run_imgenc(&processor->encoder_ctx, resized_data, img_vec);
    
    if (ret == 0) {
        memcpy(embeddings, img_vec, sizeof(img_vec));
        *embedding_size = IMAGE_TOKEN_NUM * EMBED_SIZE;
    }
    
    free(square_data);
    free(resized_data);
    
    return ret;
}

int cleanup_image_processor(ImageProcessor* processor) {
    if (!processor || !processor->initialized) {
        return -1;
    }
    
    int ret = release_imgenc(&processor->encoder_ctx);
    processor->initialized = 0;
    
    return ret;
}