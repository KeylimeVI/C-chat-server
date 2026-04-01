#!/bin/bash

# Test script to verify the prompt spam fix
# This tests that the client doesn't continuously print prompts

set -e

echo "=== Testing Prompt Spam Fix ==="
echo

# Build the project
echo "Building server and client..."
make clean > /dev/null 2>&1
make > /dev/null 2>&1
echo "Build complete."
echo

# Start server
echo "Starting server on port 4260..."
./server -p 4260 &
SERVER_PID=$!
sleep 2

echo "Starting client (will run for 3 seconds)..."
echo "If working correctly, you should NOT see rapid prompt spam."
echo "You should see only a few prompts at most."
echo

# Run client for 3 seconds without input
timeout 3 ./client -h localhost -p 4260 2>&1 | tee client_output.txt | head -20

echo
echo "=== Analyzing output ==="

# Count how many prompts were printed
PROMPT_COUNT=$(grep -c "^$" client_output.txt || true)
PROMPT_LINES=$(grep -c ">" client_output.txt || true)

echo "Empty lines (likely from prompts): $PROMPT_COUNT"
echo "Lines with '>': $PROMPT_LINES"

if [ "$PROMPT_LINES" -gt 10 ]; then
    echo "WARNING: Too many prompts detected ($PROMPT_LINES). Prompt spam may not be fixed."
else
    echo "GOOD: Reasonable number of prompts detected."
fi

echo
echo "First 10 lines of client output:"
head -10 client_output.txt

echo
echo "=== Cleanup ==="
kill $SERVER_PID 2>/dev/null || true
wait $SERVER_PID 2>/dev/null || true
rm -f client_output.txt

echo
echo "=== Test complete ==="
echo "For interactive testing:"
echo "1. Run: ./server -p 4261"
echo "2. Run: ./client -h localhost -p 4261"
echo "3. Observe if prompts spam continuously"
echo "4. Type /join test and press Enter"
echo "5. Type a message and press Enter"
echo "6. You should see prompts only after actions, not continuously"