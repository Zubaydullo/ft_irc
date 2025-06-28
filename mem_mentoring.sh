#!/bin/bash

echo "📊 IRC Server Memory Monitor"
echo "============================"

SERVER_PID=$(pgrep -f "ircserv.*6667" | head -1)

if [ -z "$SERVER_PID" ]; then
    echo "❌ No ircserv process found on port 6667"
    echo "💡 Start your server first: ./ircserv 6667 mypass"
    exit 1
fi

echo "🎯 Monitoring ircserv PID: $SERVER_PID"
echo "📈 Memory usage shown every 2 seconds"
echo ""

GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
NC='\033[0m'

monitor_memory() {
    while kill -0 $SERVER_PID 2>/dev/null; do
        if command -v ps >/dev/null; then
            MEM_INFO=$(ps -p $SERVER_PID -o pid,vsz,rss,pcpu,comm 2>/dev/null)
            if [ $? -eq 0 ]; then
                echo -e "${GREEN}$(date '+%H:%M:%S')${NC} $MEM_INFO"
            fi
        fi
        sleep 2
    done
}

echo "🔍 Key Indicators:"
echo ""
echo -e "${GREEN}✅ GOOD SIGNS:${NC}"
echo "  - 'Buffered message for client X'"
echo "  - 'Partial send to client X'"  
echo "  - Stable RSS memory"
echo ""
echo -e "${RED}❌ BAD SIGNS:${NC}"
echo "  - RSS memory constantly increasing"
echo "  - Server crashes"
echo "  - Connection failures"
echo ""
echo -e "${YELLOW}📊 MEMORY COLUMNS:${NC}"
echo "  VSZ = Virtual memory (KB)"
echo "  RSS = Physical memory (KB)"
echo "  %CPU = CPU usage"
echo ""

echo "📊 Memory monitoring (Press Ctrl+C to stop):"
echo "PID     VSZ    RSS  %CPU COMMAND"
echo "--------------------------------"

monitor_memory &
MONITOR_PID=$!

trap 'kill $MONITOR_PID 2>/dev/null; echo -e "\n🛑 Monitoring stopped"; exit 0' INT

echo ""
echo "💡 Run buffer test: bash oom_test.sh"
echo ""

wait $MONITOR_PID
