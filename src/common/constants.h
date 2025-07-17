#ifndef CONSTANTS_H
#define CONSTANTS_H

// Worker pool configuration
#define MAX_WORKERS 5

// Queue configuration
#define QUEUE_SIZE 1024
#define REQUEST_TIMEOUT_MS 30000

// Handle pool configuration
#define MAX_HANDLES 8

// Memory limits
#define MAX_REQUEST_SIZE 4096
#define MAX_RESPONSE_SIZE 8192

// Error codes
#define ERR_OK 0
#define ERR_INVALID_PARAM -1
#define ERR_QUEUE_FULL -2
#define ERR_QUEUE_EMPTY -3
#define ERR_TIMEOUT -4
#define ERR_MEMORY_LIMIT -5

#endif /* CONSTANTS_H */