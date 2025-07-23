#ifndef INSTALL_SIGNAL_HANDLERS_H
#define INSTALL_SIGNAL_HANDLERS_H

/**
 * Install signal handlers for crash prevention and graceful recovery
 * Handles SIGSEGV, SIGBUS, SIGFPE, SIGTERM, SIGINT
 * @return 0 on success, -1 on failure
 */
int install_signal_handlers(void);

#endif