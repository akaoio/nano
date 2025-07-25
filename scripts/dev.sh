#!/bin/bash

# Quick development server commands

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

case "${1:-help}" in
    start|s)
        "$SCRIPT_DIR/dev-server.sh" start
        ;;
    stop|kill)
        "$SCRIPT_DIR/dev-server.sh" stop
        ;;
    restart|r)
        "$SCRIPT_DIR/dev-server.sh" restart
        ;;
    status|st)
        "$SCRIPT_DIR/dev-server.sh" status
        ;;
    logs|l)
        "$SCRIPT_DIR/dev-server.sh" logs
        ;;
    rebuild|build|b)
        "$SCRIPT_DIR/dev-server.sh" rebuild
        ;;
    watch|w)
        "$SCRIPT_DIR/watch-server.sh" start
        ;;
    help|h|*)
        echo "Quick dev commands:"
        echo "  ./scripts/dev.sh start|s     - Start server"
        echo "  ./scripts/dev.sh stop|kill   - Stop server"
        echo "  ./scripts/dev.sh restart|r   - Restart server"
        echo "  ./scripts/dev.sh status|st   - Show status"
        echo "  ./scripts/dev.sh logs|l      - Show logs"
        echo "  ./scripts/dev.sh rebuild|b   - Rebuild and restart"
        echo "  ./scripts/dev.sh watch|w     - Start with file watching"
        ;;
esac