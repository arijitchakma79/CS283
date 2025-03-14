#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/un.h>
#include <fcntl.h>

#include "dshlib.h"
#include "rshlib.h"

/*
 * exec_remote_cmd_loop(server_ip, port)
 * Client implementation for the Remote Drexel Shell
 */
int exec_remote_cmd_loop(char *address, int port)
{
    char *cmd_buff;
    char *rsp_buff;
    int cli_socket;
    ssize_t io_size;
    int is_eof;

    // Allocate buffers for commands and responses
    cmd_buff = malloc(RDSH_COMM_BUFF_SZ);
    if (cmd_buff == NULL) {
        return client_cleanup(-1, NULL, NULL, ERR_MEMORY);
    }

    rsp_buff = malloc(RDSH_COMM_BUFF_SZ);
    if (rsp_buff == NULL) {
        return client_cleanup(-1, cmd_buff, NULL, ERR_MEMORY);
    }

    // Initialize the client socket
    cli_socket = start_client(address, port);
    if (cli_socket < 0) {
        perror("start client");
        return client_cleanup(cli_socket, cmd_buff, rsp_buff, ERR_RDSH_CLIENT);
    }

    while (1) 
    {
        // Print shell prompt
        printf("%s", SH_PROMPT);
        fflush(stdout);

        // Get user input
        if (fgets(cmd_buff, RDSH_COMM_BUFF_SZ, stdin) == NULL) {
            printf("\n");
            break;
        }

        // Remove trailing newline
        cmd_buff[strcspn(cmd_buff, "\n")] = '\0';

        // Check for empty input
        if (strlen(cmd_buff) == 0) {
            continue;
        }

        // Exit if user typed "exit"
        if (strcmp(cmd_buff, EXIT_CMD) == 0) {
            // Still send exit to server so it can close this client connection
            int send_len = strlen(cmd_buff) + 1; // +1 for null terminator
            io_size = send(cli_socket, cmd_buff, send_len, 0);
            if (io_size != send_len) {
                perror("send");
                return client_cleanup(cli_socket, cmd_buff, rsp_buff, ERR_RDSH_COMMUNICATION);
            }
            break;
        }

        // Send command to server (with null terminator)
        int send_len = strlen(cmd_buff) + 1; // +1 for null terminator
        io_size = send(cli_socket, cmd_buff, send_len, 0);
        if (io_size != send_len) {
            perror("send");
            return client_cleanup(cli_socket, cmd_buff, rsp_buff, ERR_RDSH_COMMUNICATION);
        }

        // Process server responses until we get EOF marker
        do {
            io_size = recv(cli_socket, rsp_buff, RDSH_COMM_BUFF_SZ - 1, 0);
            
            if (io_size < 0) {
                perror("recv");
                return client_cleanup(cli_socket, cmd_buff, rsp_buff, ERR_RDSH_COMMUNICATION);
            } else if (io_size == 0) {
                // Server closed the connection
                printf("%s", RCMD_SERVER_EXITED);
                return client_cleanup(cli_socket, cmd_buff, rsp_buff, ERR_RDSH_COMMUNICATION);
            }

            // Check if this message includes the EOF marker
            is_eof = (rsp_buff[io_size - 1] == RDSH_EOF_CHAR) ? 1 : 0;
            
            if (is_eof) {
                // Don't print the EOF character
                printf("%.*s", (int)io_size - 1, rsp_buff);
            } else {
                // Print the whole buffer
                printf("%.*s", (int)io_size, rsp_buff);
            }
            
            // Flush stdout to ensure immediate display
            fflush(stdout);
            
        } while (!is_eof);
    }

    return client_cleanup(cli_socket, cmd_buff, rsp_buff, OK);
}

/*
 * start_client(server_ip, port)
 * Create client socket and connect to server
 */
int start_client(char *server_ip, int port) {
    struct sockaddr_in addr;
    int cli_socket;
    int ret;

    // Create socket
    cli_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (cli_socket < 0) {
        perror("socket");
        return ERR_RDSH_CLIENT;
    }

    // Set up server address
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    
    if (inet_pton(AF_INET, server_ip, &addr.sin_addr) <= 0) {
        perror("inet_pton");
        close(cli_socket);
        return ERR_RDSH_CLIENT;
    }

    // Connect to server
    ret = connect(cli_socket, (struct sockaddr*)&addr, sizeof(addr));
    if (ret < 0) {
        perror("connect");
        close(cli_socket);
        return ERR_RDSH_CLIENT;
    }

    return cli_socket;
}

/*
 * client_cleanup(int cli_socket, char *cmd_buff, char *rsp_buff, int rc)
 * Clean up client resources
 */
int client_cleanup(int cli_socket, char *cmd_buff, char *rsp_buff, int rc) {
    // If a valid socket number, close it
    if (cli_socket > 0) {
        close(cli_socket);
    }

    // Free up the buffers 
    if (cmd_buff != NULL) {
        free(cmd_buff);
    }
    
    if (rsp_buff != NULL) {
        free(rsp_buff);
    }

    // Echo the return value that was passed as a parameter
    return rc;
}