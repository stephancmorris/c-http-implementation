#!/bin/bash

# NanoServe HTTP Server Test Suite
# Story 2.3: Basic Request/Response Test

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Test counters
TESTS_RUN=0
TESTS_PASSED=0
TESTS_FAILED=0

# Server configuration
PORT=8080
SERVER_PID=""

# Print colored output
print_test() {
    echo -e "${BLUE}[TEST $TESTS_RUN]${NC} $1"
}

print_pass() {
    echo -e "${GREEN}[PASS]${NC} $1"
    ((TESTS_PASSED++))
}

print_fail() {
    echo -e "${RED}[FAIL]${NC} $1"
    ((TESTS_FAILED++))
}

print_info() {
    echo -e "${YELLOW}[INFO]${NC} $1"
}

# Start the server
start_server() {
    print_info "Starting NanoServe on port $PORT..."

    # Build the server first
    make clean > /dev/null 2>&1
    make all > /dev/null 2>&1

    # Start server in background
    ./bin/nanoserve > /tmp/nanoserve_test.log 2>&1 &
    SERVER_PID=$!

    # Wait for server to be ready
    sleep 1

    # Check if server is running
    if ! ps -p $SERVER_PID > /dev/null; then
        echo -e "${RED}ERROR: Server failed to start${NC}"
        cat /tmp/nanoserve_test.log
        exit 1
    fi

    print_info "Server started (PID: $SERVER_PID)"
}

# Stop the server
stop_server() {
    if [ ! -z "$SERVER_PID" ]; then
        print_info "Stopping server (PID: $SERVER_PID)..."
        kill -TERM $SERVER_PID 2>/dev/null || true

        # Wait for graceful shutdown (now instant with self-pipe trick)
        sleep 1

        # Force kill if still running (should not be necessary now)
        if ps -p $SERVER_PID > /dev/null 2>&1; then
            print_info "Server did not shutdown gracefully, force killing..."
            kill -9 $SERVER_PID 2>/dev/null || true
        fi

        SERVER_PID=""
        print_info "Server stopped"
    fi
}

# Cleanup on exit
cleanup() {
    stop_server
    rm -f /tmp/nanoserve_test.log
}
trap cleanup EXIT

# Run a test
run_test() {
    ((TESTS_RUN++))
    print_test "$1"
}

###################
# TEST SUITE
###################

echo ""
echo "=========================================="
echo "  NanoServe HTTP Server Test Suite"
echo "  Story 2.3: Request/Response Testing"
echo "=========================================="
echo ""

# Build and start server
start_server

echo ""
echo "=========================================="
echo "  Basic HTTP Tests"
echo "=========================================="
echo ""

# Test 1: Basic GET request
run_test "Basic GET request to /"
RESPONSE=$(curl -s http://localhost:$PORT/)
if echo "$RESPONSE" | grep -q "NanoServe v2.0"; then
    print_pass "Received valid HTML response"
else
    print_fail "Invalid response: $RESPONSE"
fi

# Test 2: GET with different path
run_test "GET request to /test"
RESPONSE=$(curl -s http://localhost:$PORT/test)
if echo "$RESPONSE" | grep -q "NanoServe v2.0"; then
    print_pass "Server responds to different paths"
else
    print_fail "Failed to respond to /test"
fi

# Test 3: HTTP headers check
run_test "HTTP headers validation"
HEADERS=$(curl -s -I http://localhost:$PORT/)
if echo "$HEADERS" | grep -q "HTTP/1.1 200 OK"; then
    print_pass "Correct HTTP status line"
else
    print_fail "Invalid HTTP status line"
fi

if echo "$HEADERS" | grep -q "Content-Type: text/html"; then
    print_pass "Correct Content-Type header"
else
    print_fail "Missing or incorrect Content-Type"
fi

if echo "$HEADERS" | grep -q "Content-Length:"; then
    print_pass "Content-Length header present"
else
    print_fail "Missing Content-Length header"
fi

# Test 4: Multiple sequential requests
run_test "Multiple sequential requests"
SUCCESS=0
for i in {1..5}; do
    RESPONSE=$(curl -s http://localhost:$PORT/request$i)
    if echo "$RESPONSE" | grep -q "NanoServe v2.0"; then
        ((SUCCESS++))
    fi
done
if [ $SUCCESS -eq 5 ]; then
    print_pass "All 5 sequential requests succeeded"
else
    print_fail "Only $SUCCESS/5 requests succeeded"
fi

echo ""
echo "=========================================="
echo "  HTTP Method Tests"
echo "=========================================="
echo ""

# Test 5: POST request
run_test "POST request"
RESPONSE=$(curl -s -X POST -d "test=data" http://localhost:$PORT/submit)
if echo "$RESPONSE" | grep -q "NanoServe v2.0"; then
    print_pass "Server accepts POST requests"
else
    print_fail "Server failed to handle POST"
fi

# Test 6: PUT request
run_test "PUT request"
RESPONSE=$(curl -s -X PUT -d "update=data" http://localhost:$PORT/resource)
if echo "$RESPONSE" | grep -q "NanoServe v2.0"; then
    print_pass "Server accepts PUT requests"
else
    print_fail "Server failed to handle PUT"
fi

# Test 7: DELETE request
run_test "DELETE request"
RESPONSE=$(curl -s -X DELETE http://localhost:$PORT/resource)
if echo "$RESPONSE" | grep -q "NanoServe v2.0"; then
    print_pass "Server accepts DELETE requests"
else
    print_fail "Server failed to handle DELETE"
fi

echo ""
echo "=========================================="
echo "  Edge Case Tests"
echo "=========================================="
echo ""

# Test 8: Large request headers
run_test "Request with large headers"
RESPONSE=$(curl -s -H "X-Large-Header: $(head -c 1000 /dev/zero | tr '\0' 'A')" http://localhost:$PORT/)
if echo "$RESPONSE" | grep -q "NanoServe v2.0"; then
    print_pass "Handles large headers"
else
    print_fail "Failed to handle large headers"
fi

# Test 9: Query parameters
run_test "Request with query parameters"
RESPONSE=$(curl -s "http://localhost:$PORT/search?q=test&page=1&limit=10")
if echo "$RESPONSE" | grep -q "NanoServe v2.0"; then
    print_pass "Handles query parameters"
else
    print_fail "Failed to handle query parameters"
fi

# Test 10: Special characters in URL
run_test "URL with special characters"
RESPONSE=$(curl -s "http://localhost:$PORT/path/with%20spaces/and-dashes_underscores")
if echo "$RESPONSE" | grep -q "NanoServe v2.0"; then
    print_pass "Handles special characters in URL"
else
    print_fail "Failed to handle special characters"
fi

echo ""
echo "=========================================="
echo "  Concurrent Connection Tests"
echo "=========================================="
echo ""

# Test 11: Rapid sequential requests
run_test "Rapid sequential requests (10 requests)"
START_TIME=$(date +%s)
SUCCESS=0
for i in {1..10}; do
    RESPONSE=$(curl -s http://localhost:$PORT/rapid$i)
    if echo "$RESPONSE" | grep -q "NanoServe v2.0"; then
        ((SUCCESS++))
    fi
done
END_TIME=$(date +%s)
DURATION=$((END_TIME - START_TIME))

if [ $SUCCESS -eq 10 ]; then
    print_pass "All 10 rapid requests succeeded in ${DURATION}s"
else
    print_fail "Only $SUCCESS/10 rapid requests succeeded"
fi

# Test 12: Connection timeout/close behavior
run_test "Connection close behavior"
RESPONSE=$(curl -s -v http://localhost:$PORT/ 2>&1)
if echo "$RESPONSE" | grep -q "Connection.*close"; then
    print_pass "Server properly closes connections"
else
    print_fail "Connection close behavior unclear"
fi

echo ""
echo "=========================================="
echo "  Malformed Request Tests"
echo "=========================================="
echo ""

# Test 13: Invalid HTTP version
run_test "Invalid HTTP version"
RESPONSE=$(echo -e "GET / HTTP/2.0\r\n\r\n" | nc localhost $PORT)
if [ ! -z "$RESPONSE" ]; then
    print_pass "Server responds to invalid HTTP version"
else
    print_fail "Server doesn't respond to invalid HTTP version"
fi

# Test 14: Missing Host header (HTTP/1.1 requires it, but we're lenient)
run_test "Request without Host header"
RESPONSE=$(echo -e "GET / HTTP/1.1\r\n\r\n" | nc localhost $PORT)
if [ ! -z "$RESPONSE" ]; then
    print_pass "Server handles requests without Host header"
else
    print_fail "Server requires Host header"
fi

# Test 15: Empty request
run_test "Empty request (just newlines)"
RESPONSE=$(echo -e "\r\n\r\n" | nc localhost $PORT)
if [ ! -z "$RESPONSE" ]; then
    print_pass "Server responds to empty request"
else
    print_fail "Server doesn't handle empty request"
fi

echo ""
echo "=========================================="
echo "  Server Logs Analysis"
echo "=========================================="
echo ""

# Check server logs for errors
run_test "Server logs for errors"
ERROR_COUNT=$(grep -c "\[ERROR\]" /tmp/nanoserve_test.log 2>/dev/null || echo "0")
ERROR_COUNT=$(echo "$ERROR_COUNT" | tr -d '\n\r' | xargs)
if [ "$ERROR_COUNT" -eq 0 ]; then
    print_pass "No errors in server logs"
else
    print_fail "Found $ERROR_COUNT errors in server logs"
    echo "Sample errors:"
    grep "\[ERROR\]" /tmp/nanoserve_test.log | head -3
fi

# Check for successful request handling
run_test "Request logging verification"
REQUEST_COUNT=$(grep -c "Request:" /tmp/nanoserve_test.log || echo "0")
if [ "$REQUEST_COUNT" -gt 0 ]; then
    print_pass "Server logged $REQUEST_COUNT requests"
else
    print_fail "No requests found in logs"
fi

echo ""
echo "=========================================="
echo "  Test Summary"
echo "=========================================="
echo ""

echo "Total Tests Run:    $TESTS_RUN"
echo -e "Tests Passed:       ${GREEN}$TESTS_PASSED${NC}"
echo -e "Tests Failed:       ${RED}$TESTS_FAILED${NC}"
echo ""

if [ $TESTS_FAILED -eq 0 ]; then
    echo -e "${GREEN}✓ All tests passed!${NC}"
    echo ""
    exit 0
else
    echo -e "${RED}✗ Some tests failed${NC}"
    echo ""
    echo "Server log output:"
    echo "=================="
    tail -20 /tmp/nanoserve_test.log
    echo ""
    exit 1
fi
