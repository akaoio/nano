#/usr/bin/env bash
# This script is used to install essential development tools and set up the environment for development.

# Install essential development tools
sudo apt-get update
sudo apt-get install -y build-essential git curl gdb

# Install VLLM and Clang latest version

# Lấy version mới nhất từ trang releases.llvm.org
LLVM_LATEST=$(curl -s https://api.github.com/repos/llvm/llvm-project/releases/latest | grep tag_name | cut -d '"' -f4)

# Loại bỏ chữ 'llvmorg-' nếu có
LLVM_VERSION=$(echo "$LLVM_LATEST" | sed 's/^llvmorg-//')

# Cài key và repo LLVM chính thức
wget https://apt.llvm.org/llvm.sh
chmod +x llvm.sh

# Chạy script với version mới nhất
sudo ./llvm.sh "$LLVM_VERSION"

# Kiểm tra sau khi cài
clang-"$LLVM_VERSION" --version

# Set clang làm compiler cho vscode
