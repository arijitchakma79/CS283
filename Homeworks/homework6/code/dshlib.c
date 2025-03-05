#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>  // For open(), O_* constants
#include "dshlib.h"


extern void print_dragon(void);
/*
 * alloc_cmd_buff
 *
 * Allocates memory for the command buffer and initializes it
 */
int alloc_cmd_buff(cmd_buff_t *cmd_buff) {
    cmd_buff->argc = 0;
    cmd_buff->_cmd_buffer = NULL;
    
    // Initialize argv array to NULL
    for (int i = 0; i < CMD_ARGV_MAX; i++) {
        cmd_buff->argv[i] = NULL;
    }
    
    // Initialize redirection fields
    cmd_buff->in_redir_type = REDIR_NONE;
    cmd_buff->in_redir_file = NULL;
    cmd_buff->out_redir_type = REDIR_NONE;
    cmd_buff->out_redir_file = NULL;
    
    return OK;
}

/*
 * free_cmd_buff
 *
 * Frees memory allocated for the command buffer
 */
int free_cmd_buff(cmd_buff_t *cmd_buff) {
    if (cmd_buff->_cmd_buffer) {
        free(cmd_buff->_cmd_buffer);
        cmd_buff->_cmd_buffer = NULL;
    }
    
    for (int i = 0; i < cmd_buff->argc; i++) {
        cmd_buff->argv[i] = NULL;
    }
    
    cmd_buff->argc = 0;
    return OK;
}

/*
 * clear_cmd_buff
 *
 * Clears the command buffer for reuse
 */
int clear_cmd_buff(cmd_buff_t *cmd_buff) {
    cmd_buff->argc = 0;
    if (cmd_buff->_cmd_buffer) {
        cmd_buff->_cmd_buffer[0] = '\0';
    }
    
    for (int i = 0; i < CMD_ARGV_MAX; i++) {
        cmd_buff->argv[i] = NULL;
    }
    
    return OK;
}

/*
 * build_cmd_buff
 *
 * Builds a command buffer from a command line
 * Handles redirection operators for extra credit
 */
int build_cmd_buff(char *cmd_line, cmd_buff_t *cmd_buff) {
    size_t cmd_len = strlen(cmd_line);
    
    // Initialize redirection fields
    cmd_buff->in_redir_type = REDIR_NONE;
    cmd_buff->in_redir_file = NULL;
    cmd_buff->out_redir_type = REDIR_NONE;
    cmd_buff->out_redir_file = NULL;
    
    // Allocate memory for the command buffer
    cmd_buff->_cmd_buffer = (char *)malloc(cmd_len + 1);
    if (!cmd_buff->_cmd_buffer) {
        return ERR_MEMORY;
    }
    
    // Copy the command line to the buffer
    strcpy(cmd_buff->_cmd_buffer, cmd_line);
    
    // Handle redirection (for extra credit)
    char *in_redir = strchr(cmd_buff->_cmd_buffer, REDIR_IN_CHAR);
    char *out_redir = strchr(cmd_buff->_cmd_buffer, REDIR_OUT_CHAR);
    char *append_redir = strstr(cmd_buff->_cmd_buffer, ">>");
    
    // Check for append redirection first (since it contains '>')
    if (append_redir) {
        *append_redir = '\0';  // Terminate string at '>>'
        append_redir += 2;     // Move past '>>'
        
        // Skip leading spaces
        while (*append_redir && isspace(*append_redir)) {
            append_redir++;
        }
        
        if (*append_redir) {
            cmd_buff->out_redir_type = REDIR_APPEND;
            cmd_buff->out_redir_file = append_redir;
            
            // Remove trailing spaces from the filename
            char *end = append_redir + strlen(append_redir) - 1;
            while (end > append_redir && isspace(*end)) {
                *end = '\0';
                end--;
            }
        } else {
            printf(CMD_ERR_REDIR);
            return ERR_CMD_ARGS_BAD;
        }
        
        // Reset out_redir since we handled append
        out_redir = NULL;
    }
    
    // Handle output redirection
    if (out_redir && !append_redir) {
        *out_redir = '\0';  // Terminate string at '>'
        out_redir++;        // Move past '>'
        
        // Skip leading spaces
        while (*out_redir && isspace(*out_redir)) {
            out_redir++;
        }
        
        if (*out_redir) {
            cmd_buff->out_redir_type = REDIR_OUT;
            cmd_buff->out_redir_file = out_redir;
            
            // Remove trailing spaces from the filename
            char *end = out_redir + strlen(out_redir) - 1;
            while (end > out_redir && isspace(*end)) {
                *end = '\0';
                end--;
            }
        } else {
            printf(CMD_ERR_REDIR);
            return ERR_CMD_ARGS_BAD;
        }
    }
    
    // Handle input redirection
    if (in_redir) {
        *in_redir = '\0';  // Terminate string at '<'
        in_redir++;        // Move past '<'
        
        // Skip leading spaces
        while (*in_redir && isspace(*in_redir)) {
            in_redir++;
        }
        
        if (*in_redir) {
            cmd_buff->in_redir_type = REDIR_IN;
            cmd_buff->in_redir_file = in_redir;
            
            // Remove trailing spaces from the filename
            char *end = in_redir + strlen(in_redir) - 1;
            while (end > in_redir && isspace(*end)) {
                *end = '\0';
                end--;
            }
        } else {
            printf(CMD_ERR_REDIR);
            return ERR_CMD_ARGS_BAD;
        }
    }
    
    // Tokenize the command line (without redirection parts)
    char *token = strtok(cmd_buff->_cmd_buffer, " \t");
    while (token != NULL && cmd_buff->argc < CMD_ARGV_MAX - 1) {
        cmd_buff->argv[cmd_buff->argc++] = token;
        token = strtok(NULL, " \t");
    }
    
    // Ensure the last element of argv is NULL
    cmd_buff->argv[cmd_buff->argc] = NULL;
    
    return OK;
}

/*
 * close_cmd_buff
 *
 * Closes and frees the command buffer
 */
int close_cmd_buff(cmd_buff_t *cmd_buff) {
    return free_cmd_buff(cmd_buff);
}

/*
 * build_cmd_list
 *
 * Builds a command list from a command line split by pipes
 */
int build_cmd_list(char *cmd_line, command_list_t *clist) {
    clist->num = 0;
    
    // Handle empty input
    if (!cmd_line || strlen(cmd_line) == 0) {
        printf("%s", CMD_WARN_NO_CMD);
        return WARN_NO_CMDS;
    }
    
    // Make a copy of the command line
    char *cmd_copy = strdup(cmd_line);
    if (!cmd_copy) {
        return ERR_MEMORY;
    }
    
    // Split the command line by pipes
    char *cmd_ptr = cmd_copy;
    char *pipe_pos;
    
    while (cmd_ptr && *cmd_ptr) {
        // Check if we've reached the maximum number of commands
        if (clist->num >= CMD_MAX) {
            printf(CMD_ERR_PIPE_LIMIT, CMD_MAX);
            free(cmd_copy);
            return ERR_TOO_MANY_COMMANDS;
        }
        
        // Find the next pipe
        pipe_pos = strchr(cmd_ptr, PIPE_CHAR);
        if (pipe_pos) {
            *pipe_pos = '\0';
        }
        
        // Skip leading whitespace
        while (*cmd_ptr && isspace(*cmd_ptr)) {
            cmd_ptr++;
        }
        
        // Skip trailing whitespace
        char *end = cmd_ptr + strlen(cmd_ptr) - 1;
        while (end > cmd_ptr && isspace(*end)) {
            *end = '\0';
            end--;
        }
        
        // Skip empty commands
        if (*cmd_ptr) {
            // Initialize the command buffer
            alloc_cmd_buff(&clist->commands[clist->num]);
            
            // Build the command buffer
            int rc = build_cmd_buff(cmd_ptr, &clist->commands[clist->num]);
            if (rc != OK) {
                free(cmd_copy);
                return rc;
            }
            
            clist->num++;
        }
        
        // Move to the next command after the pipe
        if (pipe_pos) {
            cmd_ptr = pipe_pos + 1;
        } else {
            cmd_ptr = NULL;
        }
    }
    
    free(cmd_copy);
    
    if (clist->num == 0) {
        printf("%s", CMD_WARN_NO_CMD);
        return WARN_NO_CMDS;
    }
    
    return OK;
}

/*
 * free_cmd_list
 *
 * Frees memory allocated for the command list
 */
int free_cmd_list(command_list_t *cmd_lst) {
    for (int i = 0; i < cmd_lst->num; i++) {
        free_cmd_buff(&cmd_lst->commands[i]);
    }
    
    cmd_lst->num = 0;
    return OK;
}

/*
 * match_command
 *
 * Checks if a command is a built-in command
 */
Built_In_Cmds match_command(const char *input) {
    if (!input || !*input) {
        return BI_NOT_BI;
    }
    
    if (strcmp(input, EXIT_CMD) == 0) {
        return BI_CMD_EXIT;
    }
    
    if (strcmp(input, "dragon") == 0) {
        return BI_CMD_DRAGON;
    }
    
    if (strcmp(input, "cd") == 0) {
        return BI_CMD_CD;
    }
    
    return BI_NOT_BI;
}

/*
 * exec_built_in_cmd
 *
 * Executes a built-in command
 */
Built_In_Cmds exec_built_in_cmd(cmd_buff_t *cmd) {
    if (!cmd || cmd->argc == 0) {
        return BI_NOT_BI;
    }
    
    Built_In_Cmds bi_cmd = match_command(cmd->argv[0]);
    
    switch (bi_cmd) {
        case BI_CMD_EXIT:
            printf("exiting...\n");
            return BI_CMD_EXIT;
        
        case BI_CMD_DRAGON:
            print_dragon();
            return BI_EXECUTED;
        
        case BI_CMD_CD:
            if (cmd->argc < 2) {
                // No argument, change to home directory
                char *home = getenv("HOME");
                if (home) {
                    chdir(home);
                }
            } else {
                // Change to specified directory
                if (chdir(cmd->argv[1]) != 0) {
                    perror("cd");
                }
            }
            return BI_EXECUTED;
        
        default:
            return BI_NOT_BI;
    }
}

/*
 * exec_cmd
 *
 * Executes a single command
 * Handles input/output redirection for extra credit
 */
int exec_cmd(cmd_buff_t *cmd) {
    if (!cmd || cmd->argc == 0) {
        return ERR_CMD_ARGS_BAD;
    }
    
    // Check if it's a built-in command
    Built_In_Cmds bi_result = exec_built_in_cmd(cmd);
    if (bi_result == BI_CMD_EXIT) {
        return OK_EXIT;
    } else if (bi_result == BI_EXECUTED) {
        return OK;
    }
    
    // Fork a child process
    pid_t pid = fork();
    
    if (pid < 0) {
        perror("fork");
        return ERR_EXEC_CMD;
    } else if (pid == 0) {
        // Child process
        
        // Handle input redirection
        if (cmd->in_redir_type == REDIR_IN) {
            int fd = open(cmd->in_redir_file, O_RDONLY);
            if (fd < 0) {
                perror("open input");
                exit(EXIT_FAILURE);
            }
            if (dup2(fd, STDIN_FILENO) < 0) {
                perror("dup2 input");
                exit(EXIT_FAILURE);
            }
            close(fd);
        }
        
        // Handle output redirection
        if (cmd->out_redir_type != REDIR_NONE) {
            int flags = O_WRONLY | O_CREAT;
            if (cmd->out_redir_type == REDIR_APPEND) {
                flags |= O_APPEND;
            } else {
                flags |= O_TRUNC;
            }
            
            int fd = open(cmd->out_redir_file, flags, 0644);
            if (fd < 0) {
                perror("open output");
                exit(EXIT_FAILURE);
            }
            if (dup2(fd, STDOUT_FILENO) < 0) {
                perror("dup2 output");
                exit(EXIT_FAILURE);
            }
            close(fd);
        }
        
        execvp(cmd->argv[0], cmd->argv);
        perror("execvp");
        exit(EXIT_FAILURE);
    } else {
        // Parent process
        int status;
        waitpid(pid, &status, 0);
        return OK;
    }
}

/*
 * execute_pipeline
 *
 * Executes a pipeline of commands
 * Handles input/output redirection for extra credit
 */
int execute_pipeline(command_list_t *clist) {
    if (!clist || clist->num == 0) {
        return WARN_NO_CMDS;
    }
    
    // Check if it's a single command
    if (clist->num == 1) {
        return exec_cmd(&clist->commands[0]);
    }
    
    // Multiple commands - set up pipes
    int num_pipes = clist->num - 1;
    int pipes[num_pipes][2];
    pid_t pids[clist->num];
    
    // Create all pipes
    for (int i = 0; i < num_pipes; i++) {
        if (pipe(pipes[i]) < 0) {
            perror("pipe");
            return ERR_EXEC_CMD;
        }
    }
    
    // Create processes for each command
    for (int i = 0; i < clist->num; i++) {
        pids[i] = fork();
        
        if (pids[i] < 0) {
            perror("fork");
            return ERR_EXEC_CMD;
        } else if (pids[i] == 0) {
            // Child process
            
            // Handle input redirection for first command
            if (i == 0 && clist->commands[i].in_redir_type == REDIR_IN) {
                int fd = open(clist->commands[i].in_redir_file, O_RDONLY);
                if (fd < 0) {
                    perror("open input");
                    exit(EXIT_FAILURE);
                }
                if (dup2(fd, STDIN_FILENO) < 0) {
                    perror("dup2 input");
                    exit(EXIT_FAILURE);
                }
                close(fd);
            } 
            // Set up stdin from previous pipe (if not first command)
            else if (i > 0) {
                if (dup2(pipes[i-1][0], STDIN_FILENO) < 0) {
                    perror("dup2 stdin");
                    exit(EXIT_FAILURE);
                }
            }
            
            // Handle output redirection for last command
            if (i == clist->num - 1 && clist->commands[i].out_redir_type != REDIR_NONE) {
                int flags = O_WRONLY | O_CREAT;
                if (clist->commands[i].out_redir_type == REDIR_APPEND) {
                    flags |= O_APPEND;
                } else {
                    flags |= O_TRUNC;
                }
                
                int fd = open(clist->commands[i].out_redir_file, flags, 0644);
                if (fd < 0) {
                    perror("open output");
                    exit(EXIT_FAILURE);
                }
                if (dup2(fd, STDOUT_FILENO) < 0) {
                    perror("dup2 output");
                    exit(EXIT_FAILURE);
                }
                close(fd);
            } 
            // Set up stdout to next pipe (if not last command)
            else if (i < clist->num - 1) {
                if (dup2(pipes[i][1], STDOUT_FILENO) < 0) {
                    perror("dup2 stdout");
                    exit(EXIT_FAILURE);
                }
            }
            
            // Close all pipe fds
            for (int j = 0; j < num_pipes; j++) {
                close(pipes[j][0]);
                close(pipes[j][1]);
            }
            
            // Execute the command
            execvp(clist->commands[i].argv[0], clist->commands[i].argv);
            perror("execvp");
            exit(EXIT_FAILURE);
        }
    }
    
    // Parent process - close all pipe fds
    for (int i = 0; i < num_pipes; i++) {
        close(pipes[i][0]);
        close(pipes[i][1]);
    }
    
    // Wait for all children
    for (int i = 0; i < clist->num; i++) {
        int status;
        waitpid(pids[i], &status, 0);
    }
    
    return OK;
}

/*
 * exec_local_cmd_loop
 *
 * Main command loop
 */
int exec_local_cmd_loop() {
    char cmd_buff[SH_CMD_MAX];
    command_list_t clist;
    
    // Print the initial prompt
    printf("%s", SH_PROMPT);
    fflush(stdout);
    
    // Main command loop
    while (1) {
        // Read next command
        if (!fgets(cmd_buff, SH_CMD_MAX, stdin)) {
            // EOF => exit gracefully
            break;
        }
        
        // Strip trailing newline
        cmd_buff[strcspn(cmd_buff, "\n")] = '\0';
        
        // Check for empty input
        if (strlen(cmd_buff) == 0) {
            printf("%s", CMD_WARN_NO_CMD);
            printf("%s", SH_PROMPT);
            fflush(stdout);
            continue;
        }
        
        // Check for exit command
        if (strcmp(cmd_buff, EXIT_CMD) == 0) {
            printf("exiting...\n");
            break;
        }
        
        // Parse the command line into clist
        int rc = build_cmd_list(cmd_buff, &clist);
        if (rc != OK) {
            // If parse error or no commands, print prompt and continue
            printf("%s", SH_PROMPT);
            fflush(stdout);
            continue;
        }
        
        // Execute the pipeline
        rc = execute_pipeline(&clist);
        if (rc == OK_EXIT) {
            break;
        }
        
        // Free the command list
        free_cmd_list(&clist);
        
        // Print the prompt again
        printf("%s", SH_PROMPT);
        fflush(stdout);
    }
    
    return 0;
}
