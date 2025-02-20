#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include "dshlib.h"

#define MAX_ARGS 32

void build_argv(const char *exe, const char *args, char **argv, int *argCount) {
    *argCount = 0;
    argv[(*argCount)++] = strdup(exe);
    
    if (!args || !*args) {
        argv[*argCount] = NULL;
        return;
    }

    char *args_copy = strdup(args);
    char *p = args_copy;
    char *start = p;
    int in_quotes = 0;
    
    while (*p) {
        if (*p == '"') {
            if (!in_quotes) {
                start = p + 1;
            } else {
                *p = '\0';
                if (p > start) {
                    argv[(*argCount)++] = strdup(start);
                }
                start = p + 1;
            }
            in_quotes = !in_quotes;
        } else if (*p == ' ' && !in_quotes) {
            *p = '\0';
            if (p > start) {
                argv[(*argCount)++] = strdup(start);
            }
            start = p + 1;
        }
        p++;
    }
    
    if (*start) {
        argv[(*argCount)++] = strdup(start);
    }
    
    argv[*argCount] = NULL;
    free(args_copy);
}

int main() {
    char cmd_buff[SH_CMD_MAX];
    command_list_t clist;

    while (1) {
        printf("dsh2> ");  // Initial prompt
        fflush(stdout);

        if (fgets(cmd_buff, ARG_MAX, stdin) == NULL) {
            break;
        }
        cmd_buff[strcspn(cmd_buff, "\n")] = '\0';

        if (strcmp(cmd_buff, EXIT_CMD) == 0) {
            break;
        }
        if (strcmp(cmd_buff, "dragon") == 0) {
            print_dragon();
            continue;
        }
        if (strlen(cmd_buff) == 0) {
            printf("%s", CMD_WARN_NO_CMD);
            continue;
        }

        int rc = build_cmd_list(cmd_buff, &clist);
        if (rc != OK) {
            continue;
        }

        if (clist.num == 1) {
            int pipefd[2];
            if (pipe(pipefd) == -1) {
                perror("pipe");
                exit(EXIT_FAILURE);
            }

            pid_t pid = fork();
            if (pid < 0) {
                perror("fork");
                exit(EXIT_FAILURE);
            } else if (pid == 0) {
                close(pipefd[0]);
                dup2(pipefd[1], STDOUT_FILENO);
                close(pipefd[1]);

                char *argv[MAX_ARGS];
                int argCount;
                build_argv(clist.commands[0].exe, clist.commands[0].args, argv, &argCount);
                execvp(argv[0], argv);
                perror("execvp");
                exit(EXIT_FAILURE);
            } else {
                close(pipefd[1]);
                
                char buffer[4096];
                ssize_t count = read(pipefd[0], buffer, sizeof(buffer)-1);
                if (count > 0) {
                    buffer[count] = '\0';
                    if (count > 0 && buffer[count-1] == '\n') {
                        buffer[count-1] = '\0';
                    }
                    printf("%s", buffer);
                }
                close(pipefd[0]);
                wait(NULL);
            }
        } else {
            int num_pipes = clist.num - 1;
            int pipes[num_pipes][2];

            // Create all needed pipes
            for (int i = 0; i < num_pipes; i++) {
                if (pipe(pipes[i]) == -1) {
                    perror("pipe");
                    exit(EXIT_FAILURE);
                }
            }

            // Create a pipe for capturing final output
            int final_pipe[2];
            if (pipe(final_pipe) == -1) {
                perror("pipe");
                exit(EXIT_FAILURE);
            }

            // Fork for each command
            for (int i = 0; i < clist.num; i++) {
                pid_t pid = fork();
                if (pid < 0) {
                    perror("fork");
                    exit(EXIT_FAILURE);
                } else if (pid == 0) {
                    // Set up input from previous pipe if not first command
                    if (i > 0) {
                        dup2(pipes[i - 1][0], STDIN_FILENO);
                    }

                    // Set up output to next pipe if not last command
                    if (i < clist.num - 1) {
                        dup2(pipes[i][1], STDOUT_FILENO);
                    } else {
                        // Last command outputs to final pipe
                        dup2(final_pipe[1], STDOUT_FILENO);
                    }

                    // Close all pipe fds in child
                    for (int j = 0; j < num_pipes; j++) {
                        close(pipes[j][0]);
                        close(pipes[j][1]);
                    }
                    close(final_pipe[0]);
                    close(final_pipe[1]);

                    char *argv[MAX_ARGS];
                    int argCount;
                    build_argv(clist.commands[i].exe, clist.commands[i].args, argv, &argCount);
                    execvp(argv[0], argv);
                    perror("execvp");
                    exit(EXIT_FAILURE);
                }
            }

            // Parent: close all pipe fds
            for (int i = 0; i < num_pipes; i++) {
                close(pipes[i][0]);
                close(pipes[i][1]);
            }
            close(final_pipe[1]);

            // Read final output
            char buffer[4096];
            ssize_t count = read(final_pipe[0], buffer, sizeof(buffer)-1);
            if (count > 0) {
                buffer[count] = '\0';
                if (count > 0 && buffer[count-1] == '\n') {
                    buffer[count-1] = '\0';
                }
                printf("%s", buffer);
            }
            close(final_pipe[0]);

            // Wait for all children
            for (int i = 0; i < clist.num; i++) {
                wait(NULL);
            }
        }
        printf("dsh2> ");
        fflush(stdout);
    }
    printf("cmd loop returned 0");
    return OK;
}
