#pragma once

#include <stddef.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>

/**
 * @brief Process received buffer by removing trailing newline and null-terminating
 * @param buffer Buffer to process
 * @param received Number of bytes received
 * @return Processed buffer length
 */
size_t process_received_buffer(char* buffer, size_t received);

/**
 * @brief Check if receive operation resulted in timeout or error
 * @param result Result from recv/read operation
 * @return true if timeout or error, false otherwise
 */
bool is_receive_timeout_or_error(ssize_t result);

/**
 * @brief Standard buffer processing for all transports
 * @param buffer Buffer to process
 * @param buffer_size Size of buffer
 * @param received Number of bytes received
 * @return 0 on success, -1 on error
 */
int standard_buffer_processing(char* buffer, size_t buffer_size, ssize_t received);

/**
 * @brief Create a TCP socket
 * @return Socket fd on success, -1 on error
 */
int create_tcp_socket(void);

/**
 * @brief Create a UDP socket
 * @return Socket fd on success, -1 on error
 */
int create_udp_socket(void);

/**
 * @brief Setup socket address structure
 * @param addr Address structure to fill
 * @param host Host address (nullptr for INADDR_ANY)
 * @param port Port number
 * @return 0 on success, -1 on error
 */
int setup_socket_address(struct sockaddr_in* addr, const char* host, int port);

/**
 * @brief Setup socket for server mode (bind + listen for TCP)
 * @param socket_fd Socket file descriptor
 * @param addr Address structure
 * @param is_tcp Whether this is TCP (needs listen) or UDP
 * @return 0 on success, -1 on error
 */
int setup_server_socket(int socket_fd, struct sockaddr_in* addr, bool is_tcp);

/**
 * @brief Connect socket to server (TCP only)
 * @param socket_fd Socket file descriptor
 * @param addr Address structure
 * @return 0 on success, -1 on error
 */
int connect_socket(int socket_fd, struct sockaddr_in* addr);

/**
 * @brief Set socket to non-blocking mode
 * @param socket_fd Socket file descriptor
 * @return 0 on success, -1 on error
 */
int set_socket_nonblocking(int socket_fd);

/**
 * @brief Close socket safely
 * @param socket_fd Socket file descriptor
 */
void close_socket(int socket_fd);

/**
 * @brief Setup select timeout
 * @param timeout_ms Timeout in milliseconds
 * @param tv Timeval structure to fill
 */
void setup_select_timeout(int timeout_ms, struct timeval* tv);

/**
 * @brief Check if socket is ready for read with timeout
 * @param socket_fd Socket file descriptor
 * @param timeout_ms Timeout in milliseconds
 * @return 1 if ready, 0 if timeout, -1 on error
 */
int socket_select_read(int socket_fd, int timeout_ms);