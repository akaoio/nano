#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <time.h>
#include "../../src/io/io.h"

// Single entry point for all IO tests
int run_all_io_tests(void);

// Individual test functions (internal use)
int test_io_init(void);
int test_model_files(void);
int test_qwenvl_loading(uint32_t* handle);
int test_lora_loading(uint32_t* handle);
int test_qwenvl_inference(uint32_t handle);
int test_lora_inference(uint32_t handle);
int test_error_cases(void);
int test_cleanup(uint32_t qwenvl_handle, uint32_t lora_handle);
void print_test_summary(int failed_tests);
