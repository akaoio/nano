#ifndef FIND_CONNECTION_H
#define FIND_CONNECTION_H

#include "../add_connection/add_connection.h"

/**
 * Finds connection by file descriptor
 * @param manager Connection manager
 * @param fd File descriptor to find
 * @return Connection pointer or NULL if not found
 */
Connection* find_connection(ConnectionManager* manager, int fd);

#endif