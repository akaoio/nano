#!/bin/bash

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo -e "${GREEN}Updating nano-server service...${NC}"

# Check if we're running as root
if [ "$EUID" -ne 0 ]; then 
    echo -e "${RED}Error: This script must be run as root${NC}"
    echo "Please run with sudo: sudo $0"
    exit 1
fi

# Check if server executable exists
if [ ! -f "./build/server" ]; then
    echo -e "${RED}Error: ./build/server executable not found${NC}"
    echo "Please build the project first: cd build && make"
    exit 1
fi

# Stop the service
echo "Stopping nano-server service..."
systemctl stop nano-server || echo "Service was not running"

# Copy updated files
echo "Copying updated server executable..."
cp ./build/server /opt/nano/server

echo "Copying updated libraries..."
cp ./build/lib*.so /opt/nano/ 2>/dev/null || echo "No libraries to copy"

# Set proper ownership
echo "Setting ownership..."
chown nano:nano /opt/nano/server
chown nano:nano /opt/nano/lib*.so 2>/dev/null || true

# Make executable
chmod +x /opt/nano/server

# Start the service
echo "Starting nano-server service..."
systemctl start nano-server

# Check status
echo "Checking service status..."
if systemctl is-active --quiet nano-server; then
    echo -e "${GREEN}✅ Service updated and started successfully!${NC}"
    systemctl status nano-server --no-pager -l
else
    echo -e "${RED}❌ Service failed to start${NC}"
    systemctl status nano-server --no-pager -l
    exit 1
fi

echo -e "${GREEN}🎉 Update complete!${NC}"