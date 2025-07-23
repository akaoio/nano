# Auto-generated Makefile - delegates to build/ directory
# This is the ONLY Makefile - build/ contains CMake-generated files

.PHONY: all build clean install setup help

# Default target
all: setup build

# Ensure build directory exists and is configured
setup:
	@if [ ! -d "build" ]; then \
		echo "Creating build directory..."; \
		mkdir -p build; \
	fi
	@if [ ! -f "build/Makefile" ]; then \
		echo "Configuring CMake build system..."; \
		cd build && cmake ..; \
	fi

# Build the project
build: setup
	@echo "Building MCP Server..."
	@cd build && $(MAKE) $(MAKEFLAGS)

# Clean everything
clean:
	@echo "Cleaning build directory..."
	@rm -rf build/
	@echo "Build directory removed. Run 'make' to rebuild."

# Install the project
install: setup build
	@echo "Installing MCP Server..."
	@cd build && $(MAKE) install

# Show available targets
help:
	@echo "RKLLM MCP Server - Available Make Targets:"
	@echo ""
	@echo "  make         - Build the project (default)"
	@echo "  make clean   - Remove build directory"
	@echo "  make install - Install the project"
	@echo "  make help    - Show this help"
	@echo ""
	@echo "Build files are contained in build/ directory."
	@echo ""
	@echo "Quick start:"
	@echo "  make         # Build everything"
	@echo "  ./build/mcp_server # Run the server"
