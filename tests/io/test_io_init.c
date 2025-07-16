#include "test_io.h"

int test_io_init(void) {
    printf("Test 1: io_init()... ");
    if (io_init() != 0) {
        printf("FAIL\n");
        return 1;
    }
    printf("PASS\n");
    return 0;
}
