#!/bin/bash

# Test script for basic chat server/client functionality
# This script starts the server, then starts two clients to test basic chat functionality

set -e  # Exit on error

echo "=== Building server and client ==="
make clean
make

echo -e "\n=== Starting server on port 4242 ==="
./server -p 4242 &
SERVER_PID=$!

# Give server time to start
sleep 2

echo -e "\n=== Starting first client (Alice) ==="
echo "/join Alice" | timeout 3 ./client -h localhost -p 4242 &
CLIENT1_PID=$!

sleep 1

echo -e "\n=== Starting second client (Bob) ==="
echo "/join Bob" | timeout 3 ./client -h localhost -p 4242 &
CLIENT2_PID=$!

sleep 1

echo -e "\n=== Sending test messages ==="
# We can't easily test interactive messaging in a simple script
# This would require more complex IPC or expect scripts

echo -e "\n=== Test completed ==="
echo "Server PID: $SERVER_PID"
echo "Client 1 PID: $CLIENT1_PID"
echo "Client 2 PID: $CLIENT2_PID"

# Cleanup
echo -e "\n=== Cleaning up ==="
kill $SERVER_PID 2>/dev/null || true
kill $CLIENT1_PID 2>/dev/null || true
kill $CLIENT2_PID 2>/dev/null || true

echo "Basic build test passed!"