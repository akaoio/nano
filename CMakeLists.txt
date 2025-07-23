# RKLLM Unix Domain Socket Server
cmake_minimum_required(VERSION 3.16)
project(rkllm_uds_server VERSION 1.0.0 LANGUAGES C)

# =============================================================================
# BUILD CONFIGURATION
# =============================================================================

# Set C99 standard
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

# Build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Compiler flags
set(CMAKE_C_FLAGS "-Wall -Wextra -D_GNU_SOURCE")
set(CMAKE_C_FLAGS_DEBUG "-g -O0 -DDEBUG")
set(CMAKE_C_FLAGS_RELEASE "-O2 -DNDEBUG")

# =============================================================================
# DEPENDENCIES
# =============================================================================

# Find required packages
find_package(PkgConfig REQUIRED)
find_package(Threads REQUIRED)

# Find json-c library
pkg_check_modules(JSON_C REQUIRED json-c)

# =============================================================================
# RKLLM LIBRARY SETUP
# =============================================================================

# RKLLM paths
set(RKLLM_DIR "${CMAKE_SOURCE_DIR}/src/external/rkllm")
set(RKLLM_LIB_PATH "${RKLLM_DIR}/librkllmrt.so")
set(RKLLM_HEADER_PATH "${RKLLM_DIR}/rkllm.h")

# Ensure RKLLM files exist
if(NOT EXISTS ${RKLLM_LIB_PATH})
    message(FATAL_ERROR "RKLLM library not found at ${RKLLM_LIB_PATH}")
endif()

if(NOT EXISTS ${RKLLM_HEADER_PATH})
    message(FATAL_ERROR "RKLLM header not found at ${RKLLM_HEADER_PATH}")
endif()

# =============================================================================
# SOURCE FILES & INCLUDES
# =============================================================================

# Include directories
include_directories(
    ${CMAKE_SOURCE_DIR}/src
    ${RKLLM_DIR}
    ${JSON_C_INCLUDE_DIRS}
)

# Link directories
link_directories(
    ${RKLLM_DIR}
    ${JSON_C_LIBRARY_DIRS}
)

# Auto-discover all source files following rule: <name>/<name>.c
file(GLOB_RECURSE SERVER_SOURCES 
    "src/*/*.c"
    "src/main.c"
)

# =============================================================================
# BUILD TARGETS
# =============================================================================

# Main executable
add_executable(rkllm_uds_server ${SERVER_SOURCES})

# Link libraries
target_link_libraries(rkllm_uds_server
    ${CMAKE_THREAD_LIBS_INIT}
    rkllmrt
    ${JSON_C_LIBRARIES}
)

# Compiler options
target_compile_options(rkllm_uds_server PRIVATE ${JSON_C_CFLAGS_OTHER})

# Set RPATH for RKLLM library
set_target_properties(rkllm_uds_server PROPERTIES
    INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib:${RKLLM_DIR}"
    BUILD_WITH_INSTALL_RPATH TRUE
)

# =============================================================================
# DEVELOPMENT TARGETS
# =============================================================================

# Run target
add_custom_target(run
    COMMAND LD_LIBRARY_PATH=${RKLLM_DIR} ./rkllm_uds_server
    DEPENDS rkllm_uds_server
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Running RKLLM UDS server"
)

# =============================================================================
# CONFIGURATION SUMMARY
# =============================================================================

message(STATUS "RKLLM Unix Domain Socket Server:")
message(STATUS "  Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  C Standard: C${CMAKE_C_STANDARD}")
message(STATUS "  RKLLM library: ${RKLLM_LIB_PATH}")
message(STATUS "  RKLLM header: ${RKLLM_HEADER_PATH}")
message(STATUS "  json-c version: ${JSON_C_VERSION}")