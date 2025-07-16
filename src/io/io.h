#pragma once

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Public interface - only 4 functions
int io_init(void);
int io_push_request(const char* json_request);
int io_pop_response(char* json_response, size_t max_len);
void io_shutdown(void);

// Error codes
#define IO_OK 0
#define IO_ERROR -1
#define IO_QUEUE_FULL -2
#define IO_TIMEOUT -3
#define IO_INVALID_REQUEST -4
#define IO_MEMORY_ERROR -5
#define IO_HANDLE_NOT_FOUND -6

#ifdef __cplusplus
}
#endif
