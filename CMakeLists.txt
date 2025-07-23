# RKLLM Unix Domain Socket Server - Clean Implementation
cmake_minimum_required(VERSION 3.16)
project(RKLLM_UDS_SERVER VERSION 1.0.0 LANGUAGES C)

# =============================================================================
# BUILD CONFIGURATION
# =============================================================================

# Set C99 standard (compatible with RKLLM)
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

# Create external directory
file(MAKE_DIRECTORY "${RKLLM_DIR}")

# Download RKLLM library if missing
if(NOT EXISTS ${RKLLM_LIB_PATH})
    message(STATUS "Downloading RKLLM library...")
    file(DOWNLOAD 
        "https://github.com/airockchip/rknn-llm/raw/main/rkllm-runtime/Linux/librkllm_api/aarch64/librkllmrt.so"
        ${RKLLM_LIB_PATH}
        SHOW_PROGRESS
        STATUS download_status
    )
    list(GET download_status 0 status_code)
    if(NOT status_code EQUAL 0)
        message(FATAL_ERROR "Failed to download RKLLM library")
    endif()
    message(STATUS "RKLLM library downloaded successfully")
endif()

# Download RKLLM header if missing
if(NOT EXISTS ${RKLLM_HEADER_PATH})
    message(STATUS "Downloading RKLLM header...")
    file(DOWNLOAD 
        "https://github.com/airockchip/rknn-llm/raw/main/rkllm-runtime/Linux/librkllm_api/include/rkllm.h"
        ${RKLLM_HEADER_PATH}
        SHOW_PROGRESS
        STATUS download_status
    )
    list(GET download_status 0 status_code)
    if(NOT status_code EQUAL 0)
        message(FATAL_ERROR "Failed to download RKLLM header")
    endif()
    message(STATUS "RKLLM header downloaded successfully")
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

# Auto-discover all source files following the rule: <name>/<name>.c
file(GLOB_RECURSE SERVER_SOURCES 
    "src/*/*.c"
    "src/main.c"  # Exception: main.c can be at root of src/
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
# INSTALL
# =============================================================================

install(TARGETS rkllm_uds_server
    RUNTIME DESTINATION bin
)

install(FILES ${RKLLM_LIB_PATH}
    DESTINATION lib
)

# =============================================================================
# DEVELOPMENT TARGETS (optional)
# =============================================================================

# Run target for development convenience
add_custom_target(run
    COMMAND LD_LIBRARY_PATH=${RKLLM_DIR} ./rkllm_uds_server
    DEPENDS rkllm_uds_server
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Running RKLLM UDS server"
)

# =============================================================================
# CONFIGURATION SUMMARY
# =============================================================================

message(STATUS "RKLLM Unix Domain Socket Server Configuration:")
message(STATUS "  Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  C Compiler: ${CMAKE_C_COMPILER}")
message(STATUS "  C Standard: C${CMAKE_C_STANDARD}")
message(STATUS "  RKLLM library: ${RKLLM_LIB_PATH}")
message(STATUS "  RKLLM header: ${RKLLM_HEADER_PATH}")
message(STATUS "  json-c found: ${JSON_C_FOUND}")
message(STATUS "  json-c version: ${JSON_C_VERSION}")
message(STATUS "  Install prefix: ${CMAKE_INSTALL_PREFIX}")
message(STATUS "")
message(STATUS "Build commands (from build/ directory):")
message(STATUS "  make                 - Build the server")
message(STATUS "  make run             - Build and run the server")
message(STATUS "  make install         - Install the server")