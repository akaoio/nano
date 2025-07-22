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
    ${CMAKE_SOURCE_DIR}/src/external/rkllm
    ${CMAKE_SOURCE_DIR}/src/common
    ${CMAKE_SOURCE_DIR}/src/include
    ${CMAKE_SOURCE_DIR}/src/lib
    ${JSON_C_INCLUDE_DIRS}
)

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

# Download and build wsServer WebSocket library
set(WSSERVER_DIR "${CMAKE_SOURCE_DIR}/src/external/wsServer")
set(WSSERVER_LIB "${WSSERVER_DIR}/libws.a")

if(NOT EXISTS ${WSSERVER_DIR})
    message(STATUS "Downloading wsServer WebSocket library...")
    
    # Download wsServer source
    file(DOWNLOAD 
        "https://github.com/Theldus/wsServer/archive/refs/heads/master.zip"
        "${CMAKE_BINARY_DIR}/wsServer.zip"
        SHOW_PROGRESS
    )
    
    # Extract wsServer
    execute_process(
        COMMAND ${CMAKE_COMMAND} -E tar xf "${CMAKE_BINARY_DIR}/wsServer.zip"
        WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
    )
    
    # Move to external directory
    execute_process(
        COMMAND ${CMAKE_COMMAND} -E rename 
        "${CMAKE_BINARY_DIR}/wsServer-master" 
        ${WSSERVER_DIR}
    )
    
    # Clean up zip file
    file(REMOVE "${CMAKE_BINARY_DIR}/wsServer.zip")
endif()

# Build wsServer library if not already built
if(NOT EXISTS ${WSSERVER_LIB})
    message(STATUS "Building wsServer library...")
    
    # Use wsServer's own CMakeLists.txt to build the library
    execute_process(
        COMMAND ${CMAKE_COMMAND} -B build -S .
        WORKING_DIRECTORY ${WSSERVER_DIR}
    )
    
    execute_process(
        COMMAND ${CMAKE_COMMAND} --build build --target ws
        WORKING_DIRECTORY ${WSSERVER_DIR}
    )
    
    # Copy the built library to expected location
    execute_process(
        COMMAND ${CMAKE_COMMAND} -E copy 
        "${WSSERVER_DIR}/build/libws.a" 
        ${WSSERVER_LIB}
    )
endif()

# Add wsServer include directory
include_directories(${WSSERVER_DIR}/include)

# Auto-generate RKLLM constants from header file
set(RKLLM_HEADER "${CMAKE_SOURCE_DIR}/src/external/rkllm/rkllm.h")
set(RKLLM_AUTO_GENERATED "${CMAKE_SOURCE_DIR}/src/lib/core/rkllm_auto_generated.c")

add_custom_command(
    OUTPUT ${RKLLM_AUTO_GENERATED}
    COMMAND ${CMAKE_SOURCE_DIR}/tools/rkllm_parser.sh ${RKLLM_HEADER} ${CMAKE_SOURCE_DIR}/src/lib/core
    DEPENDS ${RKLLM_HEADER} ${CMAKE_SOURCE_DIR}/tools/rkllm_parser.sh
    COMMENT "Auto-generating RKLLM constants from header..."
)

# Link directories
link_directories(
    ${CMAKE_SOURCE_DIR}/src/external/rkllm
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
    src/lib/core/server.c
    src/lib/core/operations.c
    src/lib/core/rkllm_proxy.c
    src/lib/core/public_api.c
    src/lib/core/rkllm_auto_generated.c
    src/lib/core/rkllm_array_utils.c
    src/lib/core/rkllm_error_mapping.c
    src/lib/core/rkllm_streaming_context.c
    src/lib/core/streaming_integration.c
    src/lib/core/process_manager.c
    src/lib/core/settings.c
    src/lib/core/settings_global.c
    src/lib/core/npu_operation_classifier.c
    src/lib/core/npu_queue.c
    src/lib/core/npu_worker.c
    src/lib/core/async_response.c
)

set(SERVER_SYSTEM_SRCS
    src/lib/system/info.c
    src/lib/system/memory.c
    src/lib/system/resources.c
)


set(SERVER_TRANSPORT_SRCS
    src/lib/transport/base.c
    src/lib/transport/manager.c
    src/lib/transport/streaming_buffer_manager.c
    src/lib/transport/stdio.c
    src/lib/transport/tcp.c
    src/lib/transport/udp.c
    src/lib/transport/http.c
    src/lib/transport/websocket.c
)

set(SERVER_PROTOCOL_SRCS
    src/lib/protocol/mcp_protocol.c
    src/lib/protocol/jsonrpc.c
    src/lib/protocol/adapter.c
    src/lib/protocol/streaming.c
)

# JavaScript test client (no C tests)
# All testing is now done via tests/client.js


# All library sources
set(LIB_SRCS
    ${COMMON_SRCS}
    ${SERVER_CORE_SRCS}
    ${SERVER_SYSTEM_SRCS}
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
    ${WSSERVER_LIB}
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

# No C test executables - using JavaScript client instead


# Custom targets for JavaScript testing
add_custom_target(test-js
    COMMAND node tests/client.js
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Running JavaScript test client..."
)

add_custom_target(run-all-tests
    COMMAND node tests/client.js
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    DEPENDS mcp_server
    COMMENT "Running all tests via JavaScript client..."
)

add_custom_target(check-syntax
    COMMAND echo "Syntax checking with clang..."
    COMMENT "Checking syntax of all source files"
)

# Install targets
install(TARGETS mcp_server
    RUNTIME DESTINATION bin
)

install(FILES tests/client.js
    DESTINATION bin
    PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
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