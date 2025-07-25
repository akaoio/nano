#!/bin/bash

# Development server manager with hot reload support

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Configuration
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
DEV_PID_FILE="$PROJECT_ROOT/.dev-server.pid"
SERVER_SCRIPT="$PROJECT_ROOT/server.js"
LOG_FILE="$PROJECT_ROOT/dev-server.log"

# Source PID manager functions but override PID_FILE for dev mode
source "$SCRIPT_DIR/pid-manager.sh"
PID_FILE="$DEV_PID_FILE"

# Print usage
usage() {
    echo "Usage: $0 {start|stop|restart|status|logs|rebuild}"
    echo ""
    echo "Development Commands:"
    echo "  start    - Start development server"
    echo "  stop     - Stop development server"
    echo "  restart  - Restart development server"
    echo "  status   - Show server status"
    echo "  logs     - Show server logs (tail -f)"
    echo "  rebuild  - Stop server, rebuild, and restart"
    echo ""
}

# Start development server
start_dev_server() {
    echo -e "${GREEN}Starting development server...${NC}"
    
    if is_running; then
        echo -e "${YELLOW}Server is already running${NC}"
        get_status
        return 0
    fi
    
    # Start server in background and capture PID
    echo -e "${BLUE}Starting server: node $SERVER_SCRIPT${NC}"
    nohup node "$SERVER_SCRIPT" > "$LOG_FILE" 2>&1 &
    local server_pid=$!
    
    # Store PID
    create_pid "$server_pid"
    
    # Wait for server to start
    if wait_for_start 10; then
        echo -e "${GREEN}Development server started successfully (PID: $server_pid)${NC}"
        echo "Logs: tail -f $LOG_FILE"
        return 0
    else
        echo -e "${RED}Failed to start development server${NC}"
        show_logs_tail
        return 1
    fi
}

# Stop development server
stop_dev_server() {
    echo -e "${YELLOW}Stopping development server...${NC}"
    
    if ! is_running; then
        echo "Server is not running"
        remove_pid 2>/dev/null || true
        return 0
    fi
    
    local pid=$(read_pid)
    echo "Sending SIGTERM to process $pid..."
    
    # Send SIGTERM for graceful shutdown
    kill -TERM "$pid" 2>/dev/null || {
        echo -e "${RED}Process not found, cleaning up PID file${NC}"
        remove_pid
        return 0
    }
    
    # Wait for graceful shutdown
    if wait_for_stop 10; then
        echo -e "${GREEN}Development server stopped successfully${NC}"
        return 0
    else
        echo -e "${YELLOW}Graceful shutdown timeout, forcing kill...${NC}"
        kill -KILL "$pid" 2>/dev/null || true
        wait_for_stop 5
        echo -e "${GREEN}Development server force stopped${NC}"
        return 0
    fi
}

# Restart development server
restart_dev_server() {
    echo -e "${BLUE}Restarting development server...${NC}"
    stop_dev_server
    sleep 1
    start_dev_server
}

# Show development server status
show_dev_status() {
    echo -e "${BLUE}Development Server Status:${NC}"
    get_status
    
    if is_running; then
        local pid=$(read_pid)
        echo ""
        echo "Process info:"
        ps -p "$pid" -o pid,ppid,cmd,etime,pcpu,pmem 2>/dev/null || echo "Process details unavailable"
        
        if [ -f "$LOG_FILE" ]; then
            echo ""
            echo "Recent log entries:"
            tail -n 5 "$LOG_FILE"
        fi
    fi
}

# Show logs
show_logs_tail() {
    if [ -f "$LOG_FILE" ]; then
        echo -e "${BLUE}Showing development server logs (Ctrl+C to exit)...${NC}"
        tail -f "$LOG_FILE"
    else
        echo -e "${YELLOW}No log file found at $LOG_FILE${NC}"
    fi
}

# Rebuild and restart server
rebuild_server() {
    echo -e "${BLUE}Rebuilding and restarting server...${NC}"
    
    # Stop server if running
    if is_running; then
        echo "Stopping server for rebuild..."
        stop_dev_server
    fi
    
    # Check if there's a build script
    cd "$PROJECT_ROOT"
    
    if [ -f "package.json" ] && grep -q '"build"' package.json; then
        echo "Running npm build..."
        npm run build
    elif [ -f "Makefile" ]; then
        echo "Running make..."
        make
    else
        echo -e "${YELLOW}No build script found, assuming no build step needed${NC}"
    fi
    
    # Restart server
    echo "Restarting server..."
    start_dev_server
}

# Handle signals for graceful shutdown
cleanup() {
    echo ""
    echo -e "${YELLOW}Received interrupt signal, stopping server...${NC}"
    stop_dev_server
    exit 0
}

trap cleanup SIGINT SIGTERM

# Main command handler
case "$1" in
    start)
        start_dev_server
        ;;
    stop)
        stop_dev_server
        ;;
    restart)
        restart_dev_server
        ;;
    status)
        show_dev_status
        ;;
    logs)
        show_logs_tail
        ;;
    rebuild)
        rebuild_server
        ;;
    *)
        usage
        exit 1
        ;;
esac