#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>     // fork(), pipe(), dup2(), execvp()
#include <sys/wait.h>   // wait()
#include "dshlib.h"

#define MAX_ARGS 32  // Maximum number of arguments for a command

int main() {
    char cmd_buff[SH_CMD_MAX];
    command_list_t clist;

    while (1) {
        // Print the shell prompt
        printf("%s", SH_PROMPT);
        fflush(stdout);

        // Read user input (EOF is handled for headless testing)
        if (fgets(cmd_buff, ARG_MAX, stdin) == NULL) {
            printf("\n");
            break;
        }
        // Remove trailing newline
        cmd_buff[strcspn(cmd_buff, "\n")] = '\0';

        // Handle built-in commands: exit and dragon
        if (strcmp(cmd_buff, EXIT_CMD) == 0) {
            exit(OK);
        }
        if (strcmp(cmd_buff, "dragon") == 0) {
            print_dragon();
            continue;
        }
        // Check for empty input
        if (strlen(cmd_buff) == 0) {
            printf(CMD_WARN_NO_CMD);
            continue;
        }

        // Parse the command line into a command list
        int rc = build_cmd_list(cmd_buff, &clist);
        if (rc != OK) {
            continue;
        }

        // If only one command, no piping is required.
        if (clist.num == 1) {
            pid_t pid = fork();
            if (pid < 0) {
                perror("fork");
                exit(EXIT_FAILURE);
            } else if (pid == 0) {
                // Child: build argv array for the command
                char *argv[MAX_ARGS];
                int argCount = 0;
                argv[argCount++] = clist.commands[0].exe;

                // Tokenize the arguments string
                char *args_copy = strdup(clist.commands[0].args);
                if (args_copy) {
                    char *token = strtok(args_copy, " ");
                    while (token != NULL && argCount < MAX_ARGS - 1) {
                        argv[argCount++] = token;
                        token = strtok(NULL, " ");
                    }
                    free(args_copy);
                }
                argv[argCount] = NULL;

                // Execute the external command
                execvp(argv[0], argv);
                perror("execvp");
                exit(EXIT_FAILURE);
            } else {
                // Parent: wait for the child process to finish
                wait(NULL);
            }
        }
        // More than one command: set up pipeline
        else {
            int num_pipes = clist.num - 1;
            int pipes[num_pipes][2];

            // Create the required pipes
            for (int i = 0; i < num_pipes; i++) {
                if (pipe(pipes[i]) < 0) {
                    perror("pipe");
                    exit(EXIT_FAILURE);
                }
            }

            // Fork one child process per command
            for (int i = 0; i < clist.num; i++) {
                pid_t pid = fork();
                if (pid < 0) {
                    perror("fork");
                    exit(EXIT_FAILURE);
                } else if (pid == 0) {
                    // If not the first command, set stdin to the previous pipe’s read end
                    if (i > 0) {
                        if (dup2(pipes[i - 1][0], STDIN_FILENO) < 0) {
                            perror("dup2 (stdin)");
                            exit(EXIT_FAILURE);
                        }
                    }
                    // If not the last command, set stdout to the current pipe’s write end
                    if (i < clist.num - 1) {
                        if (dup2(pipes[i][1], STDOUT_FILENO) < 0) {
                            perror("dup2 (stdout)");
                            exit(EXIT_FAILURE);
                        }
                    }
                    // Close all pipe file descriptors in the child
                    for (int j = 0; j < num_pipes; j++) {
                        close(pipes[j][0]);
                        close(pipes[j][1]);
                    }

                    // Build argv array for the command
                    char *argv[MAX_ARGS];
                    int argCount = 0;
                    argv[argCount++] = clist.commands[i].exe;
                    char *args_copy = strdup(clist.commands[i].args);
                    if (args_copy) {
                        char *token = strtok(args_copy, " ");
                        while (token != NULL && argCount < MAX_ARGS - 1) {
                            argv[argCount++] = token;
                            token = strtok(NULL, " ");
                        }
                        free(args_copy);
                    }
                    argv[argCount] = NULL;

                    // Execute the command
                    execvp(argv[0], argv);
                    perror("execvp");
                    exit(EXIT_FAILURE);
                }
                // Parent continues to fork next command
            }

            // Parent: close all pipe file descriptors
            for (int i = 0; i < num_pipes; i++) {
                close(pipes[i][0]);
                close(pipes[i][1]);
            }
            // Wait for all child processes to finish
            for (int i = 0; i < clist.num; i++) {
                wait(NULL);
            }
        }
    }
    return OK;
}
