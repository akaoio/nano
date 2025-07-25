#include "base64_decode.h"
#include <stdlib.h>
#include <string.h>

static const unsigned char base64_decode_table[256] = {
    64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
    64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
    64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 62, 64, 64, 64, 63,
    52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 64, 64, 64, 64, 64, 64,
    64,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14,
    15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 64, 64, 64, 64, 64,
    64, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
    41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 64, 64, 64, 64, 64,
    64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
    64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
    64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
    64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
    64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
    64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
    64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
    64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64
};

size_t base64_decoded_length(const char* input) {
    if (!input) return 0;
    
    size_t len = strlen(input);
    if (len == 0) return 0;
    
    size_t padding = 0;
    if (len >= 2) {
        if (input[len - 1] == '=') padding++;
        if (input[len - 2] == '=') padding++;
    }
    
    return (len * 3) / 4 - padding;
}

int base64_decode(const char* input, unsigned char** output, size_t* output_len) {
    if (!input || !output || !output_len) {
        return -1;
    }
    
    size_t input_len = strlen(input);
    if (input_len == 0) {
        *output = NULL;
        *output_len = 0;
        return 0;
    }
    
    // Input length must be multiple of 4
    if (input_len % 4 != 0) {
        return -1;
    }
    
    *output_len = base64_decoded_length(input);
    *output = malloc(*output_len);
    if (!*output) {
        return -1;
    }
    
    size_t j = 0;
    for (size_t i = 0; i < input_len; i += 4) {
        unsigned char a = base64_decode_table[(unsigned char)input[i]];
        unsigned char b = base64_decode_table[(unsigned char)input[i + 1]];
        unsigned char c = base64_decode_table[(unsigned char)input[i + 2]];
        unsigned char d = base64_decode_table[(unsigned char)input[i + 3]];
        
        // Check for invalid characters
        if (a == 64 || b == 64) {
            free(*output);
            *output = NULL;
            return -1;
        }
        
        (*output)[j++] = (a << 2) | (b >> 4);
        
        if (c != 64) {
            (*output)[j++] = (b << 4) | (c >> 2);
        }
        
        if (d != 64) {
            (*output)[j++] = (c << 6) | d;
        }
    }
    
    return 0;
}