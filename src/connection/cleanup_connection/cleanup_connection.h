#ifndef CLEANUP_CONNECTION_H
#define CLEANUP_CONNECTION_H

#include "../create_connection/create_connection.h"

/**
 * Cleans up connection resources
 * @param conn Connection to cleanup
 */
void cleanup_connection(Connection* conn);

#endif