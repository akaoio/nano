# RKLLM Unix Domain Socket Server - Build Instructions

## Quick Start

```bash
# Install dependencies (Ubuntu/Debian)
sudo apt-get update
sudo apt-get install -y build-essential cmake pkg-config libjson-c-dev

# Build
mkdir build && cd build
cmake ..
make

# Run
LD_LIBRARY_PATH=../src/external/rkllm ./rkllm_uds_server
```

## Development

```bash
# Debug build
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Debug ..
make

# Run with gdb
LD_LIBRARY_PATH=../src/external/rkllm gdb ./rkllm_uds_server
```

## Dependencies

- **cmake** >= 3.16
- **json-c** (libjson-c-dev)
- **pthread** (usually included)

## File Structure

- `src/` - Source code following strict rules (see docs/RULES.md)
- `src/external/rkllm/` - RKLLM library (auto-downloaded)
- `build/` - CMake build directory
- `CMakeLists.txt` - Build configuration
- `docs/RULES.md` - **MANDATORY** development rules

## Development Rules

**CRITICAL**: Read `docs/RULES.md` before coding!

- Rule #1: One function per file
- Rule #2: No two functions in one file  
- Rule #3: Naming: `<name>/<name>.<c|h>`

Example:
```
src/server/create_socket/create_socket.c
src/server/create_socket/create_socket.h
```

## Notes

- RKLLM library and header are downloaded automatically by CMake
- Uses standard CMake build process
- No manual Makefile needed (CMake generates it)