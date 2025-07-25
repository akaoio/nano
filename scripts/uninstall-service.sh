#!/bin/bash

# Uninstall nano-server systemd service

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Configuration
SERVICE_NAME="nano-server"
SERVICE_FILE="nano-server.service"
INSTALL_DIR="/opt/nano"
SERVICE_USER="nano"
LOG_DIR="/var/log/nano"

# Check if running as root
if [ "$EUID" -ne 0 ]; then 
    echo -e "${RED}Error: This script must be run as root${NC}"
    exit 1
fi

echo -e "${YELLOW}Uninstalling $SERVICE_NAME service...${NC}"

# Stop service if running
if systemctl is-active --quiet "$SERVICE_NAME"; then
    echo "Stopping $SERVICE_NAME service..."
    systemctl stop "$SERVICE_NAME"
fi

# Disable service
if systemctl is-enabled --quiet "$SERVICE_NAME" 2>/dev/null; then
    echo "Disabling $SERVICE_NAME service..."
    systemctl disable "$SERVICE_NAME"
fi

# Remove service file
if [ -f "/etc/systemd/system/$SERVICE_FILE" ]; then
    echo "Removing service file..."
    rm -f "/etc/systemd/system/$SERVICE_FILE"
fi

# Reload systemd
echo "Reloading systemd daemon..."
systemctl daemon-reload
systemctl reset-failed "$SERVICE_NAME" 2>/dev/null || true

# Ask about removing application files
read -p "Remove application files from $INSTALL_DIR? (y/N) " -n 1 -r
echo
if [[ $REPLY =~ ^[Yy]$ ]]; then
    echo "Removing application files..."
    rm -rf "$INSTALL_DIR"
else
    echo "Keeping application files in $INSTALL_DIR"
fi

# Ask about removing log files
read -p "Remove log files from $LOG_DIR? (y/N) " -n 1 -r
echo
if [[ $REPLY =~ ^[Yy]$ ]]; then
    echo "Removing log files..."
    rm -rf "$LOG_DIR"
else
    echo "Keeping log files in $LOG_DIR"
fi

# Ask about removing service user
if id -u "$SERVICE_USER" >/dev/null 2>&1; then
    read -p "Remove service user '$SERVICE_USER'? (y/N) " -n 1 -r
    echo
    if [[ $REPLY =~ ^[Yy]$ ]]; then
        echo "Removing service user..."
        userdel "$SERVICE_USER"
    else
        echo "Keeping service user '$SERVICE_USER'"
    fi
fi

echo -e "${GREEN}Service uninstalled successfully!${NC}"