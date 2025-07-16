#!/bin/bash

# Test Suite for New Architecture
# Run all tests in the new structure

set -e  # Exit on any error

echo "🧪 NANO PROJECT - NEW ARCHITECTURE TEST SUITE"
echo "=============================================="

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Test counters
TOTAL_TESTS=0
PASSED_TESTS=0
FAILED_TESTS=0

# Function to run a test
run_test() {
    local test_name=$1
    local test_dir=$2
    
    TOTAL_TESTS=$((TOTAL_TESTS + 1))
    
    echo -e "${BLUE}[TEST $TOTAL_TESTS]${NC} Running $test_name..."
    
    if (cd "$test_dir" && make clean &>/dev/null && make test); then
        echo -e "${GREEN}✅ $test_name PASSED${NC}"
        PASSED_TESTS=$((PASSED_TESTS + 1))
    else
        echo -e "${RED}❌ $test_name FAILED${NC}"
        FAILED_TESTS=$((FAILED_TESTS + 1))
    fi
    echo
}

# Start testing
echo -e "${YELLOW}Starting test suite...${NC}"
echo

# Common utilities tests
echo -e "${BLUE}=== COMMON UTILITIES TESTS ===${NC}"
run_test "JSON Utils" "tests_new/common/test_json_utils"

# IO core tests
echo -e "${BLUE}=== IO CORE TESTS ===${NC}"
run_test "Queue Component" "tests_new/io/test_queue"
run_test "Worker Pool Component" "tests_new/io/test_worker_pool"
run_test "IO Component" "tests_new/io/test_io"

# IO mapping tests
echo -e "${BLUE}=== IO MAPPING TESTS ===${NC}"
run_test "Handle Pool Component" "tests_new/io/test_handle_pool"
run_test "RKLLM Proxy Component" "tests_new/io/test_rkllm_proxy"

# Nano system tests
echo -e "${BLUE}=== NANO SYSTEM TESTS ===${NC}"
run_test "System Components" "tests_new/nano/test_system"

# Nano validation tests
echo -e "${BLUE}=== NANO VALIDATION TESTS ===${NC}"
run_test "Validation Components" "tests_new/nano/test_validation"

# Summary
echo -e "${BLUE}=== TEST SUMMARY ===${NC}"
echo "Total tests: $TOTAL_TESTS"
echo -e "Passed: ${GREEN}$PASSED_TESTS${NC}"
echo -e "Failed: ${RED}$FAILED_TESTS${NC}"

if [ $FAILED_TESTS -eq 0 ]; then
    echo -e "${GREEN}🎉 ALL TESTS PASSED!${NC}"
    exit 0
else
    echo -e "${RED}💥 Some tests failed!${NC}"
    exit 1
fi
