#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

// C23 compatibility
#if __STDC_VERSION__ >= 202311L
#define NODISCARD [[nodiscard]]
#define MAYBE_UNUSED [[maybe_unused]]
#else
#define NODISCARD __attribute__((warn_unused_result))
#define MAYBE_UNUSED __attribute__((unused))
#endif

#define IO_OK 0
#define IO_ERROR -1
#define IO_TIMEOUT -2

// Forward declaration for callback
typedef void (*nano_callback_t)(const char* json_response, void* userdata);

/**
 * @brief Initialize IO system with callback to NANO
 * @param callback Function to call when response is ready
 * @param userdata Data to pass to callback
 * @return IO_OK on success, IO_ERROR on failure
 */
NODISCARD int io_init(nano_callback_t callback, void* userdata);

/**
 * @brief Process JSON request directly and synchronously
 * @param json_request JSON-RPC request string
 * @return IO_OK on success, error code on failure
 */
NODISCARD int io_push_request(const char* json_request);

/**
 * @brief Set streaming callback for real-time responses
 * @param callback Function to call for streaming chunks
 * @param userdata Data to pass to callback
 */
void io_set_streaming_callback(nano_callback_t callback, void* userdata);

/**
 * @brief Shutdown IO system
 */
void io_shutdown(void);

// Internal functions
NODISCARD int io_parse_json_request_with_handle(const char* json_request, uint32_t* request_id, 
                                                uint32_t* handle_id, char* method, char* params);