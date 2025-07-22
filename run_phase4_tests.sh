#!/bin/bash

echo "=== PHASE 4 VALIDATION TEST SUITE ==="
echo ""

# Test 1: Error Mapping System
echo "TEST 1: RKLLM Error Mapping System"
echo "-----------------------------------"
echo '{"jsonrpc":"2.0","id":"test1","method":"test_rkllm_error_mapping","params":{"simulate_error":"RKLLM_INVALID_PARAM","expected_json_rpc_code":-32602}}' | timeout 5 ./build/mcp_server 2>/dev/null | tail -1
echo ""

# Test 2: Memory Leak Detection
echo "TEST 2: Memory Leak Detection System"  
echo "------------------------------------"
echo '{"jsonrpc":"2.0","id":"test2","method":"test_memory_leak_detection","params":{"allocations_to_create":5,"bytes_per_allocation":1024,"simulate_leaks":false}}' | timeout 5 ./build/mcp_server 2>/dev/null | tail -1
echo ""

# Test 3: Transport Recovery
echo "TEST 3: Transport Recovery System"
echo "---------------------------------"
echo '{"jsonrpc":"2.0","id":"test3","method":"test_transport_recovery","params":{"transport_type":"http","simulate_failure":"CONNECTION_LOST","test_recovery":true}}' | timeout 5 ./build/mcp_server 2>/dev/null | tail -1
echo ""

echo "=== PHASE 4 VALIDATION COMPLETE ==="