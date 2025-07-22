#!/bin/bash

# RKLLM MCP Server - Test Runner Script
# Runs all Phase 3-5 tests in separate executables

set -e

echo "=== RKLLM MCP Server Test Suite ==="
echo "Running all Phase 3-5 tests..."
echo

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Test results tracking
total_tests=0
passed_tests=0
failed_tests=0

# Function to run a single test
run_test() {
    local test_name=$1
    local test_path=$2
    
    echo -e "${YELLOW}Running $test_name...${NC}"
    
    if [ -f "$test_path" ]; then
        if $test_path; then
            echo -e "${GREEN}✓ $test_name PASSED${NC}"
            ((passed_tests++))
        else
            echo -e "${RED}✗ $test_name FAILED${NC}"
            ((failed_tests++))
        fi
    else
        echo -e "${RED}✗ $test_name - Executable not found at $test_path${NC}"
        ((failed_tests++))
    fi
    
    ((total_tests++))
    echo
}

echo "=== Phase 3 Tests ==="
run_test "HTTP Buffer Manager" "./phase3/test_http_buffer_manager"
run_test "Streaming Callback" "./phase3/test_streaming_callback"

echo "=== Phase 4 Tests ==="
run_test "Error Mapping" "./phase4/test_error_mapping"
run_test "Memory Tracker" "./phase4/test_memory_tracker" 
run_test "Transport Recovery" "./phase4/test_transport_recovery"

echo "=== Phase 5 Tests ==="
run_test "Logging System" "./phase5/test_logging_system"
run_test "Metrics System" "./phase5/test_metrics_system"
run_test "Performance System" "./phase5/test_performance_system"

echo "=== Final Test Summary ==="
echo "Total tests: $total_tests"
echo -e "Passed: ${GREEN}$passed_tests${NC}"
echo -e "Failed: ${RED}$failed_tests${NC}"

if [ $failed_tests -eq 0 ]; then
    echo -e "${GREEN}🎉 All tests passed!${NC}"
    exit 0
else
    echo -e "${RED}❌ $failed_tests test(s) failed${NC}"
    exit 1
fi