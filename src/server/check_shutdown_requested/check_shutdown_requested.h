#ifndef CHECK_SHUTDOWN_REQUESTED_H
#define CHECK_SHUTDOWN_REQUESTED_H

/**
 * Check if graceful shutdown was requested by signal handler
 * @return 1 if shutdown requested, 0 otherwise
 */
int is_shutdown_requested(void);

#endif