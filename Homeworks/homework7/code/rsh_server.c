#include <sys/socket.h>
#include <sys/wait.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/un.h>
#include <fcntl.h>
#include <errno.h>
#include <pthread.h> 

#include "dshlib.h"
#include "rshlib.h"

// Declaration for dragon function 
extern void print_dragon(void);

// Structure for thread arguments
typedef struct {
    int client_socket;
} thread_args_t;

// Flag to indicate if server should stop
volatile int server_should_stop = 0;
pthread_mutex_t server_mutex = PTHREAD_MUTEX_INITIALIZER;

/*
 * start_server(ifaces, port, is_threaded)
 * Main server function - now supports multi-threading
 */
int start_server(char *ifaces, int port, int is_threaded) {
    int svr_socket;
    int rc = OK;
    
    // Initialize server mutex
    pthread_mutex_init(&server_mutex, NULL);
    server_should_stop = 0;

    // Boot up the server
    svr_socket = boot_server(ifaces, port);
    if (svr_socket < 0) {
        int err_code = svr_socket;  
        return err_code;
    }

    // Process client requests - different handling based on threading mode
    if (is_threaded) {
        rc = process_threaded_requests(svr_socket);
    } else {
        rc = process_cli_requests(svr_socket);
    }

    // Stop the server
    stop_server(svr_socket);
    
    // Clean up mutex
    pthread_mutex_destroy(&server_mutex);

    return rc;
}

/*
 * stop_server(svr_socket)
 * Close server socket
 */
int stop_server(int svr_socket) {
    return close(svr_socket);
}

/*
 * boot_server(ifaces, port)
 * Initialize server socket
 */
int boot_server(char *ifaces, int port) {
    int svr_socket;
    int ret;
    struct sockaddr_in addr;
    
    // Create socket
    svr_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (svr_socket < 0) {
        perror("socket");
        return ERR_RDSH_COMMUNICATION;
    }
    
    // Set socket options to reuse address
    int enable = 1;
    ret = setsockopt(svr_socket, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int));
    if (ret < 0) {
        perror("setsockopt");
        close(svr_socket);
        return ERR_RDSH_COMMUNICATION;
    }

    // Set up address info
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    
    // Convert IP string to network format
    if (inet_pton(AF_INET, ifaces, &addr.sin_addr) <= 0) {
        perror("inet_pton");
        close(svr_socket);
        return ERR_RDSH_COMMUNICATION;
    }

    // Bind socket to address
    ret = bind(svr_socket, (struct sockaddr*)&addr, sizeof(addr));
    if (ret < 0) {
        perror("bind");
        close(svr_socket);
        return ERR_RDSH_COMMUNICATION;
    }

    // Start listening for connections
    ret = listen(svr_socket, 20);
    if (ret == -1) {
        perror("listen");
        close(svr_socket);
        return ERR_RDSH_COMMUNICATION;
    }

    printf("Server listening on %s:%d\n", ifaces, port);
    return svr_socket;
}

/*
 * process_cli_requests(svr_socket)
 * Accept and process client connections
 */
int process_cli_requests(int svr_socket) {
    int cli_socket;
    int rc = OK;
    struct sockaddr_in client_addr;
    socklen_t addr_len = sizeof(client_addr);

    while(1) {
        // Accept client connection
        cli_socket = accept(svr_socket, (struct sockaddr*)&client_addr, &addr_len);
        if (cli_socket < 0) {
            perror("accept");
            return ERR_RDSH_COMMUNICATION;
        }
        
        // Get client address information for logging
        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, sizeof(client_ip));
        printf("Client connected from %s:%d\n", client_ip, ntohs(client_addr.sin_port));
        
        // Process client requests
        rc = exec_client_requests(cli_socket);
        
        // Close client socket
        close(cli_socket);
        
        // Check if we should stop the server
        if (rc == OK_EXIT) {
            printf("%s", RCMD_MSG_SVR_STOP_REQ);
            break;
        } else {
            printf("%s", RCMD_MSG_CLIENT_EXITED);
        }
    }

    return rc;
}

/*
 * process_threaded_requests(svr_socket)
 * Accept and process client connections in separate threads
 */
int process_threaded_requests(int svr_socket) {
    int cli_socket;
    pthread_t thread_id;
    struct sockaddr_in client_addr;
    socklen_t addr_len = sizeof(client_addr);
    thread_args_t *thread_args;
    
    // Set up server for non-blocking accept in case we need to check for shutdown
    int flags = fcntl(svr_socket, F_GETFL, 0);
    fcntl(svr_socket, F_SETFL, flags | O_NONBLOCK);

    while(!server_should_stop) {
        // Accept client connection (non-blocking)
        cli_socket = accept(svr_socket, (struct sockaddr*)&client_addr, &addr_len);
        
        if (cli_socket < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                // No connection available, sleep a bit and try again
                usleep(100000);  // 100ms
                continue;
            } else {
                perror("accept");
                return ERR_RDSH_COMMUNICATION;
            }
        }
        
        // Get client address information for logging
        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, sizeof(client_ip));
        printf("Client connected from %s:%d\n", client_ip, ntohs(client_addr.sin_port));
        
        // Allocate thread arguments 
        thread_args = (thread_args_t*)malloc(sizeof(thread_args_t));
        if (!thread_args) {
            perror("malloc");
            close(cli_socket);
            continue;
        }
        thread_args->client_socket = cli_socket;
        
        // Create a new thread to handle this client
        if (pthread_create(&thread_id, NULL, client_thread, (void*)thread_args) != 0) {
            perror("pthread_create");
            free(thread_args);
            close(cli_socket);
            continue;
        }
        
        // Detach thread so it cleans up automatically when done
        pthread_detach(thread_id);
    }
    
    printf("Multi-threaded server stopping...\n");
    return OK_EXIT;
}

/*
 * client_thread(void *arg)
 * Thread function to handle a client connection
 */
void *client_thread(void *arg) {
    thread_args_t *thread_args = (thread_args_t*)arg;
    int cli_socket = thread_args->client_socket;
    int rc;
    
    // Process client requests
    rc = exec_client_requests(cli_socket);
    
    // Check if we should stop the server
    if (rc == OK_EXIT) {
        printf("%s", RCMD_MSG_SVR_STOP_REQ);
        pthread_mutex_lock(&server_mutex);
        server_should_stop = 1;
        pthread_mutex_unlock(&server_mutex);
    } else {
        printf("%s", RCMD_MSG_CLIENT_EXITED);
    }
    
    // Close client socket
    close(cli_socket);
    
    // Free thread arguments
    free(thread_args);
    
    return NULL;
}

/*
 * exec_client_requests(cli_socket)
 * Handle commands from a client
 */
int exec_client_requests(int cli_socket) {
    int io_size;
    command_list_t cmd_list;
    int rc;
    int cmd_rc;
    char *io_buff;
    
    // Allocate buffer for network I/O
    io_buff = malloc(RDSH_COMM_BUFF_SZ);
    if (io_buff == NULL) {
        return ERR_RDSH_SERVER;
    }

    while(1) {
        // Receive command from client
        io_size = recv(cli_socket, io_buff, RDSH_COMM_BUFF_SZ - 1, 0);
        
        if (io_size <= 0) {
            // Client closed connection or error
            free(io_buff);
            return OK;
        }
        
        // Ensure null termination
        io_buff[io_size] = '\0';
        
        // Print debug message showing received command
        printf(RCMD_MSG_SVR_EXEC_REQ, io_buff);
        
        // Check for exit command
        if (strcmp(io_buff, EXIT_CMD) == 0) {
            printf(RCMD_MSG_CLIENT_EXITED);
            free(io_buff);
            return OK;
        }
        
        // Check for stop-server command
        if (strcmp(io_buff, "stop-server") == 0) {
            send_message_string(cli_socket, "Stopping server...\n");
            send_message_eof(cli_socket);
            free(io_buff);
            return OK_EXIT;
        }
        
        // Check for empty command
        if (strlen(io_buff) == 0) {
            send_message_string(cli_socket, CMD_WARN_NO_CMD);
            send_message_eof(cli_socket);
            continue;
        }
        
        // Build command list
        rc = build_cmd_list(io_buff, &cmd_list);
        if (rc != OK) {
            // Error parsing command
            char error_msg[256];
            if (rc == WARN_NO_CMDS) {
                snprintf(error_msg, sizeof(error_msg), "%s", CMD_WARN_NO_CMD);
            } else if (rc == ERR_TOO_MANY_COMMANDS) {
                snprintf(error_msg, sizeof(error_msg), CMD_ERR_PIPE_LIMIT, CMD_MAX);
            } else if (rc == ERR_CMD_ARGS_BAD) {
                snprintf(error_msg, sizeof(error_msg), "%s", CMD_ERR_REDIR);
            } else {
                snprintf(error_msg, sizeof(error_msg), CMD_ERR_RDSH_ITRNL, rc);
            }
            send_message_string(cli_socket, error_msg);
            send_message_eof(cli_socket);
            continue;
        }
        
        // Handle built-in commands for the first command in the pipeline
        if (cmd_list.num == 1) {
            Built_In_Cmds bi_cmd = match_command(cmd_list.commands[0].argv[0]);
            
            if (bi_cmd == BI_CMD_DRAGON) {
                // Special handling for dragon command
                char dragon_output[4096] = {0};
                FILE *temp_file = fmemopen(dragon_output, sizeof(dragon_output), "w");
                if (temp_file != NULL) {
                    // Redirect stdout temporarily to capture dragon output
                    int old_stdout = dup(STDOUT_FILENO);
                    dup2(fileno(temp_file), STDOUT_FILENO);
                    
                    // Call the dragon function
                    print_dragon();
                    
                    // Restore stdout
                    fflush(stdout);
                    dup2(old_stdout, STDOUT_FILENO);
                    close(old_stdout);
                    fclose(temp_file);
                    
                    // Send the captured output to client
                    send_message_string(cli_socket, dragon_output);
                    send_message_eof(cli_socket);
                    
                    free_cmd_list(&cmd_list);
                    continue;
                }
            }
            else if (bi_cmd == BI_CMD_CD) {
                // Handle cd command
                if (cmd_list.commands[0].argc > 1) {
                    if (chdir(cmd_list.commands[0].argv[1]) != 0) {
                        char error_msg[256];
                        snprintf(error_msg, sizeof(error_msg), "cd: %s: No such file or directory\n", 
                                cmd_list.commands[0].argv[1]);
                        send_message_string(cli_socket, error_msg);
                    }
                } else {
                    // No argument, change to home directory
                    char *home = getenv("HOME");
                    if (home) {
                        chdir(home);
                    }
                }
                send_message_eof(cli_socket);
                free_cmd_list(&cmd_list);
                continue;
            }
            // Handle stop-server 
            else if (strcmp(cmd_list.commands[0].argv[0], "stop-server") == 0) {
                send_message_string(cli_socket, "Stopping server...\n");
                send_message_eof(cli_socket);
                free_cmd_list(&cmd_list);
                free(io_buff);
                return OK_EXIT;
            }
        }
        
        // Execute command pipeline
        cmd_rc = rsh_execute_pipeline(cli_socket, &cmd_list);
        
        // Print return code for debugging
        printf(RCMD_MSG_SVR_RC_CMD, cmd_rc);
        
        // Free command list
        free_cmd_list(&cmd_list);
        
        // Send EOF to mark end of command output
        rc = send_message_eof(cli_socket);
        if (rc != OK) {
            free(io_buff);
            return ERR_RDSH_COMMUNICATION;
        }
    }

    free(io_buff);
    return OK;
}

/*
 * send_message_eof(cli_socket)
 * Send EOF character to mark end of response
 */
int send_message_eof(int cli_socket) {
    int send_len = (int)sizeof(RDSH_EOF_CHAR);
    int sent_len;
    sent_len = send(cli_socket, &RDSH_EOF_CHAR, send_len, 0);

    if (sent_len != send_len) {
        perror("send_message_eof");
        return ERR_RDSH_COMMUNICATION;
    }
    return OK;
}

/*
 * send_message_string(cli_socket, buff)
 * Send a message string to the client
 */
int send_message_string(int cli_socket, char *buff) {
    int send_len = strlen(buff);
    int sent_len;
    
    sent_len = send(cli_socket, buff, send_len, 0);
    if (sent_len != send_len) {
        perror("send_message_string");
        return ERR_RDSH_COMMUNICATION;
    }
    
    return OK;
}

/*
 * rsh_execute_pipeline(cli_sock, clist)
 * Execute command pipeline, redirecting I/O to client socket
 */
int rsh_execute_pipeline(int cli_sock, command_list_t *clist) {
    int pipes[clist->num - 1][2];  // Array of pipes
    pid_t pids[clist->num];
    int pids_st[clist->num];       // Array to store process status
    Built_In_Cmds bi_cmd;
    int exit_code;

    // Check for built-in commands first
    if (clist->num == 1) {
        bi_cmd = match_command(clist->commands[0].argv[0]);
        // Special handling for stop-server without using enum value
        if (strcmp(clist->commands[0].argv[0], "stop-server") == 0) {
            return EXIT_SC;  // Signal to stop server
        } else if (bi_cmd == BI_EXECUTED) {
            // Built-in command was executed
            return OK;
        }
    }

    // Create pipes for command pipeline
    for (int i = 0; i < clist->num - 1; i++) {
        if (pipe(pipes[i]) == -1) {
            perror("pipe");
            send_message_string(cli_sock, "Error creating pipe\n");
            return ERR_RDSH_CMD_EXEC;
        }
    }

    // Fork and execute each command in the pipeline
    for (int i = 0; i < clist->num; i++) {
        pids[i] = fork();
        
        if (pids[i] < 0) {
            // Fork failed
            perror("fork");
            send_message_string(cli_sock, "Error creating process\n");
            return ERR_RDSH_CMD_EXEC;
        } else if (pids[i] == 0) {
            // Child process
            
            // Handle input redirection for first command
            if (i == 0 && clist->commands[i].in_redir_type == REDIR_IN) {
                int fd = open(clist->commands[i].in_redir_file, O_RDONLY);
                if (fd < 0) {
                    char error_msg[256];
                    snprintf(error_msg, sizeof(error_msg), 
                             "Cannot open input file: %s\n", clist->commands[i].in_redir_file);
                    write(cli_sock, error_msg, strlen(error_msg));
                    exit(EXIT_FAILURE);
                }
                dup2(fd, STDIN_FILENO);
                close(fd);
            } 
            // If no input redirection for first command, use client socket for stdin
            else if (i == 0) {
                dup2(cli_sock, STDIN_FILENO);
            } 
            // Not first process: redirect stdin from previous pipe
            else {
                dup2(pipes[i-1][0], STDIN_FILENO);
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
                    char error_msg[256];
                    snprintf(error_msg, sizeof(error_msg), 
                             "Cannot open output file: %s\n", clist->commands[i].out_redir_file);
                    write(cli_sock, error_msg, strlen(error_msg));
                    exit(EXIT_FAILURE);
                }
                dup2(fd, STDOUT_FILENO);
                close(fd);
                
                // Always redirect stderr to client
                dup2(cli_sock, STDERR_FILENO);
            } 
            // If no output redirection for last command, use client socket
            else if (i == clist->num - 1) {
                dup2(cli_sock, STDOUT_FILENO);
                dup2(cli_sock, STDERR_FILENO);
            } 
            // Not last process: redirect stdout to next pipe
            else {
                dup2(pipes[i][1], STDOUT_FILENO);
                // For stderr, always redirect to client socket
                dup2(cli_sock, STDERR_FILENO);
            }
            
            // Close all pipe file descriptors
            for (int j = 0; j < clist->num - 1; j++) {
                close(pipes[j][0]);
                close(pipes[j][1]);
            }
            
            // Execute command
            execvp(clist->commands[i].argv[0], clist->commands[i].argv);
            
            // If execvp returns, there was an error
            char error_msg[256];
            
            
            if (errno == ENOENT) {
                snprintf(error_msg, sizeof(error_msg), "%s: command not found\n", clist->commands[i].argv[0]);
            } else {
                snprintf(error_msg, sizeof(error_msg), "%s: %s\n", clist->commands[i].argv[0], strerror(errno));
            }


            write(cli_sock, error_msg, strlen(error_msg));
            exit(EXIT_FAILURE);
        }
    }

    // Parent process: close all pipe ends
    for (int i = 0; i < clist->num - 1; i++) {
        close(pipes[i][0]);
        close(pipes[i][1]);
    }

    // Wait for all children to complete
    for (int i = 0; i < clist->num; i++) {
        waitpid(pids[i], &pids_st[i], 0);
    }

    // Get exit code from last process in pipeline
    exit_code = WEXITSTATUS(pids_st[clist->num - 1]);
    
    // Check if any process requested server exit
    for (int i = 0; i < clist->num; i++) {
        if (WEXITSTATUS(pids_st[i]) == EXIT_SC) {
            exit_code = EXIT_SC;
        }
    }
    
    return exit_code;
}