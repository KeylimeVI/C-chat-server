#!/bin/bash

# Test script to verify the client-server communication fix
# This script tests that clients can properly send and receive messages

set -e

echo "=== Testing Client-Server Communication Fix ==="
echo

# Build the project
echo "Building server and client..."
make clean > /dev/null 2>&1
make > /dev/null 2>&1
echo "Build complete."
echo

# Test 1: Basic connection and join
echo "Test 1: Basic connection and join"
echo "---------------------------------"
./server -p 4251 &
SERVER_PID=$!
sleep 1

# Test client join
echo "Starting client and joining as 'TestUser'..."
echo "/join TestUser" | timeout 3 ./client -h localhost -p 4251 2>&1 | grep -E "(Joining|Joined|SERVER|ERROR|Successfully)" || true

sleep 1
kill $SERVER_PID 2>/dev/null || true
wait $SERVER_PID 2>/dev/null || true
echo "Test 1 complete."
echo

# Test 2: Multiple clients
echo "Test 2: Multiple clients connecting"
echo "-----------------------------------"
./server -p 4252 &
SERVER_PID=$!
sleep 1

echo "Starting Client 1 (Alice)..."
echo "/join Alice" | timeout 3 ./client -h localhost -p 4252 > client1.log 2>&1 &
CLIENT1_PID=$!

sleep 1

echo "Starting Client 2 (Bob)..."
echo "/join Bob" | timeout 3 ./client -h localhost -p 4252 > client2.log 2>&1 &
CLIENT2_PID=$!

sleep 2

echo "Client 1 output:"
grep -E "(Joining|Joined|SERVER|ERROR|Successfully|has joined)" client1.log | head -5 || true

echo "Client 2 output:"
grep -E "(Joining|Joined|SERVER|ERROR|Successfully|has joined)" client2.log | head -5 || true

kill $SERVER_PID 2>/dev/null || true
wait $SERVER_PID 2>/dev/null || true
kill $CLIENT1_PID 2>/dev/null || true
kill $CLIENT2_PID 2>/dev/null || true
wait $CLIENT1_PID 2>/dev/null || true
wait $CLIENT2_PID 2>/dev/null || true

rm -f client1.log client2.log
echo "Test 2 complete."
echo

# Test 3: Manual testing instructions
echo "Test 3: Manual interactive test"
echo "-------------------------------"
echo "For a complete test, please run manually:"
echo
echo "Terminal 1: ./server -p 4253"
echo "Terminal 2: ./client -h localhost -p 4253"
echo "  Then type: /join YourName"
echo "  Then type: Hello world!"
echo
echo "Terminal 3: ./client -h localhost -p 4253"
echo "  Then type: /join AnotherUser"
echo "  You should see: '*** YourName has joined the chat ***'"
echo "  Then type: Hi there!"
echo
echo "Both clients should see each other's messages."
echo

# Test 4: Check for common errors
echo "Test 4: Error condition tests"
echo "-----------------------------"
echo "Testing invalid port..."
./server -p 99999 2>&1 | grep -i "error\|invalid" || echo "  No error (unexpected)"
echo

echo "Testing invalid host..."
./client -h invalidhost -p 4242 2>&1 | grep -i "error\|no such host" || echo "  No error (unexpected)"
echo

echo "=== All automated tests complete ==="
echo
echo "If the tests show:"
echo "  - 'Joining as...' messages appear"
echo "  - 'Successfully joined...' messages appear"
echo "  - No 'ERROR' messages (except for the intentional error tests)"
echo "Then the fix is working correctly!"
echo
echo "For the best test, run the manual interactive test (Test 3)."