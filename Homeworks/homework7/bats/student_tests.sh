#!/usr/bin/env bats
# File: student_tests.sh
#
# BATS test suite for Drexel Shell with remote functionality

# Helper function to create a temporary file
setup() {
  TEST_TEMP_DIR="$(mktemp -d)"
  TEST_FILE="${TEST_TEMP_DIR}/test_file.txt"
  echo "This is a test file content" > "$TEST_FILE"
  
  # Start server for remote shell tests
  ./dsh -s -p 8888 &
  SERVER_PID=$!
  # Give server time to start
  sleep 1
}

# Clean up after tests
teardown() {
  rm -rf "$TEST_TEMP_DIR"
  
  # Kill server if it's running
  if [[ -n "$SERVER_PID" ]]; then
    kill $SERVER_PID 2>/dev/null || true
    wait $SERVER_PID 2>/dev/null || true
  fi
}

# ---- Local Shell Tests ----

@test "Local: basic command execution" {
  run bash -c 'echo "ls" | ./dsh'
  [ "$status" -eq 0 ]
  [[ "$output" == *"dsh"* ]]
}

@test "Local: exit command" {
  run bash -c 'echo "exit" | ./dsh'
  [ "$status" -eq 0 ]
  [[ "$output" == *"exiting"* ]]
}

@test "Local: empty command" {
  run bash -c 'echo "" | ./dsh'
  [ "$status" -eq 0 ]
  [[ "$output" == *"warning: no commands provided"* ]]
}

@test "Local: pipe command execution" {
  run bash -c 'echo "ls | grep .c" | ./dsh'
  [ "$status" -eq 0 ]
  [[ "$output" == *".c"* ]]
}

@test "Local: built-in cd command" {
  run bash -c 'echo -e "cd '"$TEST_TEMP_DIR"'\npwd\nexit" | ./dsh'
  [ "$status" -eq 0 ]
  [[ "$output" == *"$TEST_TEMP_DIR"* ]]
}

@test "Local: multiple commands" {
  run bash -c 'echo -e "echo hello\necho world\nexit" | ./dsh'
  [ "$status" -eq 0 ]
  [[ "$output" == *"hello"* ]]
  [[ "$output" == *"world"* ]]
}

@test "Local: command not found" {
  run bash -c 'echo "nonexistentcommand" | ./dsh'
  [ "$status" -eq 0 ]
  [[ "$output" == *"nonexistentcommand"* ]]
}

# ---- Remote Client-Server Tests ----

@test "Remote: basic command execution" {
  run bash -c 'echo "ls" | ./dsh -c -p 8888'
  [ "$status" -eq 0 ]
  [[ "$output" == *"dsh"* ]]
}

@test "Remote: exit command" {
  run bash -c 'echo "exit" | ./dsh -c -p 8888'
  [ "$status" -eq 0 ]
}

@test "Remote: empty command" {
  run bash -c 'echo "" | ./dsh -c -p 8888'
  [ "$status" -eq 0 ]
}

@test "Remote: pipe command execution" {
  run bash -c 'echo "ls | grep .c" | ./dsh -c -p 8888'
  [ "$status" -eq 0 ]
  [[ "$output" == *".c"* ]]
}

@test "Remote: built-in cd command" {
  run bash -c 'echo -e "cd '"$TEST_TEMP_DIR"'\npwd\nexit" | ./dsh -c -p 8888'
  [ "$status" -eq 0 ]
  [[ "$output" == *"$TEST_TEMP_DIR"* ]]
}

@test "Remote: server shutdown and restart" {
  # Stop the server with stop-server command
  run bash -c 'echo "stop-server" | ./dsh -c -p 8888'
  [ "$status" -eq 0 ]
  [[ "$output" == *"Stopping server"* ]]
  
  sleep 1
  
  # Start a new server
  ./dsh -s -p 8888 &
  SERVER_PID=$!
  sleep 1
  
  # Verify new server works
  run bash -c 'echo "echo server_restarted" | ./dsh -c -p 8888'
  [ "$status" -eq 0 ]
  [[ "$output" == *"server_restarted"* ]]
}

# ---- Error Handling Tests ----

@test "Error: too many piped commands" {
  # Create a command with more pipes than CMD_MAX (8)
  LONG_PIPE="echo test | cat | grep test | cat | grep test | cat | grep test | cat | grep test | cat"
  run bash -c 'echo "'"$LONG_PIPE"'" | ./dsh'
  [ "$status" -eq 0 ]
  [[ "$output" == *"error: piping limited"* ]]
}

# ---- Extra Credit Tests (uncomment if implemented) ----

# @test "Extra Credit: input redirection" {
#   run bash -c 'echo "cat < '"$TEST_FILE"'" | ./dsh'
#   [ "$status" -eq 0 ]
#   [[ "$output" == *"This is a test file content"* ]]
# }

# @test "Extra Credit: output redirection" {
#   OUTPUT_FILE="${TEST_TEMP_DIR}/output.txt"
#   run bash -c 'echo "echo redirected output > '"$OUTPUT_FILE"'" | ./dsh'
#   [ "$status" -eq 0 ]
#   [ -f "$OUTPUT_FILE" ]
#   [[ "$(cat "$OUTPUT_FILE")" == "redirected output" ]]
# }

# @test "Extra Credit: append redirection" {
#   APPEND_FILE="${TEST_TEMP_DIR}/append.txt"
#   echo "first line" > "$APPEND_FILE"
#   run bash -c 'echo "echo second line >> '"$APPEND_FILE"'" | ./dsh'
#   [ "$status" -eq 0 ]
#   [[ "$(cat "$APPEND_FILE")" == *"first line"* ]]
#   [[ "$(cat "$APPEND_FILE")" == *"second line"* ]]
# }

# @test "Extra Credit: multi-threaded server connections" {
#   # Start a long-running command on one client
#   bash -c 'echo "sleep 5" | ./dsh -c -p 8888' &
#   FIRST_CLIENT_PID=$!
#   sleep 1
#   
#   # Connect with a second client while first is still running
#   run bash -c 'echo "echo concurrent_connection" | ./dsh -c -p 8888'
#   
#   # Wait for first client to finish
#   wait $FIRST_CLIENT_PID
#   
#   # Check that second client was successful
#   [ "$status" -eq 0 ]
#   [[ "$output" == *"concurrent_connection"* ]]
# }