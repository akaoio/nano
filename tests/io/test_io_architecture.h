#pragma once

// IO Architecture Test Suite
// Tests the queue-based architecture as designed
// Merged from multiple test files for better organization

// Main test function
int test_io_architecture(void);

// Individual test functions
void test_io_json_parsing(void);
void test_io_queue_operations(void);
void test_io_worker_pool(void);
void test_io_error_handling(void);
void test_io_queue_full(void);
void test_nano_uses_io_queues(void);
