#!/bin/bash

# Graceful shutdown handler for server rebuilds

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"

# Source PID manager
source "$SCRIPT_DIR/pid-manager.sh"
PID_FILE="$PROJECT_ROOT/.dev-server.pid"

# Wait for active connections to finish
wait_for_connections() {
    local timeout=${1:-30}
    local port=${2:-3000}
    local count=0
    
    echo -n "Waiting for active connections to finish"
    
    while [ $count -lt $timeout ]; do
        # Check if port is still being used
        if ! netstat -ln 2>/dev/null | grep -q ":$port "; then
            echo " OK"
            return 0
        fi
        
        # Count active connections
        local connections=$(netstat -an 2>/dev/null | grep ":$port " | grep ESTABLISHED | wc -l)
        
        if [ "$connections" -eq 0 ]; then
            echo " OK"
            return 0
        fi
        
        echo -n "."
        sleep 1
        count=$((count + 1))
    done
    
    echo " TIMEOUT ($connections active connections)"
    return 1
}

# Send graceful shutdown signal
graceful_shutdown() {
    local signal=${1:-TERM}
    local timeout=${2:-15}
    
    if ! is_running; then
        echo "Server is not running"
        return 0
    fi
    
    local pid=$(read_pid)
    echo -e "${BLUE}Sending $signal signal to server (PID: $pid)${NC}"
    
    # Send the signal
    kill -$signal "$pid" 2>/dev/null || {
        echo -e "${RED}Process not found${NC}"
        remove_pid
        return 1
    }
    
    # Wait for process to stop
    if wait_for_stop $timeout; then
        echo -e "${GREEN}Server shutdown gracefully${NC}"
        return 0
    else
        echo -e "${YELLOW}Graceful shutdown timeout, server may still be running${NC}"
        return 1
    fi
}

# Force shutdown if graceful fails
force_shutdown() {
    if ! is_running; then
        return 0
    fi
    
    local pid=$(read_pid)
    echo -e "${YELLOW}Force killing server (PID: $pid)${NC}"
    
    kill -KILL "$pid" 2>/dev/null || true
    wait_for_stop 5
    
    echo -e "${GREEN}Server force stopped${NC}"
}

# Main shutdown procedure
main_shutdown() {
    local force_mode=${1:-false}
    
    echo -e "${BLUE}Initiating server shutdown...${NC}"
    
    if [ "$force_mode" = "true" ]; then
        force_shutdown
        return 0
    fi
    
    # Try graceful shutdown first
    if graceful_shutdown TERM 15; then
        # Wait for connections to finish
        wait_for_connections 10
        return 0
    fi
    
    # If graceful shutdown fails, try with QUIT signal
    echo -e "${YELLOW}Trying QUIT signal...${NC}"
    if graceful_shutdown QUIT 10; then
        wait_for_connections 5
        return 0
    fi
    
    # Last resort: force kill
    echo -e "${YELLOW}Graceful shutdown failed, forcing...${NC}"
    force_shutdown
}

# Usage
usage() {
    echo "Usage: $0 [graceful|force]"
    echo ""
    echo "Shutdown modes:"
    echo "  graceful  - Try graceful shutdown (default)"
    echo "  force     - Force kill immediately"
    echo ""
    echo "Graceful shutdown process:"
    echo "  1. Send SIGTERM (15s timeout)"
    echo "  2. Send SIGQUIT (10s timeout)"  
    echo "  3. Force kill with SIGKILL"
}

# Main command handler
case "${1:-graceful}" in
    graceful|g)
        main_shutdown false
        ;;
    force|f|kill)
        main_shutdown true
        ;;
    *)
        usage
        exit 1
        ;;
esac