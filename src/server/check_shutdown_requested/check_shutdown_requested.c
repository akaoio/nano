#include "check_shutdown_requested.h"

// External declaration - defined in install_signal_handlers.c
extern int is_shutdown_requested(void);

int check_shutdown_requested(void) {
    return is_shutdown_requested();
}