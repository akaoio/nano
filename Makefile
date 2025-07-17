# Makefile for NANO project - New Architecture
# Modern C build system with comprehensive source file support

CC = clang
CFLAGS = -std=c2x -Wall -Wextra -g
LDFLAGS = -lpthread -ldl -Lsrc/libs/rkllm -lrkllmrt -Wl,-rpath,src/libs/rkllm

# Directories
SRC_DIR = src
BUILD_DIR = build
TESTS_DIR = tests
MODELS_DIR = models

# Source files (without main.c)
COMMON_SRCS = $(SRC_DIR)/common/json_utils/json_utils.c \
              $(SRC_DIR)/common/memory_utils/memory_utils.c \
              $(SRC_DIR)/common/string_utils/string_utils.c

IO_CORE_SRCS = $(SRC_DIR)/io/core/queue/queue.c \
               $(SRC_DIR)/io/core/worker_pool/worker_pool.c \
               $(SRC_DIR)/io/core/io/io.c \
               $(SRC_DIR)/io/core/io/io_worker.c \
               $(SRC_DIR)/io/operations.c

IO_MAPPING_SRCS = $(SRC_DIR)/io/mapping/handle_pool/handle_pool.c \
                  $(SRC_DIR)/io/mapping/handle_pool/handle_pool_utils.c \
                  $(SRC_DIR)/io/mapping/rkllm_proxy/rkllm_proxy.c \
                  $(SRC_DIR)/io/mapping/rkllm_proxy/rkllm_operations.c

NANO_SYSTEM_SRCS = $(SRC_DIR)/nano/system/system_info/system_info.c \
                   $(SRC_DIR)/nano/system/system_info/system_memory.c \
                   $(SRC_DIR)/nano/system/system_info/system_model.c \
                   $(SRC_DIR)/nano/system/resource_mgr/resource_mgr.c

NANO_VALIDATION_SRCS = $(SRC_DIR)/nano/validation/model_checker/model_checker.c

NANO_TRANSPORT_SRCS = $(SRC_DIR)/nano/transport/mcp_base/mcp_base.c \
                      $(SRC_DIR)/nano/transport/udp_transport/udp_transport.c \
                      $(SRC_DIR)/nano/transport/tcp_transport/tcp_transport.c \
                      $(SRC_DIR)/nano/transport/http_transport/http_transport.c \
                      $(SRC_DIR)/nano/transport/ws_transport/ws_transport.c \
                      $(SRC_DIR)/nano/transport/stdio_transport/stdio_transport.c

NANO_CORE_SRCS = $(SRC_DIR)/nano/core/nano/nano.c

# Library source files (without main functions)
LIB_SRCS = $(COMMON_SRCS) $(IO_CORE_SRCS) $(IO_MAPPING_SRCS) $(NANO_SYSTEM_SRCS) \
           $(NANO_VALIDATION_SRCS) $(NANO_TRANSPORT_SRCS) $(NANO_CORE_SRCS)

# Test source files
TEST_SRCS = $(TESTS_DIR)/common/test_json_utils.c \
            $(TESTS_DIR)/io/test_io/test_io.c \
            $(TESTS_DIR)/nano/test_validation/test_validation.c \
            $(TESTS_DIR)/nano/test_system/test_system.c \
            $(TESTS_DIR)/integration/test_qwenvl/test_qwenvl.c \
            $(TESTS_DIR)/integration/test_lora/test_lora.c

# Object files
LIB_OBJS = $(LIB_SRCS:$(SRC_DIR)/%.c=$(BUILD_DIR)/%.o)
TEST_OBJS = $(TEST_SRCS:$(TESTS_DIR)/%.c=$(BUILD_DIR)/tests/%.o)

# Include paths
INCLUDES = -I$(SRC_DIR) -I$(SRC_DIR)/libs/rkllm -I$(SRC_DIR)/common \
           -I$(SRC_DIR)/io/core -I$(SRC_DIR)/io/mapping \
           -I$(SRC_DIR)/nano/system -I$(SRC_DIR)/nano/validation \
           -I$(SRC_DIR)/nano/transport -I$(SRC_DIR)/nano/core

# Main targets
NANO_TARGET = nano
IO_TARGET = io
TEST_TARGET = test

.PHONY: all clean test run-test check-syntax help

all: $(NANO_TARGET) $(IO_TARGET) $(TEST_TARGET)

# Create build directory structure
$(BUILD_DIR):
	@mkdir -p $(BUILD_DIR)
	@mkdir -p $(BUILD_DIR)/common/json_utils
	@mkdir -p $(BUILD_DIR)/common/memory_utils
	@mkdir -p $(BUILD_DIR)/common/string_utils
	@mkdir -p $(BUILD_DIR)/io/core/queue
	@mkdir -p $(BUILD_DIR)/io/core/worker_pool
	@mkdir -p $(BUILD_DIR)/io/core/io
	@mkdir -p $(BUILD_DIR)/io/mapping/handle_pool
	@mkdir -p $(BUILD_DIR)/io/mapping/rkllm_proxy
	@mkdir -p $(BUILD_DIR)/nano/system/system_info
	@mkdir -p $(BUILD_DIR)/nano/system/resource_mgr
	@mkdir -p $(BUILD_DIR)/nano/validation/model_checker
	@mkdir -p $(BUILD_DIR)/nano/transport/mcp_base
	@mkdir -p $(BUILD_DIR)/nano/transport/udp_transport
	@mkdir -p $(BUILD_DIR)/nano/transport/tcp_transport
	@mkdir -p $(BUILD_DIR)/nano/transport/http_transport
	@mkdir -p $(BUILD_DIR)/nano/transport/ws_transport
	@mkdir -p $(BUILD_DIR)/nano/transport/stdio_transport
	@mkdir -p $(BUILD_DIR)/nano/core/nano
	@mkdir -p $(BUILD_DIR)/tests
	@mkdir -p $(BUILD_DIR)/tests/common
	@mkdir -p $(BUILD_DIR)/tests/io/test_io
	@mkdir -p $(BUILD_DIR)/tests/nano/test_validation
	@mkdir -p $(BUILD_DIR)/tests/nano/test_system
	@mkdir -p $(BUILD_DIR)/tests/integration/test_qwenvl
	@mkdir -p $(BUILD_DIR)/tests/integration/test_lora

# Build nano target
$(NANO_TARGET): $(BUILD_DIR) $(LIB_OBJS) $(BUILD_DIR)/main.o
	$(CC) $(LIB_OBJS) $(BUILD_DIR)/main.o $(LDFLAGS) -o $@
	@echo "✅ Built $(NANO_TARGET) successfully"

# Build io target (same as nano but with different entry point)
$(IO_TARGET): $(BUILD_DIR) $(LIB_OBJS) $(BUILD_DIR)/main.o
	$(CC) $(LIB_OBJS) $(BUILD_DIR)/main.o $(LDFLAGS) -o $@
	@echo "✅ Built $(IO_TARGET) successfully"

# Build test target
$(TEST_TARGET): $(BUILD_DIR) $(LIB_OBJS) $(TEST_OBJS) $(BUILD_DIR)/tests/test.o
	$(CC) $(LIB_OBJS) $(TEST_OBJS) $(BUILD_DIR)/tests/test.o $(LDFLAGS) -o $@
	@echo "✅ Built $(TEST_TARGET) successfully"

# Compile main.c
$(BUILD_DIR)/main.o: $(SRC_DIR)/main.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# Compile test.c
$(BUILD_DIR)/tests/test.o: $(TESTS_DIR)/test.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# Compile library object files
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# Compile test object files  
$(BUILD_DIR)/tests/%.o: $(TESTS_DIR)/%.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# Test targets - build test executable
test: $(TEST_TARGET)

# Run tests - build and execute
run-test: $(TEST_TARGET)
	@echo "🧪 Running test suite..."
	./$(TEST_TARGET)

# Syntax check
check-syntax:
	@echo "🔍 Checking syntax..."
	@for file in $(LIB_SRCS) $(SRC_DIR)/main.c $(TESTS_DIR)/test.c; do \
		if [ -f "$$file" ]; then \
			echo "Checking $$file..."; \
			$(CC) $(CFLAGS) $(INCLUDES) -fsyntax-only "$$file" || exit 1; \
		fi; \
	done
	@echo "✅ All syntax checks passed"

# Clean build artifacts
clean:
	rm -rf $(BUILD_DIR) $(NANO_TARGET) $(IO_TARGET) $(TEST_TARGET)
	@echo "🧹 Cleaned build artifacts"

# Help
help:
	@echo "🔧 NANO PROJECT BUILD SYSTEM"
	@echo "============================"
	@echo "Available targets:"
	@echo "  all          - Build nano, io, and test executables"
	@echo "  nano         - Build nano executable"
	@echo "  io           - Build io executable"
	@echo "  test         - Build test executable"
	@echo "  run-test     - Build and run test suite"
	@echo "  check-syntax - Check syntax of all source files"
	@echo "  clean        - Clean all build artifacts"
	@echo "  help         - Show this help"
