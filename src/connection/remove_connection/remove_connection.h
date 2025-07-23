#ifndef REMOVE_CONNECTION_H
#define REMOVE_CONNECTION_H

#include "../add_connection/add_connection.h"

/**
 * Removes connection from manager
 * @param manager Connection manager
 * @param fd File descriptor to remove
 * @return 0 on success, -1 on error
 */
int remove_connection(ConnectionManager* manager, int fd);

#endif