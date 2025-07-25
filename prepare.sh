#!/bin/bash

# prepare.sh - Download required model files for nano project

set -e  # Exit on any error

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Configuration
MODEL_DIR="models/qwen2vl2b"
BASE_URL="https://huggingface.co/thanhtantran/Qwen2-VL-2B-RKLLM/resolve/main"
FILES=(
    "Qwen2-VL-2B-Instruct.rkllm"
    "Qwen2-VL-2B-Instruct.rknn"
)

echo -e "${GREEN}=== Nano Project Model Preparation ===${NC}"
echo "Target directory: $MODEL_DIR"

# Create model directory if it doesn't exist
if [ ! -d "$MODEL_DIR" ]; then
    echo -e "${YELLOW}Creating directory: $MODEL_DIR${NC}"
    mkdir -p "$MODEL_DIR"
else
    echo -e "${GREEN}Directory already exists: $MODEL_DIR${NC}"
fi

# Check if wget is available
if ! command -v wget &> /dev/null; then
    echo -e "${RED}Error: wget is not installed. Please install wget first.${NC}"
    echo "On Ubuntu/Debian: sudo apt-get install wget"
    echo "On CentOS/RHEL: sudo yum install wget"
    exit 1
fi

# Download each file
for file in "${FILES[@]}"; do
    file_path="$MODEL_DIR/$file"
    url="$BASE_URL/$file"
    
    echo -e "${YELLOW}Downloading: $file${NC}"
    echo "From: $url"
    echo "To: $file_path"
    
    # Check if file already exists
    if [ -f "$file_path" ]; then
        echo -e "${YELLOW}File already exists. Do you want to re-download? (y/N)${NC}"
        read -r response
        if [[ ! "$response" =~ ^[Yy]$ ]]; then
            echo -e "${GREEN}Skipping $file${NC}"
            continue
        fi
    fi
    
    # Download with progress bar and resume capability
    if wget --progress=bar:force:noscroll --continue --timeout=30 --tries=3 "$url" -O "$file_path"; then
        echo -e "${GREEN}✓ Successfully downloaded: $file${NC}"
        
        # Display file size
        if [ -f "$file_path" ]; then
            file_size=$(du -h "$file_path" | cut -f1)
            echo -e "${GREEN}  File size: $file_size${NC}"
        fi
    else
        echo -e "${RED}✗ Failed to download: $file${NC}"
        echo -e "${RED}  Please check your internet connection and try again.${NC}"
        exit 1
    fi
    
    echo ""
done

echo -e "${GREEN}=== Download Complete ===${NC}"
echo "All model files have been downloaded to: $MODEL_DIR"
echo ""
echo "Downloaded files:"
for file in "${FILES[@]}"; do
    file_path="$MODEL_DIR/$file"
    if [ -f "$file_path" ]; then
        file_size=$(du -h "$file_path" | cut -f1)
        echo -e "${GREEN}  ✓ $file ($file_size)${NC}"
    else
        echo -e "${RED}  ✗ $file (missing)${NC}"
    fi
done

echo -e "${GREEN}Preparation complete!${NC}"