#!/bin/bash

# PID management functions for nano server

PID_FILE="/var/run/nano-server.pid"
SERVER_NAME="nano-server"

# Create PID file
create_pid() {
    local pid=$1
    if [ -z "$pid" ]; then
        echo "Error: PID is required"
        return 1
    fi
    
    # Create directory if it doesn't exist
    mkdir -p $(dirname "$PID_FILE")
    
    # Write PID to file
    echo "$pid" > "$PID_FILE"
    
    if [ $? -eq 0 ]; then
        echo "PID $pid written to $PID_FILE"
        return 0
    else
        echo "Error: Failed to write PID file"
        return 1
    fi
}

# Read PID from file
read_pid() {
    if [ -f "$PID_FILE" ]; then
        cat "$PID_FILE"
        return 0
    else
        echo "Error: PID file not found at $PID_FILE" >&2
        return 1
    fi
}

# Check if process is running
is_running() {
    local pid=$(read_pid 2>/dev/null)
    
    if [ -z "$pid" ]; then
        return 1
    fi
    
    # Check if process exists
    if kill -0 "$pid" 2>/dev/null; then
        return 0
    else
        return 1
    fi
}

# Remove PID file
remove_pid() {
    if [ -f "$PID_FILE" ]; then
        rm -f "$PID_FILE"
        echo "PID file removed"
        return 0
    else
        echo "PID file not found"
        return 1
    fi
}

# Get process status
get_status() {
    if is_running; then
        local pid=$(read_pid)
        echo "$SERVER_NAME is running (PID: $pid)"
        return 0
    else
        echo "$SERVER_NAME is not running"
        return 1
    fi
}

# Wait for process to start
wait_for_start() {
    local timeout=${1:-30}
    local count=0
    
    echo -n "Waiting for $SERVER_NAME to start"
    while [ $count -lt $timeout ]; do
        if is_running; then
            echo " OK"
            return 0
        fi
        echo -n "."
        sleep 1
        count=$((count + 1))
    done
    
    echo " TIMEOUT"
    return 1
}

# Wait for process to stop
wait_for_stop() {
    local timeout=${1:-30}
    local count=0
    
    echo -n "Waiting for $SERVER_NAME to stop"
    while [ $count -lt $timeout ]; do
        if ! is_running; then
            echo " OK"
            remove_pid
            return 0
        fi
        echo -n "."
        sleep 1
        count=$((count + 1))
    done
    
    echo " TIMEOUT"
    return 1
}

# Export functions if sourced
if [ "${BASH_SOURCE[0]}" != "${0}" ]; then
    export -f create_pid
    export -f read_pid
    export -f is_running
    export -f remove_pid
    export -f get_status
    export -f wait_for_start
    export -f wait_for_stop
fi