# CMakeLists.txt for MCP Server project - Modern C build system
cmake_minimum_required(VERSION 3.20)
project(MCP_SERVER VERSION 1.0.0 LANGUAGES C)

# =============================================================================
# ROOT MAKEFILE GENERATION (Simple build-only approach)
# =============================================================================

# Only generate root Makefile if we're in the source directory build
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    set(ROOT_MAKEFILE "${CMAKE_SOURCE_DIR}/Makefile")
    
    # Generate the single root Makefile that delegates to build directory
    file(WRITE ${ROOT_MAKEFILE}
"# Auto-generated Makefile - delegates to build/ directory
# This is the ONLY Makefile - build/ contains CMake-generated files

.PHONY: all build clean install setup help

# Default target
all: setup build

# Ensure build directory exists and is configured
setup:
\t@if [ ! -d \"build\" ]; then \\
\t\techo \"Creating build directory...\"; \\
\t\tmkdir -p build; \\
\tfi
\t@if [ ! -f \"build/Makefile\" ]; then \\
\t\techo \"Configuring CMake build system...\"; \\
\t\tcd build && cmake ..; \\
\tfi

# Build the project
build: setup
\t@echo \"Building MCP Server...\"
\t@cd build && \$(MAKE) \$(MAKEFLAGS)

# Clean everything
clean:
\t@echo \"Cleaning build directory...\"
\t@rm -rf build/
\t@echo \"Build directory removed. Run 'make' to rebuild.\"

# Install the project
install: setup build
\t@echo \"Installing MCP Server...\"
\t@cd build && \$(MAKE) install

# Show available targets
help:
\t@echo \"RKLLM MCP Server - Available Make Targets:\"
\t@echo \"\"
\t@echo \"  make         - Build the project (default)\"
\t@echo \"  make clean   - Remove build directory\"
\t@echo \"  make install - Install the project\"
\t@echo \"  make help    - Show this help\"
\t@echo \"\"
\t@echo \"Build files are contained in build/ directory.\"
\t@echo \"\"
\t@echo \"Quick start:\"
\t@echo \"  make         # Build everything\"
\t@echo \"  ./build/mcp_server # Run the server\"
"
    )
    
    message(STATUS "✅ Generated single root Makefile: ${ROOT_MAKEFILE}")
    message(STATUS "📁 Build files will be in: build/")
endif()

# =============================================================================
# BUILD CONFIGURATION
# =============================================================================

# Set C23 standard and use clang as default compiler
set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

# Set default compiler to clang if not specified
if(NOT CMAKE_C_COMPILER_ID STREQUAL "Clang")
    set(CMAKE_C_COMPILER clang)
endif()

# Build configuration
set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type")
set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "RelWithDebInfo" "MinSizeRel")

# Compiler flags
set(CMAKE_C_FLAGS "-Wall -Wextra -g")
set(CMAKE_C_FLAGS_DEBUG "-O0 -g3 -DDEBUG")
set(CMAKE_C_FLAGS_RELEASE "-O3 -DNDEBUG")

# Find required packages
find_package(PkgConfig REQUIRED)
find_package(Threads REQUIRED)

# Find json-c library
pkg_check_modules(JSON_C REQUIRED json-c)

# Include directories
include_directories(
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/external/rkllm
    ${CMAKE_SOURCE_DIR}/src/common
    ${CMAKE_SOURCE_DIR}/src/include
    ${CMAKE_SOURCE_DIR}/src/lib
    ${JSON_C_INCLUDE_DIRS}
)

# =============================================================================
# EXTERNAL DEPENDENCIES
# =============================================================================

# Download RKLLM library and header files if they don't exist
set(RKLLM_LIB_PATH "${CMAKE_SOURCE_DIR}/src/external/rkllm/librkllmrt.so")
set(RKLLM_HEADER_PATH "${CMAKE_SOURCE_DIR}/src/external/rkllm/rkllm.h")

# Create rkllm directory if it doesn't exist
file(MAKE_DIRECTORY "${CMAKE_SOURCE_DIR}/src/external/rkllm")

# Download librkllmrt.so if it doesn't exist
if(NOT EXISTS ${RKLLM_LIB_PATH})
    message(STATUS "Downloading librkllmrt.so for aarch64...")
    file(DOWNLOAD 
        "https://github.com/airockchip/rknn-llm/raw/main/rkllm-runtime/Linux/librkllm_api/aarch64/librkllmrt.so"
        ${RKLLM_LIB_PATH}
        SHOW_PROGRESS
        STATUS download_status
    )
    list(GET download_status 0 status_code)
    if(NOT status_code EQUAL 0)
        message(FATAL_ERROR "Failed to download librkllmrt.so")
    endif()
    message(STATUS "Successfully downloaded librkllmrt.so")
endif()

# Download rkllm.h if it doesn't exist
if(NOT EXISTS ${RKLLM_HEADER_PATH})
    message(STATUS "Downloading rkllm.h header file...")
    file(DOWNLOAD 
        "https://github.com/airockchip/rknn-llm/raw/main/rkllm-runtime/Linux/librkllm_api/include/rkllm.h"
        ${RKLLM_HEADER_PATH}
        SHOW_PROGRESS
        STATUS download_status
    )
    list(GET download_status 0 status_code)
    if(NOT status_code EQUAL 0)
        message(FATAL_ERROR "Failed to download rkllm.h")
    endif()
    message(STATUS "Successfully downloaded rkllm.h")
endif()

# Link directories
link_directories(
    ${CMAKE_SOURCE_DIR}/src/external/rkllm
    ${JSON_C_LIBRARY_DIRS}
)

# =============================================================================
# SOURCE FILES
# =============================================================================

# Source files
set(COMMON_SRCS
    src/common/memory_utils/memory_utils.c
    src/common/string_utils/string_utils.c
    src/common/error_utils/error_utils.c
    src/common/handle_utils/handle_utils.c
    src/common/transport_utils/transport_utils.c
    src/common/time_utils/time_utils.c
)

set(SERVER_CORE_SRCS
    src/lib/core/server.c
    src/lib/core/operations.c
    src/lib/core/rkllm_proxy.c
    src/lib/core/stream_manager.c
    src/lib/core/public_api.c
    src/lib/core/rkllm_array_utils.c
    src/lib/core/rkllm_error_mapping.c
    src/lib/core/rkllm_streaming_context.c
    src/lib/core/process_manager.c
    src/lib/core/settings.c
    src/lib/core/settings_global.c
    src/lib/core/npu_operation_classifier.c
    src/lib/core/npu_queue.c
    src/lib/core/npu_worker.c
    src/lib/core/async_response.c
    src/lib/core/error_mapping.c
    src/lib/core/memory_tracker.c
    src/lib/system/logger.c
    src/lib/system/metrics.c
    src/lib/system/performance.c
)

set(SERVER_SYSTEM_SRCS
    src/lib/system/info.c
    src/lib/system/memory.c
    src/lib/system/resources.c
)

set(SERVER_PROTOCOL_SRCS
    src/lib/protocol/mcp_protocol.c
    src/lib/protocol/jsonrpc.c
    src/lib/protocol/adapter.c
    src/lib/protocol/http_buffer_manager.c
)

# All library sources
set(LIB_SRCS
    ${COMMON_SRCS}
    ${SERVER_CORE_SRCS}
    ${SERVER_SYSTEM_SRCS}
    ${SERVER_PROTOCOL_SRCS}
)

# =============================================================================
# BUILD TARGETS
# =============================================================================

# Create main library
add_library(mcp_server_lib STATIC ${LIB_SRCS})
target_link_libraries(mcp_server_lib 
    ${CMAKE_THREAD_LIBS_INIT}
    ${CMAKE_DL_LIBS}
    rkllmrt
    ${JSON_C_LIBRARIES}
)
target_compile_options(mcp_server_lib PRIVATE ${JSON_C_CFLAGS_OTHER})

# Set RPATH for runtime library search
set_target_properties(mcp_server_lib PROPERTIES
    LINK_FLAGS "-Wl,-rpath,${CMAKE_SOURCE_DIR}/src/external/rkllm"
)

# Main executable
add_executable(mcp_server src/main.c)
target_link_libraries(mcp_server mcp_server_lib)
set_target_properties(mcp_server PROPERTIES
    LINK_FLAGS "-Wl,-rpath,${CMAKE_SOURCE_DIR}/src/external/rkllm"
)

# =============================================================================
# INSTALL TARGETS
# =============================================================================

install(TARGETS mcp_server
    RUNTIME DESTINATION bin
)

install(TARGETS mcp_server_lib
    ARCHIVE DESTINATION lib
)

# =============================================================================
# CONFIGURATION INFO
# =============================================================================

# Print configuration info
message(STATUS "MCP Server Project Configuration:")
message(STATUS "  Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  C Compiler: ${CMAKE_C_COMPILER}")
message(STATUS "  C Standard: C${CMAKE_C_STANDARD}")
message(STATUS "  JSON-C found: ${JSON_C_FOUND}")
message(STATUS "  JSON-C version: ${JSON_C_VERSION}")
message(STATUS "  JSON-C libraries: ${JSON_C_LIBRARIES}")
message(STATUS "  JSON-C include dirs: ${JSON_C_INCLUDE_DIRS}")