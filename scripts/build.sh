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

# Detect system
echo -e "Detecting system..."
if [[ "$OSTYPE" == "linux-android"* ]] || [[ -n "$TERMUX_VERSION" ]]; then
    echo "Detected Termux (Android Linux environment)"
    SYSTEM="termux"
elif [[ "$OSTYPE" == "linux-gnu"* ]]; then
    DISTRO=$(lsb_release -si 2>/dev/null || echo "Unknown")
    echo "Linux distribution: $DISTRO"
    SYSTEM="linux"
else
    echo -e "Unsupported system: $OSTYPE"
    exit 1
fi

# Install essential packages based on system
echo -e "Installing essential development packages..."
if [[ "$SYSTEM" == "termux" ]]; then
    # Termux package installation
    echo "Installing packages for Termux..."
    if command_exists pkg; then
        pkg update -y
        pkg install -y \
            build-essential \
            cmake \
            pkg-config \
            git \
            wget \
            curl \
            libjansson \
            python \
            clang \
            make
    else
        echo -e "pkg command not found. Please install packages manually."
    fi
    
    # Check if json-c is available, fallback to libjansson
    if ! pkg list-installed | grep -q json-c 2>/dev/null; then
        echo "Note: Using libjansson instead of json-c in Termux"
    fi
    
elif [[ "$SYSTEM" == "linux" ]]; then
    # Standard Linux package installation
    if command_exists apt-get; then
        apt-get update
        apt-get install -y \
            build-essential \
            cmake \
            pkg-config \
            git \
            wget \
            curl \
            libjson-c-dev \
            python3-pip \
            clang
    elif command_exists yum; then
        yum install -y \
            gcc \
            gcc-c++ \
            cmake \
            pkgconfig \
            git \
            wget \
            curl \
            json-c-devel \
            python3-pip \
            clang
    elif command_exists pacman; then
        pacman -Sy --noconfirm \
            base-devel \
            cmake \
            pkgconf \
            git \
            wget \
            curl \
            json-c \
            python-pip \
            clang
    else
        echo -e "No supported package manager found. Please install packages manually."
    fi
fi

# Display versions
echo "System information:"
if command_exists clang; then
    echo "Clang version: $(clang --version | head -1)"
fi
if command_exists cmake; then
    echo "CMake version: $(cmake --version | head -1)"
fi
if command_exists git; then
    echo "Git version: $(git --version)"
fi
if command_exists python3; then
    echo "Python version: $(python3 --version)"
elif command_exists python; then
    echo "Python version: $(python --version)"
fi

# Create build directory
echo -e "Creating build directory..."
mkdir -p build
cd build

# Configure with CMake
echo -e "Configuring with CMake..."
cmake .. -DCMAKE_BUILD_TYPE=RelWithDebInfo

# Build
echo -e "Building..."
if command_exists nproc; then
    make -j$(nproc)
else
    make -j4  # Fallback for systems without nproc
fi

# Check if build was successful
if [ -f ./server ]; then
    echo -e "Build successful! Server executable is in the build directory."
    echo -e "To run the server: LD_LIBRARY_PATH=. ./server"
else
    echo -e "Build failed!"
    exit 1
fi
