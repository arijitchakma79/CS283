#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include "dshlib.h"

#define MAX_ARGS 32

void build_argv(const char *exe, const char *args, char **argv, int *argCount) {
    *argCount = 0;
    argv[(*argCount)++] = strdup(exe);  // Make a copy of exe
    
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
        printf("%s", SH_PROMPT);
        fflush(stdout);

        if (fgets(cmd_buff, ARG_MAX, stdin) == NULL) {
            printf("\n");
            break;
        }
        cmd_buff[strcspn(cmd_buff, "\n")] = '\0';

        if (strcmp(cmd_buff, EXIT_CMD) == 0) {
            exit(OK);
        }
        if (strcmp(cmd_buff, "dragon") == 0) {
            print_dragon();
            continue;
        }
        if (strlen(cmd_buff) == 0) {
            printf(CMD_WARN_NO_CMD);
            continue;
        }

        int rc = build_cmd_list(cmd_buff, &clist);
        if (rc != OK) {
            continue;
        }

        if (clist.num == 1) {
            pid_t pid = fork();
            if (pid < 0) {
                perror("fork");
                exit(EXIT_FAILURE);
            } else if (pid == 0) {
                char *argv[MAX_ARGS];
                int argCount;
                build_argv(clist.commands[0].exe, clist.commands[0].args, argv, &argCount);
                execvp(argv[0], argv);
                perror("execvp");
                exit(EXIT_FAILURE);
            } else {
                wait(NULL);
            }
        } else {
            int num_pipes = clist.num - 1;
            int pipes[num_pipes][2];

            for (int i = 0; i < num_pipes; i++) {
                if (pipe(pipes[i]) < 0) {
                    perror("pipe");
                    exit(EXIT_FAILURE);
                }
            }

            for (int i = 0; i < clist.num; i++) {
                pid_t pid = fork();
                if (pid < 0) {
                    perror("fork");
                    exit(EXIT_FAILURE);
                } else if (pid == 0) {
                    if (i > 0) {
                        dup2(pipes[i - 1][0], STDIN_FILENO);
                    }
                    if (i < clist.num - 1) {
                        dup2(pipes[i][1], STDOUT_FILENO);
                    }

                    for (int j = 0; j < num_pipes; j++) {
                        close(pipes[j][0]);
                        close(pipes[j][1]);
                    }

                    char *argv[MAX_ARGS];
                    int argCount;
                    build_argv(clist.commands[i].exe, clist.commands[i].args, argv, &argCount);
                    execvp(argv[0], argv);
                    perror("execvp");
                    exit(EXIT_FAILURE);
                }
            }

            for (int i = 0; i < num_pipes; i++) {
                close(pipes[i][0]);
                close(pipes[i][1]);
            }

            for (int i = 0; i < clist.num; i++) {
                wait(NULL);
            }
        }
    }
    return OK;
}