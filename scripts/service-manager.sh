#!/bin/bash

# Service management script for nano-server

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Configuration
SERVICE_NAME="nano-server"
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# Source PID manager functions
source "$SCRIPT_DIR/pid-manager.sh"

# Check if running as root for certain operations
check_root() {
    if [ "$EUID" -ne 0 ]; then 
        echo -e "${RED}Error: This operation requires root privileges${NC}"
        echo "Please run with sudo: sudo $0 $@"
        exit 1
    fi
}

# Print usage
usage() {
    echo "Usage: $0 {start|stop|restart|status|enable|disable|logs|install|uninstall}"
    echo ""
    echo "Commands:"
    echo "  start      - Start the service"
    echo "  stop       - Stop the service"
    echo "  restart    - Restart the service"
    echo "  status     - Show service status"
    echo "  enable     - Enable service to start on boot"
    echo "  disable    - Disable service from starting on boot"
    echo "  logs       - Show service logs (follow mode)"
    echo "  install    - Install the service"
    echo "  uninstall  - Uninstall the service"
    echo ""
    echo "Note: Some operations require root privileges"
}

# Start service
start_service() {
    check_root
    echo -e "${GREEN}Starting $SERVICE_NAME...${NC}"
    
    if systemctl is-active --quiet "$SERVICE_NAME"; then
        echo -e "${YELLOW}Service is already running${NC}"
        systemctl status "$SERVICE_NAME" --no-pager
        return 0
    fi
    
    systemctl start "$SERVICE_NAME"
    
    # Wait for service to start
    if wait_for_start 10; then
        echo -e "${GREEN}Service started successfully${NC}"
        systemctl status "$SERVICE_NAME" --no-pager
        return 0
    else
        echo -e "${RED}Failed to start service${NC}"
        systemctl status "$SERVICE_NAME" --no-pager
        return 1
    fi
}

# Stop service
stop_service() {
    check_root
    echo -e "${YELLOW}Stopping $SERVICE_NAME...${NC}"
    
    if ! systemctl is-active --quiet "$SERVICE_NAME"; then
        echo "Service is not running"
        return 0
    fi
    
    systemctl stop "$SERVICE_NAME"
    
    # Wait for service to stop
    if wait_for_stop 10; then
        echo -e "${GREEN}Service stopped successfully${NC}"
        return 0
    else
        echo -e "${RED}Failed to stop service gracefully${NC}"
        return 1
    fi
}

# Restart service
restart_service() {
    check_root
    echo -e "${BLUE}Restarting $SERVICE_NAME...${NC}"
    
    if systemctl is-active --quiet "$SERVICE_NAME"; then
        systemctl restart "$SERVICE_NAME"
    else
        systemctl start "$SERVICE_NAME"
    fi
    
    # Wait for service to start
    if wait_for_start 10; then
        echo -e "${GREEN}Service restarted successfully${NC}"
        systemctl status "$SERVICE_NAME" --no-pager
        return 0
    else
        echo -e "${RED}Failed to restart service${NC}"
        systemctl status "$SERVICE_NAME" --no-pager
        return 1
    fi
}

# Show status
show_status() {
    echo -e "${BLUE}$SERVICE_NAME Status:${NC}"
    
    if systemctl is-loaded --quiet "$SERVICE_NAME" 2>/dev/null; then
        systemctl status "$SERVICE_NAME" --no-pager
        
        # Also show PID file status
        echo ""
        get_status
    else
        echo -e "${YELLOW}Service is not installed${NC}"
        echo "Run '$0 install' to install the service"
    fi
}

# Enable service
enable_service() {
    check_root
    echo -e "${GREEN}Enabling $SERVICE_NAME to start on boot...${NC}"
    
    if ! systemctl is-loaded --quiet "$SERVICE_NAME" 2>/dev/null; then
        echo -e "${RED}Error: Service is not installed${NC}"
        echo "Run '$0 install' first"
        return 1
    fi
    
    systemctl enable "$SERVICE_NAME"
    echo -e "${GREEN}Service enabled successfully${NC}"
}

# Disable service
disable_service() {
    check_root
    echo -e "${YELLOW}Disabling $SERVICE_NAME from starting on boot...${NC}"
    
    if ! systemctl is-loaded --quiet "$SERVICE_NAME" 2>/dev/null; then
        echo -e "${RED}Error: Service is not installed${NC}"
        return 1
    fi
    
    systemctl disable "$SERVICE_NAME"
    echo -e "${GREEN}Service disabled successfully${NC}"
}

# Show logs
show_logs() {
    echo -e "${BLUE}Showing logs for $SERVICE_NAME (Ctrl+C to exit)...${NC}"
    journalctl -u "$SERVICE_NAME" -f
}

# Install service
install_service() {
    check_root
    "$SCRIPT_DIR/install-service.sh"
}

# Uninstall service
uninstall_service() {
    check_root
    "$SCRIPT_DIR/uninstall-service.sh"
}

# Main command handler
case "$1" in
    start)
        start_service
        ;;
    stop)
        stop_service
        ;;
    restart)
        restart_service
        ;;
    status)
        show_status
        ;;
    enable)
        enable_service
        ;;
    disable)
        disable_service
        ;;
    logs)
        show_logs
        ;;
    install)
        install_service
        ;;
    uninstall)
        uninstall_service
        ;;
    *)
        usage
        exit 1
        ;;
esac