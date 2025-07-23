#ifndef ADD_CONNECTION_H
#define ADD_CONNECTION_H

#include "../create_connection/create_connection.h"

/**
 * Connection manager structure
 */
typedef struct {
    Connection** connections;   // Array of connection pointers
    int max_connections;       // Maximum connections
    int count;                 // Current connection count
} ConnectionManager;

/**
 * Adds connection to manager
 * @param manager Connection manager
 * @param conn Connection to add
 * @return 0 on success, -1 on error
 */
int add_connection(ConnectionManager* manager, Connection* conn);

#endif