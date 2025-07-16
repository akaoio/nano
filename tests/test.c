#include <stdio.h>
#include <stdlib.h>
#include "io/test_io.h"

int main() {
    int failed = run_all_io_tests();
    return failed;
}
