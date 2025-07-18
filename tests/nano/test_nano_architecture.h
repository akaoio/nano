#pragma once

// NANO Architecture Test Suite
// Tests NANO's interaction with IO layer as designed

// Main test function
int test_nano_architecture(void);

// Individual test functions
void test_nano_uses_io_layer(void);
void test_nano_mcp_compliance(void);
void test_nano_transport_isolation(void);
void test_nano_error_propagation(void);
