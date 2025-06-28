#!/bin/bash

echo "🧪 Buffer Management & OOM Prevention Test"
echo "=========================================="

SERVER_PORT=6667
SERVER_PASS="mypass"

test_client() {
    local client_id=$1
    local message_count=$2
    
    (
        echo "PASS $SERVER_PASS"
        sleep 0.1
        echo "NICK test$client_id"
        sleep 0.1  
        echo "USER test$client_id 0 * :Test User $client_id"
        sleep 0.2
        echo "JOIN #test"
        sleep 0.1
        
        for j in $(seq 1 $message_count); do
            echo "PRIVMSG #test :Client $client_id Message $j - $(date +%s.%3N)"
            sleep 0.01
        done
        
        echo "PART #test :Test completed"
        echo "QUIT :Client $client_id finished"
    ) | nc localhost $SERVER_PORT &
}

echo "Test 1: Sequential clients with proper authentication..."
for i in {1..5}; do
    echo "Starting client $i..."
    test_client $i 20
    sleep 0.5
done

sleep 2
echo ""

echo "Test 2: Concurrent clients (buffer stress test)..."
for i in {10..20}; do
    test_client $i 50 &
done

sleep 2
echo ""

echo "Test 3: Large message burst test..."
(
    echo "PASS $SERVER_PASS"
    sleep 0.1
    echo "NICK bigmessenger"
    sleep 0.1
    echo "USER bigmessenger 0 * :Big Message User"
    sleep 0.2
    echo "JOIN #test"
    sleep 0.1
    
    for i in {1..100}; do
        large_msg="PRIVMSG #test :LARGE_MSG_$i$(printf 'X%.0s' {1..500})"
        echo "$large_msg"
    done
    
    echo "PART #test :Large message test completed"
    echo "QUIT :Big messenger finished"
) | nc localhost $SERVER_PORT &

echo "Test 4: Memory pressure test (rapid fire messages)..."
(
    echo "PASS $SERVER_PASS"
    sleep 0.1
    echo "NICK rapidfire"
    sleep 0.1  
    echo "USER rapidfire 0 * :Rapid Fire User"
    sleep 0.2
    echo "JOIN #test"
    sleep 0.1
    
    for i in {1..1000}; do
        echo "PRIVMSG #test :Rapid$i"
    done
    
    echo "QUIT :Rapid fire test completed"
) | nc localhost $SERVER_PORT &

echo ""
echo "🔍 Monitor Server For:"
echo "  ✅ 'Buffered message for client X'"
echo "  ✅ 'Partial send to client X'"
echo "  ✅ Stable memory usage"
echo "  ❌ No crashes or OOM errors"

echo ""
echo "⏳ Tests running... Monitor server for ~30 seconds"

wait

echo ""
echo "✅ Test completed! Check server stability and memory usage."
