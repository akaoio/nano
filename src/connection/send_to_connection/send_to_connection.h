#ifndef SEND_TO_CONNECTION_H
#define SEND_TO_CONNECTION_H

#include "../create_connection/create_connection.h"
#include <stddef.h>

/**
 * Sends data to specific connection
 * @param conn Connection to send to
 * @param data Data to send
 * @param len Length of data
 * @return Number of bytes sent or -1 on error
 */
int send_to_connection(Connection* conn, const void* data, size_t len);

#endif