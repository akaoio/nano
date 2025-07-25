#!/bin/bash

# File watcher for automatic server restart on changes

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
DEV_SERVER_SCRIPT="$SCRIPT_DIR/dev-server.sh"

# Files and directories to watch
WATCH_DIRS=(
    "$PROJECT_ROOT/src"
    "$PROJECT_ROOT/server.js"
    "$PROJECT_ROOT/package.json"
)

# Files to ignore (will be converted to grep pattern)
IGNORE_PATTERNS=(
    "\.log$"
    "\.pid$"
    "node_modules"
    "\.git"
    "\.tmp"
    "\.swp$"
    "\.DS_Store$"
)

# Check if inotify-tools is available
check_dependencies() {
    if ! command -v inotifywait >/dev/null 2>&1; then
        echo -e "${RED}Error: inotifywait not found${NC}"
        echo "Install inotify-tools:"
        echo "  Ubuntu/Debian: sudo apt-get install inotify-tools"
        echo "  CentOS/RHEL: sudo yum install inotify-tools"
        echo "  Arch: sudo pacman -S inotify-tools"
        exit 1
    fi
}

# Create ignore pattern for grep
create_ignore_pattern() {
    local pattern=""
    for ignore in "${IGNORE_PATTERNS[@]}"; do
        if [ -n "$pattern" ]; then
            pattern="$pattern\\|$ignore"
        else
            pattern="$ignore"
        fi
    done
    echo "$pattern"
}

# Start file watcher
start_watcher() {
    local ignore_pattern=$(create_ignore_pattern)
    
    echo -e "${GREEN}Starting file watcher for development server...${NC}"
    echo "Watching directories:"
    for dir in "${WATCH_DIRS[@]}"; do
        if [ -e "$dir" ]; then
            echo "  ✓ $dir"
        else
            echo -e "  ${YELLOW}⚠ $dir (not found)${NC}"
        fi
    done
    
    echo ""
    echo -e "${BLUE}File watcher is running. Press Ctrl+C to stop.${NC}"
    echo "Ignoring: ${IGNORE_PATTERNS[*]}"
    echo ""
    
    # Start the development server first
    "$DEV_SERVER_SCRIPT" start
    
    # Watch for file changes
    while true; do
        # Build list of existing watch targets
        local existing_dirs=()
        for dir in "${WATCH_DIRS[@]}"; do
            if [ -e "$dir" ]; then
                existing_dirs+=("$dir")
            fi
        done
        
        if [ ${#existing_dirs[@]} -eq 0 ]; then
            echo -e "${RED}No watch targets found, exiting${NC}"
            break
        fi
        
        # Wait for file changes
        local changed_file=$(inotifywait -r -e modify,create,delete,move \
            --format '%w%f' "${existing_dirs[@]}" 2>/dev/null)
        
        # Skip if file matches ignore pattern
        if echo "$changed_file" | grep -q "$ignore_pattern"; then
            continue
        fi
        
        echo -e "${YELLOW}File changed: $changed_file${NC}"
        echo -e "${BLUE}Restarting server...${NC}"
        
        # Restart the server
        "$DEV_SERVER_SCRIPT" restart
        
        # Brief pause to avoid rapid restarts
        sleep 1
    done
}

# Stop watcher and server
stop_watcher() {
    echo ""
    echo -e "${YELLOW}Stopping file watcher and development server...${NC}"
    "$DEV_SERVER_SCRIPT" stop
    exit 0
}

# Print usage
usage() {
    echo "Usage: $0 [start|stop]"
    echo ""
    echo "File Watcher Commands:"
    echo "  start  - Start file watcher and development server"
    echo "  stop   - Stop development server"
    echo ""
    echo "The watcher monitors these locations:"
    for dir in "${WATCH_DIRS[@]}"; do
        echo "  - $dir"
    done
    echo ""
    echo "Ignored patterns: ${IGNORE_PATTERNS[*]}"
}

# Setup signal handlers
trap stop_watcher SIGINT SIGTERM

# Check dependencies
check_dependencies

# Main command handler
case "${1:-start}" in
    start)
        start_watcher
        ;;
    stop)
        "$DEV_SERVER_SCRIPT" stop
        ;;
    *)
        usage
        exit 1
        ;;
esac