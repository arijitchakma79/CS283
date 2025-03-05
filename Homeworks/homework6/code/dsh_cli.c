#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>     // fork(), dup2()
#include <sys/wait.h>   // wait()
#include "dshlib.h"

#define MAX_ARGS 32

// Proper declaration of print_dragon function
extern void print_dragon(void);
/*
 * build_argv
 *
 * Splits 'exe' + 'args' into an argv array for execvp.
 * Preserves trailing spaces inside quotes.
 */
void build_argv(const char *exe, const char *args, char **argv, int *argCount) {
    *argCount = 0;
    // First element is always the executable
    argv[(*argCount)++] = strdup(exe);

    // If no args, terminate argv
    if (!args || !*args) {
        argv[*argCount] = NULL;
        return;
    }

    // Copy args so we can modify it
    char *args_copy = strdup(args);
    char *p = args_copy;
    char *start = p;
    int in_quotes = 0;

    while (*p) {
        if (*p == '"') {
            // Toggle in_quotes
            if (!in_quotes) {
                // Start of quoted section (skip the quote)
                start = p + 1;
            } else {
                // End of quoted section
                *p = '\0'; // terminate the token
                if (p > start) {
                    argv[(*argCount)++] = strdup(start);
                }
                start = p + 1; // move past the closing quote
            }
            in_quotes = !in_quotes;
        }
        else if (*p == ' ' && !in_quotes) {
            // End of an unquoted token
            *p = '\0';
            if (p > start) {
                argv[(*argCount)++] = strdup(start);
            }
            start = p + 1;
        }
        p++;
    }
    // Last token
    if (*start) {
        argv[(*argCount)++] = strdup(start);
    }
    argv[*argCount] = NULL;
    free(args_copy);
}


int main() {
    char cmd_buff[SH_CMD_MAX];
    command_list_t clist;

    // Print the initial prompt
    printf("dsh3> ");
    fflush(stdout);

    // Main command loop
    while (1) {
        // Read next command
        if (!fgets(cmd_buff, ARG_MAX, stdin)) {
            // EOF => print one final prompt, then exit
            printf("dsh3> ");
            fflush(stdout);
            break;
        }
        // Strip trailing newline
        cmd_buff[strcspn(cmd_buff, "\n")] = '\0';

        // Check built-ins
        if (strcmp(cmd_buff, EXIT_CMD) == 0) {
            printf("exiting...\n");
            break; // exit the shell
        }
        if (strcmp(cmd_buff, "dragon") == 0) {
            print_dragon();
            // Print prompt again
            printf("dsh3> ");
            fflush(stdout);
            continue;
        }
        // Empty input
        if (strlen(cmd_buff) == 0) {
            printf("%s", CMD_WARN_NO_CMD);
            printf("dsh3> ");
            fflush(stdout);
            continue;
        }

        // Parse the command line into clist
        int rc = build_cmd_list(cmd_buff, &clist);
        if (rc != OK) {
            // If parse error or no commands, print prompt and continue
            printf("dsh3> ");
            fflush(stdout);
            continue;
        }

        // Single command
        if (clist.num == 1) {
            pid_t pid = fork();
            if (pid < 0) {
                perror("fork");
                exit(EXIT_FAILURE);
            } else if (pid == 0) {
                // Child - use the argv directly from cmd_buff_t
                execvp(clist.commands[0].argv[0], clist.commands[0].argv);
                perror("execvp");
                exit(EXIT_FAILURE);
            } else {
                // Parent
                wait(NULL);
            }
        }
        // Pipeline
        else {
            int num_pipes = clist.num - 1;
            int pipes[num_pipes][2];

            // Create all pipes
            for (int i = 0; i < num_pipes; i++) {
                if (pipe(pipes[i]) < 0) {
                    perror("pipe");
                    exit(EXIT_FAILURE);
                }
            }

            // Fork for each command
            for (int i = 0; i < clist.num; i++) {
                pid_t pid = fork();
                if (pid < 0) {
                    perror("fork");
                    exit(EXIT_FAILURE);
                } else if (pid == 0) {
                    // Child: set up stdin from previous pipe if not the first command
                    if (i > 0) {
                        dup2(pipes[i - 1][0], STDIN_FILENO);
                    }
                    // Set up stdout to next pipe if not the last command
                    if (i < clist.num - 1) {
                        dup2(pipes[i][1], STDOUT_FILENO);
                    }
                    // Close all pipe fds
                    for (int j = 0; j < num_pipes; j++) {
                        close(pipes[j][0]);
                        close(pipes[j][1]);
                    }
                    // Exec - use the argv directly from cmd_buff_t
                    execvp(clist.commands[i].argv[0], clist.commands[i].argv);
                    perror("execvp");
                    exit(EXIT_FAILURE);
                }
            }
            // Parent: close all pipes
            for (int i = 0; i < num_pipes; i++) {
                close(pipes[i][0]);
                close(pipes[i][1]);
            }
            // Wait for children
            for (int i = 0; i < clist.num; i++) {
                wait(NULL);
            }
        }

        // After executing the command, print the prompt again
        printf("dsh3> ");
        fflush(stdout);
    }

    // Print the final message 
    printf("cmd loop returned 0");
    return 0;
}