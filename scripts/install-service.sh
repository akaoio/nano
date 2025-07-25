#!/bin/bash

# Install nano-server as a systemd service

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
SERVICE_GROUP="nano"
LOG_DIR="/var/log/nano"

# Check if running as root
if [ "$EUID" -ne 0 ]; then 
    echo -e "${RED}Error: This script must be run as root${NC}"
    exit 1
fi

echo -e "${GREEN}Installing $SERVICE_NAME service...${NC}"

# Create service user if it doesn't exist
if ! id -u "$SERVICE_USER" >/dev/null 2>&1; then
    echo "Creating service user: $SERVICE_USER"
    useradd -r -s /bin/false -d /nonexistent -c "Nano Server Service" "$SERVICE_USER"
else
    echo "Service user $SERVICE_USER already exists"
fi

# Create installation directory
echo "Creating installation directory: $INSTALL_DIR"
mkdir -p "$INSTALL_DIR"

# Copy application files
echo "Copying application files..."
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"

# Copy necessary files
cp -r "$PROJECT_ROOT"/{build/server,build/lib*.so,src} "$INSTALL_DIR/" 2>/dev/null || {
    echo -e "${YELLOW}Warning: Some files may not exist. Make sure to build the project first.${NC}"
}

# Create log directories
echo "Creating log directory: $LOG_DIR"
mkdir -p "$LOG_DIR"
chown "$SERVICE_USER:$SERVICE_GROUP" "$LOG_DIR"

echo "Creating application log directory: $INSTALL_DIR/logs"
mkdir -p "$INSTALL_DIR/logs"

# Set ownership
echo "Setting ownership..."
chown -R "$SERVICE_USER:$SERVICE_GROUP" "$INSTALL_DIR"

# Copy service file
echo "Installing systemd service file..."
cp "$SCRIPT_DIR/$SERVICE_FILE" "/etc/systemd/system/"

# Update service file with actual paths
sed -i "s|/opt/nano|$INSTALL_DIR|g" "/etc/systemd/system/$SERVICE_FILE"
sed -i "s|User=nano|User=$SERVICE_USER|g" "/etc/systemd/system/$SERVICE_FILE"
sed -i "s|Group=nano|Group=$SERVICE_GROUP|g" "/etc/systemd/system/$SERVICE_FILE"

# Reload systemd
echo "Reloading systemd daemon..."
systemctl daemon-reload

# Enable service
echo "Enabling service..."
systemctl enable "$SERVICE_NAME"

echo -e "${GREEN}Service installed successfully!${NC}"
echo ""
echo "To start the service, run:"
echo "  sudo systemctl start $SERVICE_NAME"
echo ""
echo "To check service status:"
echo "  sudo systemctl status $SERVICE_NAME"
echo ""
echo "To view logs:"
echo "  sudo journalctl -u $SERVICE_NAME -f"