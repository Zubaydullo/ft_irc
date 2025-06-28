#!/bin/bash

echo "🧪 Comprehensive IRC Server Test Suite"
echo "======================================"

SERVER_PORT=6667
SERVER_PASS="mypass"

GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

run_test() {
    local test_name="$1"
    echo -e "\n${BLUE}🧪 Testing: $test_name${NC}"
    echo "----------------------------------------"
}

pass_test() {
    echo -e "${GREEN}✅ PASS${NC}"
}

fail_test() {
    echo -e "${RED}❌ FAIL${NC}"
}

run_test "Partial Message Handling (nc with Ctrl+D)"
echo "Testing message reconstruction from partial packets..."
(
    echo -n "PASS "
    sleep 0.1
    echo -n "$SERVER_PASS"
    sleep 0.1
    echo -e "\r\n"
    sleep 0.1
    echo -n "NICK "
    sleep 0.1
    echo -n "partial"
    sleep 0.1
    echo -e "test\r\n"
    sleep 0.1
    echo "USER partialtest 0 * :Partial Test User"
    sleep 0.5
    echo "QUIT :Partial test done"
) | nc localhost $SERVER_PORT &

sleep 2

run_test "Authentication Edge Cases"
echo "Testing wrong password, empty nick, empty user..."

echo "Wrong password test:"
(
    echo "PASS wrongpass"
    sleep 0.2
    echo "NICK testuser"
    echo "USER testuser 0 * :Test"
) | timeout 3 nc localhost $SERVER_PORT &

sleep 1

echo "Empty nickname test:"
(
    echo "PASS $SERVER_PASS"
    echo "NICK"
    echo "USER testuser 0 * :Test"
) | timeout 3 nc localhost $SERVER_PORT &

sleep 1

echo "Empty username test:"
(
    echo "PASS $SERVER_PASS"
    echo "NICK testuser"
    echo "USER"
) | timeout 3 nc localhost $SERVER_PORT &

sleep 2

run_test "Channel Operations"
echo "Testing channel creation, join, part, modes..."

(
    echo "PASS $SERVER_PASS"
    sleep 0.1
    echo "NICK chanop"
    sleep 0.1
    echo "USER chanop 0 * :Channel Operator"
    sleep 0.3
    echo "JOIN #testchan"
    sleep 0.2
    echo "MODE #testchan +t"
    sleep 0.1
    echo "MODE #testchan +i"
    sleep 0.1
    echo "TOPIC #testchan :This is a test topic"
    sleep 2
    echo "QUIT :Channel op done"
) | nc localhost $SERVER_PORT &

sleep 1
(
    echo "PASS $SERVER_PASS"
    sleep 0.1
    echo "NICK joiner"
    sleep 0.1
    echo "USER joiner 0 * :Channel Joiner"
    sleep 0.3
    echo "JOIN #testchan"
    sleep 1
    echo "QUIT :Joiner done"
) | nc localhost $SERVER_PORT &

sleep 3

run_test "PRIVMSG Edge Cases"
echo "Testing private messages, channel messages, non-existent targets..."

(
    echo "PASS $SERVER_PASS"
    sleep 0.1
    echo "NICK msgtest"
    sleep 0.1
    echo "USER msgtest 0 * :Message Tester"
    sleep 0.3
    echo "JOIN #msgtest"
    sleep 0.2
    echo "PRIVMSG #msgtest :Hello channel"
    sleep 0.1
    echo "PRIVMSG nonexistent :This should fail"
    sleep 0.1
    echo "PRIVMSG #nonexistent :This should also fail"
    sleep 0.1
    echo "PRIVMSG"
    sleep 0.1
    echo "PRIVMSG msgtest"
    sleep 1
    echo "QUIT :Message test done"
) | nc localhost $SERVER_PORT &

sleep 2

run_test "Multiple Rapid Connections (FD Leak Test)"
echo "Testing rapid connect/disconnect to check fd leaks..."

for i in {1..20}; do
    (
        echo "PASS $SERVER_PASS"
        echo "NICK rapid$i"
        echo "USER rapid$i 0 * :Rapid User $i"
        sleep 0.1
        echo "QUIT :Rapid disconnect"
    ) | timeout 2 nc localhost $SERVER_PORT &
done

sleep 3

run_test "Large Message Handling"
echo "Testing very large messages (potential buffer overflow)..."

(
    echo "PASS $SERVER_PASS"
    sleep 0.1
    echo "NICK bigmsg"
    sleep 0.1
    echo "USER bigmsg 0 * :Big Message User"
    sleep 0.3
    echo "JOIN #bigtest"
    sleep 0.2
    
    large_msg="PRIVMSG #bigtest :$(printf 'A%.0s' {1..400})"
    echo "$large_msg"
    
    huge_msg="PRIVMSG #bigtest :$(printf 'B%.0s' {1..1000})"
    echo "$huge_msg"
    
    sleep 1
    echo "QUIT :Big message test done"
) | nc localhost $SERVER_PORT &

sleep 2

run_test "Operator Commands (KICK, INVITE, MODE)"
echo "Testing operator-only commands..."

(
    echo "PASS $SERVER_PASS"
    sleep 0.1
    echo "NICK operator"
    sleep 0.1
    echo "USER operator 0 * :Operator User"
    sleep 0.3
    echo "JOIN #optest"
    sleep 0.2
    echo "MODE #optest +i"
    sleep 0.1
    echo "INVITE normaluser #optest"
    sleep 2
    echo "KICK normaluser #optest :Test kick"
    sleep 1
    echo "QUIT :Operator done"
) | nc localhost $SERVER_PORT &

sleep 1
(
    echo "PASS $SERVER_PASS"
    sleep 0.1
    echo "NICK normaluser"
    sleep 0.1
    echo "USER normaluser 0 * :Normal User"
    sleep 0.3
    echo "JOIN #optest"
    sleep 0.5
    echo "MODE #optest +t"
    sleep 0.5
    echo "KICK operator #optest :Normal user trying to kick"
    sleep 2
    echo "QUIT :Normal user done"
) | nc localhost $SERVER_PORT &

sleep 4

run_test "Stress Test (Multiple Simultaneous Operations)"
echo "Testing server under heavy load..."

for i in {1..10}; do
    (
        echo "PASS $SERVER_PASS"
        echo "NICK stress$i"
        echo "USER stress$i 0 * :Stress User $i"
        sleep 0.2
        echo "JOIN #stress"
        sleep 0.1
        
        for j in {1..20}; do
            echo "PRIVMSG #stress :Stress message $j from user $i"
            sleep 0.01
        done
        
        echo "PART #stress :Stress test complete"
        echo "QUIT :Stress user $i done"
    ) | nc localhost $SERVER_PORT &
done

sleep 5

run_test "Standard IRC Client Commands"
echo "Testing PING, NAMES, TOPIC, etc..."

(
    echo "PASS $SERVER_PASS"
    sleep 0.1
    echo "NICK irctest"
    sleep 0.1
    echo "USER irctest 0 * :IRC Test User"
    sleep 0.3
    echo "JOIN #irctest"
    sleep 0.2
    echo "NAMES #irctest"
    sleep 0.1
    echo "TOPIC #irctest"
    sleep 0.1
    echo "TOPIC #irctest :New test topic"
    sleep 0.1
    echo "PING :test.server.com"
    sleep 0.1
    echo "QUIT :IRC test done"
) | nc localhost $SERVER_PORT &

sleep 2

run_test "Malformed Input and Edge Cases"
echo "Testing server resilience to bad input..."

(
    echo "PASS $SERVER_PASS"
    sleep 0.1
    echo "NICK edgetest"
    sleep 0.1
    echo "USER edgetest 0 * :Edge Test User"
    sleep 0.3
    echo ""
    echo "    "
    echo "INVALID_COMMAND"
    echo "JOIN"
    echo "PRIVMSG"
    echo "MODE"
    echo "KICK"
    echo "INVITE"
    printf '\000\001\002\003'
    echo ""
    sleep 1
    echo "QUIT :Edge test done"
) | nc localhost $SERVER_PORT &

sleep 2

echo ""
echo -e "${YELLOW}📋 Test Summary${NC}"
echo "================================="
echo "✅ All tests completed!"
echo ""
echo "🔍 Check server logs for:"
echo "  - No crashes or segfaults"
echo "  - Proper error handling"
echo "  - Buffer management working"
echo "  - Authentication flow correct"
echo "  - Channel operations working"
echo "  - Operator commands functioning"
echo ""
echo "📊 Memory should remain stable throughout all tests"
echo "🎯 Server should handle all edge cases gracefully"
