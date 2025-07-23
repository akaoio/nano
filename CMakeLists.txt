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
set(RKLLM_DIR "${CMAKE_SOURCE_DIR}/third_party/rknn-llm")
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
    ${JSON_C_INCLUDE_DIRS}
)

# Link directories
link_directories(
    ${RKLLM_RUNTIME_DIR}/aarch64
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
    ${JSON_C_LIBRARIES}
)

# Compiler options
target_compile_options(server PRIVATE ${JSON_C_CFLAGS_OTHER})

# Set RPATH for RKLLM library
set_target_properties(server PROPERTIES
    INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib:${RKLLM_RUNTIME_DIR}/aarch64"
    BUILD_WITH_INSTALL_RPATH TRUE
)

# =============================================================================
# POST-BUILD COMMANDS
# =============================================================================

# Copy server to root directory after build
add_custom_command(TARGET server POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:server> ${CMAKE_SOURCE_DIR}/server
    COMMENT "Copying server executable to root directory"
)

# Copy RKLLM library to build directory
add_custom_command(TARGET server POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy ${RKLLM_LIB_PATH} ${CMAKE_BINARY_DIR}/
    COMMENT "Copying RKLLM library to build directory"
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

# Function to get Ubuntu version
get_ubuntu_version() {
    if [ -f /etc/os-release ]; then
        . /etc/os-release
        echo \"$VERSION_ID\"
    else
        echo \"unknown\"
    fi
}

# Detect system
echo -e \"${YELLOW}Detecting system...${NC}\"
if [[ \"$OSTYPE\" == \"linux-gnu\"* ]]; then
    DISTRO=$(lsb_release -si 2>/dev/null || echo \"Unknown\")
    echo \"Linux distribution: $DISTRO\"
else
    echo -e \"${RED}This script is designed for Linux systems only${NC}\"
    exit 1
fi

# Install essential packages
echo -e \"${YELLOW}Installing essential development packages...${NC}\"
if command_exists apt-get; then
    sudo apt-get update
    sudo apt-get install -y \\
        build-essential \\
        cmake \\
        pkg-config \\
        git \\
        wget \\
        curl \\
        libjson-c-dev \\
        python3-pip \\
        software-properties-common \\
        gnupg \\
        lsb-release
else
    echo -e \"${RED}apt-get not found. Please install packages manually.${NC}\"
fi

# Install LLVM and Clang (latest version)
echo -e \"${YELLOW}Installing LLVM and Clang...${NC}\"
if command_exists apt-get; then
    UBUNTU_VERSION=$(get_ubuntu_version)
    LLVM_VERSION=18
    
    # Add LLVM repository
    wget -O - https://apt.llvm.org/llvm-snapshot.gpg.key | sudo apt-key add -
    sudo add-apt-repository -y \"deb http://apt.llvm.org/$(lsb_release -sc)/ llvm-toolchain-$(lsb_release -sc)-${LLVM_VERSION} main\"
    sudo apt-get update
    
    # Install LLVM and Clang
    sudo apt-get install -y \\
        llvm-${LLVM_VERSION} \\
        clang-${LLVM_VERSION} \\
        clang-tools-${LLVM_VERSION} \\
        libc++-${LLVM_VERSION}-dev \\
        libc++abi-${LLVM_VERSION}-dev
    
    # Set as default
    sudo update-alternatives --install /usr/bin/clang clang /usr/bin/clang-${LLVM_VERSION} 100
    sudo update-alternatives --install /usr/bin/clang++ clang++ /usr/bin/clang++-${LLVM_VERSION} 100
fi

# Install Node.js (latest LTS)
echo -e \"${YELLOW}Installing Node.js...${NC}\"
if ! command_exists node; then
    curl -fsSL https://deb.nodesource.com/setup_lts.x | sudo -E bash -
    sudo apt-get install -y nodejs
fi
echo \"Node.js version: $(node --version)\"
echo \"npm version: $(npm --version)\"

# Install Python (latest version)
echo -e \"${YELLOW}Installing Python...${NC}\"
if command_exists apt-get; then
    sudo add-apt-repository -y ppa:deadsnakes/ppa
    sudo apt-get update
    sudo apt-get install -y python3.12 python3.12-venv python3.12-dev
    
    # Set Python 3.12 as default python3
    sudo update-alternatives --install /usr/bin/python3 python3 /usr/bin/python3.12 100
fi
echo \"Python version: $(python3 --version)\"

# Create build directory
echo -e \"${YELLOW}Creating build directory...${NC}\"
mkdir -p build
cd build

# Configure with CMake
echo -e \"${YELLOW}Configuring with CMake...${NC}\"
cmake .. -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}

# Build
echo -e \"${YELLOW}Building...${NC}\"
make -j$(nproc)

# Check if build was successful
if [ -f ../server ]; then
    echo -e \"${GREEN}Build successful! Server executable is in the root directory.${NC}\"
    echo -e \"${GREEN}To run the server: ./server${NC}\"
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
# GENERATE ROOT MAKEFILE
# =============================================================================

# Generate Makefile in root directory
file(WRITE ${CMAKE_SOURCE_DIR}/Makefile
"# Auto-generated Makefile for RKLLM Unix Domain Socket Server

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
	@echo \"Build artifacts cleaned.\"

# Run the server
run: build
	@if [ -f ./server ]; then \\
		LD_LIBRARY_PATH=./build:$$LD_LIBRARY_PATH ./server; \\
	else \\
		echo \"Server not found. Please build first.\"; \\
		exit 1; \\
	fi

# Display help
help:
	@echo \"RKLLM Unix Domain Socket Server Makefile\"
	@echo \"\"
	@echo \"Available targets:\"
	@echo \"  all     - Build the project (default)\"
	@echo \"  build   - Build the project\"
	@echo \"  clean   - Remove build artifacts\"
	@echo \"  run     - Build and run the server\"
	@echo \"  help    - Display this help message\"
	@echo \"\"
	@echo \"Usage:\"
	@echo \"  make          # Build the project\"
	@echo \"  make clean    # Clean build artifacts\"
	@echo \"  make run      # Build and run the server\"
")

# =============================================================================
# DEVELOPMENT TARGETS
# =============================================================================

# Run target
add_custom_target(run
    COMMAND LD_LIBRARY_PATH=${CMAKE_BINARY_DIR} ./server
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
message(STATUS "  json-c version: ${JSON_C_VERSION}")
message(STATUS "  Build directory: ${CMAKE_BINARY_DIR}")
message(STATUS "  Output executable: ${CMAKE_SOURCE_DIR}/server")
message(STATUS "")
message(STATUS "Build script generated at: ${CMAKE_SOURCE_DIR}/scripts/build.sh")
message(STATUS "Root Makefile generated at: ${CMAKE_SOURCE_DIR}/Makefile")