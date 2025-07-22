#ifndef NANO_TIME_UTILS_H
#define NANO_TIME_UTILS_H

#include <stdint.h>

/**
 * @brief Get current timestamp in milliseconds
 * @return Timestamp in milliseconds since epoch
 */
uint64_t get_timestamp_ms(void);

#endif // NANO_TIME_UTILS_H