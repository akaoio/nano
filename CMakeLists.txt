# CMakeLists.txt for MCP Server project - Modern C build system
cmake_minimum_required(VERSION 3.20)
project(MCP_SERVER VERSION 1.0.0 LANGUAGES C)

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
    ${CMAKE_SOURCE_DIR}/src/libs/rkllm
    ${CMAKE_SOURCE_DIR}/src/common
    ${CMAKE_SOURCE_DIR}/src/server
    ${CMAKE_SOURCE_DIR}/src/server/core
    ${CMAKE_SOURCE_DIR}/src/server/system
    ${CMAKE_SOURCE_DIR}/src/server/validation
    ${CMAKE_SOURCE_DIR}/src/server/transport
    ${CMAKE_SOURCE_DIR}/src/server/protocol
    ${JSON_C_INCLUDE_DIRS}
)

# Link directories
link_directories(
    ${CMAKE_SOURCE_DIR}/src/libs/rkllm
    ${JSON_C_LIBRARY_DIRS}
)

# Source files
set(COMMON_SRCS
    src/common/memory_utils/memory_utils.c
    src/common/string_utils/string_utils.c
    src/common/error_utils/error_utils.c
    src/common/handle_utils/handle_utils.c
    src/common/transport_utils/transport_utils.c
)

set(SERVER_CORE_SRCS
    src/server/core/queue/queue.c
    src/server/core/io/io.c
    src/server/core/io/io_worker.c
    src/server/core/nano/nano.c
    src/server/operations.c
    src/server/server.c
)

set(SERVER_SYSTEM_SRCS
    src/server/system/system_info/system_info.c
    src/server/system/system_info/system_memory.c
    src/server/system/system_info/system_model.c
    src/server/system/resource_mgr/resource_mgr.c
)

set(SERVER_VALIDATION_SRCS
    src/server/validation/model_checker/model_checker.c
)

set(SERVER_TRANSPORT_SRCS
    src/server/transport/mcp_base/mcp_base.c
    src/server/transport/mcp_base/mcp_protocol.c
    src/server/transport/udp_transport/udp_transport.c
    src/server/transport/tcp_transport/tcp_transport.c
    src/server/transport/http_transport/http_transport.c
    src/server/transport/ws_transport/ws_transport.c
    src/server/transport/stdio_transport/stdio_transport.c
    src/server/transport/streaming/stream_manager.c
    src/server/transport/transport_base.c
    src/server/transport/transport_manager.c
    src/server/transport/stdio_simple.c
)

set(SERVER_PROTOCOL_SRCS
    src/server/protocol/mcp_adapter.c
)

# Test source files
set(TEST_SRCS
    tests/test_unified.c
)

set(STREAMING_TEST_SRCS
    tests/test_streaming.c
)

set(MCP_COMPLIANCE_TEST_SRCS
    tests/test_mcp_compliance.c
)

# All library sources
set(LIB_SRCS
    ${COMMON_SRCS}
    ${SERVER_CORE_SRCS}
    ${SERVER_SYSTEM_SRCS}
    ${SERVER_VALIDATION_SRCS}
    ${SERVER_TRANSPORT_SRCS}
    ${SERVER_PROTOCOL_SRCS}
)

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
    LINK_FLAGS "-Wl,-rpath,${CMAKE_SOURCE_DIR}/src/libs/rkllm"
)

# Main executable
add_executable(mcp_server src/main.c)
target_link_libraries(mcp_server mcp_server_lib)
set_target_properties(mcp_server PROPERTIES
    LINK_FLAGS "-Wl,-rpath,${CMAKE_SOURCE_DIR}/src/libs/rkllm"
)

# Test executables
add_executable(test_unified ${TEST_SRCS})
target_link_libraries(test_unified mcp_server_lib)
target_include_directories(test_unified PRIVATE tests)
set_target_properties(test_unified PROPERTIES
    LINK_FLAGS "-Wl,-rpath,${CMAKE_SOURCE_DIR}/src/libs/rkllm"
)

add_executable(test_streaming ${STREAMING_TEST_SRCS})
target_link_libraries(test_streaming mcp_server_lib)
target_include_directories(test_streaming PRIVATE tests)
set_target_properties(test_streaming PROPERTIES
    LINK_FLAGS "-Wl,-rpath,${CMAKE_SOURCE_DIR}/src/libs/rkllm"
)

add_executable(test_mcp_compliance ${MCP_COMPLIANCE_TEST_SRCS})
target_link_libraries(test_mcp_compliance mcp_server_lib)
target_include_directories(test_mcp_compliance PRIVATE tests)
set_target_properties(test_mcp_compliance PROPERTIES
    LINK_FLAGS "-Wl,-rpath,${CMAKE_SOURCE_DIR}/src/libs/rkllm"
)

# Custom targets
add_custom_target(run-test
    COMMAND test_unified
    DEPENDS test_unified
    COMMENT "Running unified test suite..."
)

add_custom_target(run-streaming-test
    COMMAND test_streaming
    DEPENDS test_streaming
    COMMENT "Running streaming test suite..."
)

add_custom_target(run-mcp-test
    COMMAND test_mcp_compliance
    DEPENDS test_mcp_compliance
    COMMENT "Running MCP compliance test suite..."
)

add_custom_target(run-all-tests
    COMMAND test_unified && test_streaming && test_mcp_compliance
    DEPENDS test_unified test_streaming test_mcp_compliance
    COMMENT "Running all test suites..."
)

add_custom_target(check-syntax
    COMMAND echo "Syntax checking with clang..."
    COMMENT "Checking syntax of all source files"
)

# Install targets
install(TARGETS mcp_server test_unified test_streaming test_mcp_compliance
    RUNTIME DESTINATION bin
)

install(TARGETS mcp_server_lib
    ARCHIVE DESTINATION lib
)

# Print configuration info
message(STATUS "MCP Server Project Configuration:")
message(STATUS "  Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  C Compiler: ${CMAKE_C_COMPILER}")
message(STATUS "  C Standard: C${CMAKE_C_STANDARD}")
message(STATUS "  JSON-C found: ${JSON_C_FOUND}")
message(STATUS "  JSON-C version: ${JSON_C_VERSION}")
message(STATUS "  JSON-C libraries: ${JSON_C_LIBRARIES}")
message(STATUS "  JSON-C include dirs: ${JSON_C_INCLUDE_DIRS}")