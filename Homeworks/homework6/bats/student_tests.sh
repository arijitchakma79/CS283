#!/usr/bin/env bats
# File: student_tests.sh
# 
# Create your unit tests suit in this file

# Basic command execution test
@test "Example: check ls runs without errors" {
    run ./dsh <<EOF                
ls
EOF
    # Assertions
    [ "$status" -eq 0 ]
}

# Test basic pipe functionality
@test "Basic pipe: ls | grep c" {
    run ./dsh <<EOF
ls | grep c
EOF
    # Should find at least the .c files
    [[ "$output" == *".c"* ]]
    [ "$status" -eq 0 ]
}

# Test multiple pipes
@test "Multiple pipes: ls | grep c | wc -l" {
    run ./dsh <<EOF
ls | grep c | wc -l
EOF
    # Should show a number (count of files with 'c')
    [[ "$output" =~ [0-9] ]]
    [ "$status" -eq 0 ]
}

# Test built-in exit command
@test "Built-in: exit command" {
    run ./dsh <<EOF
exit
EOF
    # Check for exit message
    [[ "$output" == *"exiting"* ]]
    [ "$status" -eq 0 ]
}

# Test empty input
@test "Empty input handling" {
    run ./dsh <<EOF

EOF
    # Should warn about no command
    [[ "$output" == *"warning: no commands provided"* ]]
    [ "$status" -eq 0 ]
}

# Test whitespace handling in commands
@test "Whitespace handling in piped commands" {
    run ./dsh <<EOF
  ls   |   grep c   
EOF
    # Should handle extra whitespace correctly
    [[ "$output" == *".c"* ]]
    [ "$status" -eq 0 ]
}

# Test built-in dragon command
@test "Built-in: dragon command" {
    run ./dsh <<EOF
dragon
EOF
    # Should contain some pattern from the dragon art
    [[ "$output" == *"%"* ]]
    [ "$status" -eq 0 ]
}

# Test executing multiple commands
@test "Multiple commands execution" {
    run ./dsh <<EOF
ls
dragon
exit
EOF
    # Output should contain files and dragon art
    [[ "$output" == *".c"* ]]
    [[ "$output" == *"%"* ]]
    [[ "$output" == *"exiting"* ]]
    [ "$status" -eq 0 ]
}

# Test command not found
@test "Error handling: command not found" {
    run ./dsh <<EOF
nonexistentcommand
EOF
    # Should show error about command not found but continue
    [[ "$output" == *"No such file or directory"* ]] || [[ "$output" == *"not found"* ]]
    [ "$status" -eq 0 ]
}

# Test piping with a non-existent command
@test "Error handling: pipe with non-existent command" {
    run ./dsh <<EOF
ls | nonexistentcommand
EOF
    # Should show error but continue
    [[ "$output" == *"No such file or directory"* ]] || [[ "$output" == *"not found"* ]]
    [ "$status" -eq 0 ]
}

# ===== EXTRA CREDIT TESTS =====

# Test output redirection
@test "Extra Credit: Output redirection with >" {
    # First create a test file
    run ./dsh <<EOF
echo "test output redirection" > test_out.txt
cat test_out.txt
EOF
    # Check if the file was created and has correct content
    [[ "$output" == *"test output redirection"* ]]
    [ "$status" -eq 0 ]
    
    # Clean up
    rm -f test_out.txt
}

# Test input redirection
@test "Extra Credit: Input redirection with <" {
    # First create a test file
    echo "test input redirection" > test_in.txt
    
    run ./dsh <<EOF
cat < test_in.txt
EOF
    # Check if the content was read correctly
    [[ "$output" == *"test input redirection"* ]]
    [ "$status" -eq 0 ]
    
    # Clean up
    rm -f test_in.txt
}

# Test append redirection
@test "Extra Credit++: Append redirection with >>" {
    # First create a test file
    run ./dsh <<EOF
echo "line 1" > test_append.txt
echo "line 2" >> test_append.txt
cat test_append.txt
EOF
    # Check if both lines are in the output
    [[ "$output" == *"line 1"* ]]
    [[ "$output" == *"line 2"* ]]
    [ "$status" -eq 0 ]
    
    # Clean up
    rm -f test_append.txt
}

# Test pipe with redirection
@test "Extra Credit: Pipe with redirection" {
    run ./dsh <<EOF
ls | grep c > pipe_out.txt
cat pipe_out.txt
EOF
    # Should have .c files in the output
    [[ "$output" == *".c"* ]]
    [ "$status" -eq 0 ]
    
    # Clean up
    rm -f pipe_out.txt
}

# Test complex redirection
@test "Extra Credit: Complex redirection" {
    # Create input file
    echo "This is a test file with the word dragon in it." > complex_in.txt
    
    run ./dsh <<EOF
cat < complex_in.txt | grep dragon > complex_out.txt
cat complex_out.txt
EOF
    # Check if the filtered content was saved correctly
    [[ "$output" == *"dragon"* ]]
    [ "$status" -eq 0 ]
    
    # Clean up
    rm -f complex_in.txt complex_out.txt
}