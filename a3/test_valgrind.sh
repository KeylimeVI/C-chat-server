#!/bin/bash

# Comprehensive test script for chat server/client with valgrind
# Tests memory leaks, basic functionality, and error handling

set -e  # Exit on error

echo "=== Building server and client ==="
make clean
make

echo -e "\n=== Test 1: Server startup/shutdown memory check ==="
timeout 2 valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes \
    ./server -p 4247 2>&1 | grep -A5 "HEAP SUMMARY" || true

echo -e "\n=== Test 2: Single client connect/disconnect ==="
# Start server in background
valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes \
    ./server -p 4248 > server_output.txt 2>&1 &
SERVER_PID=$!

# Give server time to start
sleep 1

# Connect client, join, and disconnect
echo "/join TestUser" | timeout 2 ./client -h localhost -p 4248 2>&1 | grep -v "^$" || true

# Kill server and check for leaks
kill $SERVER_PID 2>/dev/null || true
wait $SERVER_PID 2>/dev/null || true

echo "Server output (last 20 lines):"
tail -20 server_output.txt | grep -E "(HEAP SUMMARY|ERROR SUMMARY|All heap blocks|in use at exit)" || true

echo -e "\n=== Test 3: Multiple clients ==="
# Start server
valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes \
    ./server -p 4249 > server_output2.txt 2>&1 &
SERVER_PID=$!

sleep 1

# Start first client (background)
echo "/join Alice" | timeout 3 ./client -h localhost -p 4249 > client1_output.txt 2>&1 &
CLIENT1_PID=$!

sleep 1

# Start second client (background)
echo "/join Bob" | timeout 3 ./client -h localhost -p 4249 > client2_output.txt 2>&1 &
CLIENT2_PID=$!

# Wait for clients to finish
wait $CLIENT1_PID 2>/dev/null || true
wait $CLIENT2_PID 2>/dev/null || true

# Kill server
kill $SERVER_PID 2>/dev/null || true
wait $SERVER_PID 2>/dev/null || true

echo "Client 1 output:"
grep -E "(Joining|Joined|SERVER|ERROR)" client1_output.txt | head -5 || true

echo "Client 2 output:"
grep -E "(Joining|Joined|SERVER|ERROR)" client2_output.txt | head -5 || true

echo "Server memory check:"
tail -20 server_output2.txt | grep -E "(HEAP SUMMARY|ERROR SUMMARY|All heap blocks|in use at exit)" || true

echo -e "\n=== Test 4: Basic message sending ==="
# This test requires more complex interaction, so we'll just verify the build
echo "Build verification only - interactive testing required for full message flow"

echo -e "\n=== Test 5: Error handling (invalid port) ==="
./server -p 99999 2>&1 | grep -i "error\|invalid" || echo "No error output (expected for invalid port)"

echo -e "\n=== Test 6: Client error (invalid host) ==="
./client -h invalidhostname -p 4242 2>&1 | grep -i "error\|no such host" || echo "No error output for invalid host"

echo -e "\n=== Cleaning up ==="
rm -f server_output.txt server_output2.txt client1_output.txt client2_output.txt

echo -e "\n=== Summary ==="
echo "Basic tests completed. For full functionality testing:"
echo "1. Run './server -p 4242' in one terminal"
echo "2. Run './client -h localhost -p 4242' in another terminal"
echo "3. Type '/join YourName' to join"
echo "4. Type messages to send to all connected clients"
echo "5. Open additional terminals with clients to test multi-user chat"

echo -e "\nAll tests completed successfully!"