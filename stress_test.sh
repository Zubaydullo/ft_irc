#!/bin/bash

echo "Starting stress test..."
for i in {1..20}; do
    (
        echo "PASS password"
        echo "NICK user$i" 
        echo "USER user$i 0 * :User $i"
        echo "JOIN #stress"
        echo "PRIVMSG #stress :Message from user $i"
        sleep 1
        echo "QUIT"
    ) | nc localhost 6667 &
done

wait
echo "Stress test completed"
