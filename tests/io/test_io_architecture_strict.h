#pragma once

// STRICT IO Architecture Test Suite
// These tests will FAIL if NANO bypasses the IO layer

// Main test function
int test_io_architecture_strict(void);

// Individual test functions
void test_nano_must_use_io_queues(void);
void test_direct_rkllm_calls_prohibited(void);
void test_queue_based_timing(void);
