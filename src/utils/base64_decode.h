#ifndef BASE64_DECODE_H
#define BASE64_DECODE_H

#include <stddef.h>

/**
 * Decode base64 string to binary data
 * @param input Base64 encoded string
 * @param output Buffer to store decoded data (caller must free)
 * @param output_len Length of decoded data
 * @return 0 on success, -1 on error
 */
int base64_decode(const char* input, unsigned char** output, size_t* output_len);

/**
 * Calculate the decoded length of a base64 string
 * @param input Base64 encoded string
 * @return Expected decoded length
 */
size_t base64_decoded_length(const char* input);

#endif // BASE64_DECODE_H