#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <time.h>
#include "../../src/io/io.h"
#include "../../src/io/system_info.h"

// Single entry point for all IO tests
int run_all_io_tests(void);

// Individual test functions
int test_io_init(void);
int test_model_files(void);
int test_error_cases(void);
int test_cleanup(uint32_t qwenvl_handle, uint32_t lora_handle);

// Combined test functions with cleanup
int test_qwenvl_full(void);
int test_lora_full(void);

// Inference helper functions
int test_model_inference(uint32_t handle, const char* model_name);
void test_cleanup_model(uint32_t handle, const char* model_name);
