#pragma once

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Error codes
#define IO_OK           0
#define IO_ERROR       -1
#define IO_QUEUE_FULL  -2
#define IO_TIMEOUT     -3

// Constants
#define MAX_WORKERS 5
#define REQUEST_TIMEOUT_MS 30000

// Public interface - only 4 functions
int io_init(void);
int io_push_request(const char* json_request);
int io_pop_response(char* json_response, size_t max_len);
void io_shutdown(void);

#ifdef __cplusplus
}
#endif
