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

# Output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

# =============================================================================
# RKLLM LIBRARY DOWNLOAD
# =============================================================================

# RKLLM configuration
set(RKLLM_REPO_URL "https://github.com/airockchip/rknn-llm.git")
set(RKLLM_DIR "${CMAKE_SOURCE_DIR}/external/rknn-llm")
set(RKLLM_RUNTIME_DIR "${RKLLM_DIR}/rkllm-runtime/Linux/librkllm_api")
set(RKLLM_LIB_PATH "${RKLLM_RUNTIME_DIR}/aarch64/librkllmrt.so")
set(RKLLM_HEADER_PATH "${RKLLM_RUNTIME_DIR}/include/rkllm.h")

# Check if RKLLM is already downloaded
if(NOT EXISTS ${RKLLM_DIR})
    message(STATUS "RKLLM not found. Downloading from ${RKLLM_REPO_URL}...")
    execute_process(
        COMMAND git clone --depth 1 ${RKLLM_REPO_URL} ${RKLLM_DIR}
        RESULT_VARIABLE GIT_RESULT
        OUTPUT_VARIABLE GIT_OUTPUT
        ERROR_VARIABLE GIT_ERROR
    )
    if(NOT GIT_RESULT EQUAL 0)
        message(FATAL_ERROR "Failed to clone RKLLM repository: ${GIT_ERROR}")
    endif()
else()
    message(STATUS "RKLLM already exists at ${RKLLM_DIR}")
endif()

# Verify RKLLM files exist
if(NOT EXISTS ${RKLLM_LIB_PATH})
    message(FATAL_ERROR "RKLLM library not found at ${RKLLM_LIB_PATH}")
endif()

if(NOT EXISTS ${RKLLM_HEADER_PATH})
    message(FATAL_ERROR "RKLLM header not found at ${RKLLM_HEADER_PATH}")
endif()

# =============================================================================
# RKNN LIBRARY DOWNLOAD
# =============================================================================

# RKNN configuration
set(RKNN_REPO_URL "https://github.com/airockchip/rknn-toolkit2.git")
set(RKNN_DIR "${CMAKE_SOURCE_DIR}/external/rknn-toolkit2")
set(RKNN_RUNTIME_DIR "${RKNN_DIR}/rknpu2/runtime/Linux/librknn_api")
set(RKNN_LIB_PATH "${RKNN_RUNTIME_DIR}/aarch64/librknnrt.so")
set(RKNN_HEADER_PATH "${RKNN_RUNTIME_DIR}/include/rknn_api.h")

# Check if RKNN is already downloaded
if(NOT EXISTS ${RKNN_DIR})
    message(STATUS "RKNN not found. Downloading from ${RKNN_REPO_URL}...")
    execute_process(
        COMMAND git clone --depth 1 ${RKNN_REPO_URL} ${RKNN_DIR}
        RESULT_VARIABLE GIT_RESULT
        OUTPUT_VARIABLE GIT_OUTPUT
        ERROR_VARIABLE GIT_ERROR
    )
    if(NOT GIT_RESULT EQUAL 0)
        message(FATAL_ERROR "Failed to clone RKNN repository: ${GIT_ERROR}")
    endif()
else()
    message(STATUS "RKNN already exists at ${RKNN_DIR}")
endif()

# Verify RKNN files exist
if(NOT EXISTS ${RKNN_LIB_PATH})
    message(FATAL_ERROR "RKNN library not found at ${RKNN_LIB_PATH}")
endif()

if(NOT EXISTS ${RKNN_HEADER_PATH})
    message(FATAL_ERROR "RKNN header not found at ${RKNN_HEADER_PATH}")
endif()

# =============================================================================
# DEPENDENCIES
# =============================================================================

# Find required packages
find_package(PkgConfig REQUIRED)
find_package(Threads REQUIRED)

# Find json-c library
pkg_check_modules(JSON_C REQUIRED json-c)

# =============================================================================
# SOURCE FILES & INCLUDES
# =============================================================================

# Include directories
include_directories(
    ${CMAKE_SOURCE_DIR}/src
    ${RKLLM_RUNTIME_DIR}/include
    ${RKNN_RUNTIME_DIR}/include
    ${JSON_C_INCLUDE_DIRS}
)

# Link directories
link_directories(
    ${RKLLM_RUNTIME_DIR}/aarch64
    ${RKNN_RUNTIME_DIR}/aarch64
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
add_executable(server ${SERVER_SOURCES})

# Link libraries
target_link_libraries(server
    ${CMAKE_THREAD_LIBS_INIT}
    rkllmrt
    rknnrt
    ${JSON_C_LIBRARIES}
)

# Compiler options
target_compile_options(server PRIVATE ${JSON_C_CFLAGS_OTHER})

# Set RPATH for RKLLM and RKNN libraries
set_target_properties(server PROPERTIES
    INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib:${RKLLM_RUNTIME_DIR}/aarch64:${RKNN_RUNTIME_DIR}/aarch64"
    BUILD_WITH_INSTALL_RPATH TRUE
)

# =============================================================================
# POST-BUILD COMMANDS
# =============================================================================

# Server executable stays in build directory - no copying needed

# Copy RKLLM library to build directory
add_custom_command(TARGET server POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy ${RKLLM_LIB_PATH} ${CMAKE_BINARY_DIR}/
    COMMENT "Copying RKLLM library to build directory"
)

# Copy RKNN library to build directory
add_custom_command(TARGET server POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy ${RKNN_LIB_PATH} ${CMAKE_BINARY_DIR}/
    COMMENT "Copying RKNN library to build directory"
)

# =============================================================================
# GENERATE BUILD SCRIPT
# =============================================================================

# Create scripts directory
file(MAKE_DIRECTORY ${CMAKE_SOURCE_DIR}/scripts)

# Generate build.sh script
file(WRITE ${CMAKE_SOURCE_DIR}/scripts/build.sh
"#!/bin/bash
# Auto-generated build script for RKLLM Unix Domain Socket Server

set -e

# Colors for output
RED='\\033[0;31m'
GREEN='\\033[0;32m'
YELLOW='\\033[1;33m'
NC='\\033[0m' # No Color

echo -e \"${GREEN}=== RKLLM Unix Domain Socket Server Build Script ===${NC}\"
echo

# Function to check if command exists
command_exists() {
    command -v \"$1\" >/dev/null 2>&1
}

# Detect system
echo -e \"${YELLOW}Detecting system...${NC}\"
if [[ \"$OSTYPE\" == \"linux-android\"* ]] || [[ -n \"$TERMUX_VERSION\" ]]; then
    echo \"Detected Termux (Android Linux environment)\"
    SYSTEM=\"termux\"
elif [[ \"$OSTYPE\" == \"linux-gnu\"* ]]; then
    DISTRO=$(lsb_release -si 2>/dev/null || echo \"Unknown\")
    echo \"Linux distribution: $DISTRO\"
    SYSTEM=\"linux\"
else
    echo -e \"${RED}Unsupported system: $OSTYPE${NC}\"
    exit 1
fi

# Install essential packages based on system
echo -e \"${YELLOW}Installing essential development packages...${NC}\"
if [[ \"$SYSTEM\" == \"termux\" ]]; then
    # Termux package installation
    echo \"Installing packages for Termux...\"
    if command_exists pkg; then
        pkg update -y
        pkg install -y \\
            build-essential \\
            cmake \\
            pkg-config \\
            git \\
            wget \\
            curl \\
            libjansson \\
            python \\
            clang \\
            make
    else
        echo -e \"${RED}pkg command not found. Please install packages manually.${NC}\"
    fi
    
    # Check if json-c is available, fallback to libjansson
    if ! pkg list-installed | grep -q json-c 2>/dev/null; then
        echo \"Note: Using libjansson instead of json-c in Termux\"
    fi
    
elif [[ \"$SYSTEM\" == \"linux\" ]]; then
    # Standard Linux package installation
    if command_exists apt-get; then
        apt-get update
        apt-get install -y \\
            build-essential \\
            cmake \\
            pkg-config \\
            git \\
            wget \\
            curl \\
            libjson-c-dev \\
            python3-pip \\
            clang
    elif command_exists yum; then
        yum install -y \\
            gcc \\
            gcc-c++ \\
            cmake \\
            pkgconfig \\
            git \\
            wget \\
            curl \\
            json-c-devel \\
            python3-pip \\
            clang
    elif command_exists pacman; then
        pacman -Sy --noconfirm \\
            base-devel \\
            cmake \\
            pkgconf \\
            git \\
            wget \\
            curl \\
            json-c \\
            python-pip \\
            clang
    else
        echo -e \"${RED}No supported package manager found. Please install packages manually.${NC}\"
    fi
fi

# Display versions
echo \"System information:\"
if command_exists clang; then
    echo \"Clang version: $(clang --version | head -1)\"
fi
if command_exists cmake; then
    echo \"CMake version: $(cmake --version | head -1)\"
fi
if command_exists git; then
    echo \"Git version: $(git --version)\"
fi
if command_exists python3; then
    echo \"Python version: $(python3 --version)\"
elif command_exists python; then
    echo \"Python version: $(python --version)\"
fi

# Create build directory
echo -e \"${YELLOW}Creating build directory...${NC}\"
mkdir -p build
cd build

# Configure with CMake
echo -e \"${YELLOW}Configuring with CMake...${NC}\"
cmake .. -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}

# Build
echo -e \"${YELLOW}Building...${NC}\"
if command_exists nproc; then
    make -j$(nproc)
else
    make -j4  # Fallback for systems without nproc
fi

# Check if build was successful
if [ -f ./server ]; then
    echo -e \"${GREEN}Build successful! Server executable is in the build directory.${NC}\"
    echo -e \"${GREEN}To run the server: LD_LIBRARY_PATH=. ./server${NC}\"
else
    echo -e \"${RED}Build failed!${NC}\"
    exit 1
fi
")

# Make build.sh executable
file(CHMOD ${CMAKE_SOURCE_DIR}/scripts/build.sh
    FILE_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE 
                    GROUP_READ GROUP_EXECUTE 
                    WORLD_READ WORLD_EXECUTE
)


# =============================================================================
# DEVELOPMENT TARGETS
# =============================================================================

# Run target
add_custom_target(run
    COMMAND LD_LIBRARY_PATH=${CMAKE_BINARY_DIR} ${CMAKE_BINARY_DIR}/server
    DEPENDS server
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Running RKLLM UDS server"
)

# =============================================================================
# CONFIGURATION SUMMARY
# =============================================================================

message(STATUS "RKLLM Unix Domain Socket Server Configuration:")
message(STATUS "  Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  C Standard: C${CMAKE_C_STANDARD}")
message(STATUS "  RKLLM repository: ${RKLLM_REPO_URL}")
message(STATUS "  RKLLM library: ${RKLLM_LIB_PATH}")
message(STATUS "  RKLLM header: ${RKLLM_HEADER_PATH}")
message(STATUS "  RKNN repository: ${RKNN_REPO_URL}")
message(STATUS "  RKNN library: ${RKNN_LIB_PATH}")
message(STATUS "  RKNN header: ${RKNN_HEADER_PATH}")
message(STATUS "  json-c version: ${JSON_C_VERSION}")
message(STATUS "  Build directory: ${CMAKE_BINARY_DIR}")
message(STATUS "  Output executable: ${CMAKE_BINARY_DIR}/server")
message(STATUS "")
message(STATUS "Build script generated at: ${CMAKE_SOURCE_DIR}/scripts/build.sh")
