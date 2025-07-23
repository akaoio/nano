#!/bin/bash
# Auto-generated build script for RKLLM Unix Domain Socket Server

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo -e "=== RKLLM Unix Domain Socket Server Build Script ==="
echo

# Function to check if command exists
command_exists() {
    command -v "$1" >/dev/null 2>&1
}

# Function to get Ubuntu version
get_ubuntu_version() {
    if [ -f /etc/os-release ]; then
        . /etc/os-release
        echo "$VERSION_ID"
    else
        echo "unknown"
    fi
}

# Detect system
echo -e "Detecting system..."
if [[ "$OSTYPE" == "linux-gnu"* ]]; then
    DISTRO=$(lsb_release -si 2>/dev/null || echo "Unknown")
    echo "Linux distribution: $DISTRO"
else
    echo -e "This script is designed for Linux systems only"
    exit 1
fi

# Install essential packages
echo -e "Installing essential development packages..."
if command_exists apt-get; then
    sudo apt-get update
    sudo apt-get install -y \
        build-essential \
        cmake \
        pkg-config \
        git \
        wget \
        curl \
        libjson-c-dev \
        python3-pip \
        software-properties-common \
        gnupg \
        lsb-release
else
    echo -e "apt-get not found. Please install packages manually."
fi

# Install LLVM and Clang (latest version)
echo -e "Installing LLVM and Clang..."
if command_exists apt-get; then
    UBUNTU_VERSION=$(get_ubuntu_version)
    LLVM_VERSION=18
    
    # Add LLVM repository
    wget -O - https://apt.llvm.org/llvm-snapshot.gpg.key | sudo apt-key add -
    sudo add-apt-repository -y "deb http://apt.llvm.org/$(lsb_release -sc)/ llvm-toolchain-$(lsb_release -sc)- main"
    sudo apt-get update
    
    # Install LLVM and Clang
    sudo apt-get install -y \
        llvm- \
        clang- \
        clang-tools- \
        libc++--dev \
        libc++abi--dev
    
    # Set as default
    sudo update-alternatives --install /usr/bin/clang clang /usr/bin/clang- 100
    sudo update-alternatives --install /usr/bin/clang++ clang++ /usr/bin/clang++- 100
fi

# Install Node.js (latest LTS)
echo -e "Installing Node.js..."
if ! command_exists node; then
    curl -fsSL https://deb.nodesource.com/setup_lts.x | sudo -E bash -
    sudo apt-get install -y nodejs
fi
echo "Node.js version: $(node --version)"
echo "npm version: $(npm --version)"

# Install Python (latest version)
echo -e "Installing Python..."
if command_exists apt-get; then
    sudo add-apt-repository -y ppa:deadsnakes/ppa
    sudo apt-get update
    sudo apt-get install -y python3.12 python3.12-venv python3.12-dev
    
    # Set Python 3.12 as default python3
    sudo update-alternatives --install /usr/bin/python3 python3 /usr/bin/python3.12 100
fi
echo "Python version: $(python3 --version)"

# Create build directory
echo -e "Creating build directory..."
mkdir -p build
cd build

# Configure with CMake
echo -e "Configuring with CMake..."
cmake .. -DCMAKE_BUILD_TYPE=Release

# Build
echo -e "Building..."
make -j$(nproc)

# Check if build was successful
if [ -f ../server ]; then
    echo -e "Build successful! Server executable is in the root directory."
    echo -e "To run the server: ./server"
else
    echo -e "Build failed!"
    exit 1
fi
