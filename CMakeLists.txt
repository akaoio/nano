# CMakeLists.txt for NANO project - Modern C build system
cmake_minimum_required(VERSION 3.20)
project(NANO VERSION 1.0.0 LANGUAGES C)

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
    ${CMAKE_SOURCE_DIR}/src/io/core
    ${CMAKE_SOURCE_DIR}/src/io/mapping
    ${CMAKE_SOURCE_DIR}/src/nano/system
    ${CMAKE_SOURCE_DIR}/src/nano/validation
    ${CMAKE_SOURCE_DIR}/src/nano/transport
    ${CMAKE_SOURCE_DIR}/src/nano/core
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

set(IO_CORE_SRCS
    src/io/core/queue/queue.c
    src/io/core/worker_pool/worker_pool.c
    src/io/core/io/io.c
    src/io/core/io/io_worker.c
    src/io/operations.c
)

set(IO_MAPPING_SRCS
    src/io/mapping/handle_pool/handle_pool.c
    src/io/mapping/handle_pool/handle_pool_utils.c
    src/io/mapping/rkllm_proxy/rkllm_proxy.c
    src/io/mapping/rkllm_proxy/rkllm_operations.c
)

set(NANO_SYSTEM_SRCS
    src/nano/system/system_info/system_info.c
    src/nano/system/system_info/system_memory.c
    src/nano/system/system_info/system_model.c
    src/nano/system/resource_mgr/resource_mgr.c
)

set(NANO_VALIDATION_SRCS
    src/nano/validation/model_checker/model_checker.c
)

set(NANO_TRANSPORT_SRCS
    src/nano/transport/mcp_base/mcp_base.c
    src/nano/transport/udp_transport/udp_transport.c
    src/nano/transport/tcp_transport/tcp_transport.c
    src/nano/transport/http_transport/http_transport.c
    src/nano/transport/ws_transport/ws_transport.c
    src/nano/transport/stdio_transport/stdio_transport.c
)

set(NANO_CORE_SRCS
    src/nano/core/nano/nano.c
)

# Test sources - simplified for real model testing
set(IO_TEST_SRCS
    tests/io/test_io_architecture.c
)

set(NANO_TEST_SRCS
    tests/nano/test_nano_architecture.c
)

# All library sources
set(LIB_SRCS
    ${COMMON_SRCS}
    ${IO_CORE_SRCS}
    ${IO_MAPPING_SRCS}
    ${NANO_SYSTEM_SRCS}
    ${NANO_VALIDATION_SRCS}
    ${NANO_TRANSPORT_SRCS}
    ${NANO_CORE_SRCS}
)

# Create main library
add_library(nano_lib STATIC ${LIB_SRCS})
target_link_libraries(nano_lib 
    ${CMAKE_THREAD_LIBS_INIT}
    ${CMAKE_DL_LIBS}
    rkllmrt
    ${JSON_C_LIBRARIES}
)
target_compile_options(nano_lib PRIVATE ${JSON_C_CFLAGS_OTHER})

# Set RPATH for runtime library search
set_target_properties(nano_lib PROPERTIES
    LINK_FLAGS "-Wl,-rpath,${CMAKE_SOURCE_DIR}/src/libs/rkllm"
)

# Main executables
add_executable(nano src/main.c)
target_link_libraries(nano nano_lib)
set_target_properties(nano PROPERTIES
    LINK_FLAGS "-Wl,-rpath,${CMAKE_SOURCE_DIR}/src/libs/rkllm"
)

add_executable(io src/main.c)
target_link_libraries(io nano_lib)
set_target_properties(io PROPERTIES
    LINK_FLAGS "-Wl,-rpath,${CMAKE_SOURCE_DIR}/src/libs/rkllm"
)

# Test executable with new architecture
add_executable(test tests/test.c ${IO_TEST_SRCS} ${NANO_TEST_SRCS})
target_link_libraries(test nano_lib)
target_include_directories(test PRIVATE tests)
set_target_properties(test PROPERTIES
    LINK_FLAGS "-Wl,-rpath,${CMAKE_SOURCE_DIR}/src/libs/rkllm"
)

# Custom targets
add_custom_target(run-test
    COMMAND test
    DEPENDS test
    COMMENT "Running test suite..."
)

add_custom_target(check-syntax
    COMMAND echo "Syntax checking with clang..."
    COMMENT "Checking syntax of all source files"
)

# Install targets
install(TARGETS nano io test
    RUNTIME DESTINATION bin
)

install(TARGETS nano_lib
    ARCHIVE DESTINATION lib
)

# Print configuration info
message(STATUS "NANO Project Configuration:")
message(STATUS "  Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  C Compiler: ${CMAKE_C_COMPILER}")
message(STATUS "  C Standard: C${CMAKE_C_STANDARD}")
message(STATUS "  JSON-C found: ${JSON_C_FOUND}")
message(STATUS "  JSON-C version: ${JSON_C_VERSION}")
message(STATUS "  JSON-C libraries: ${JSON_C_LIBRARIES}")
message(STATUS "  JSON-C include dirs: ${JSON_C_INCLUDE_DIRS}")
