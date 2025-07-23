# Auto-generated Makefile for RKLLM Unix Domain Socket Server

.PHONY: all clean build run help

# Default target
all: build

# Build the project
build:
	@mkdir -p build
	@cd build && cmake .. && make -j$$(nproc)

# Clean build artifacts
clean:
	@rm -rf build/
	@rm -f server
	@echo "Build artifacts cleaned."

# Run the server
run: build
	@if [ -f ./server ]; then \
		LD_LIBRARY_PATH=./build:$$LD_LIBRARY_PATH ./server; \
	else \
		echo "Server not found. Please build first."; \
		exit 1; \
	fi

# Display help
help:
	@echo "RKLLM Unix Domain Socket Server Makefile"
	@echo ""
	@echo "Available targets:"
	@echo "  all     - Build the project (default)"
	@echo "  build   - Build the project"
	@echo "  clean   - Remove build artifacts"
	@echo "  run     - Build and run the server"
	@echo "  help    - Display this help message"
	@echo ""
	@echo "Usage:"
	@echo "  make          # Build the project"
	@echo "  make clean    # Clean build artifacts"
	@echo "  make run      # Build and run the server"
