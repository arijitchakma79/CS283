#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "dshlib.h"

/*
 *  build_cmd_list
 *    cmd_line:     the command line from the user
 *    clist *:      pointer to clist structure to be populated
 *
 *  This function builds the command_list_t structure passed by the caller
 *  It does this by first splitting the cmd_line into commands by spltting
 *  the string based on any pipe characters '|'.  It then traverses each
 *  command.  For each command (a substring of cmd_line), it then parses
 *  that command by taking the first token as the executable name, and
 *  then the remaining tokens as the arguments.
 *
 *  NOTE your implementation should be able to handle properly removing
 *  leading and trailing spaces!
 *
 *  errors returned:
 *
 *    OK:                      No Error
 *    ERR_TOO_MANY_COMMANDS:   There is a limit of CMD_MAX (see dshlib.h)
 *                             commands.
 *    ERR_CMD_OR_ARGS_TOO_BIG: One of the commands provided by the user
 *                             was larger than allowed, either the
 *                             executable name, or the arg string.
 *
 *  Standard Library Functions You Might Want To Consider Using
 *      memset(), strcmp(), strcpy(), strtok(), strlen(), strchr()
 */


int build_cmd_list(char *cmd_line, command_list_t *clist) {
    clist->num = 0;
    
    char *cmd_copy = strdup(cmd_line);
    if (!cmd_copy) return ERR_CMD_OR_ARGS_TOO_BIG;
    
    char *token = strtok(cmd_copy, PIPE_STRING);
    
    while (token != NULL) {
        if (clist->num >= CMD_MAX) {
            printf(CMD_ERR_PIPE_LIMIT, CMD_MAX);
            free(cmd_copy);
            return ERR_TOO_MANY_COMMANDS;
        }
        
        // Skip leading spaces
        while (*token == SPACE_CHAR) token++;
        
        // Handle the executable name first
        char *space = strchr(token, SPACE_CHAR);
        if (space) {
            size_t exe_len = space - token;
            if (exe_len >= EXE_MAX) {
                free(cmd_copy);
                return ERR_CMD_OR_ARGS_TOO_BIG;
            }
            strncpy(clist->commands[clist->num].exe, token, exe_len);
            clist->commands[clist->num].exe[exe_len] = '\0';
            
            // Skip spaces after executable
            while (*space == SPACE_CHAR) space++;
            
            // Handle the arguments
            if (*space) {
                if (strlen(space) >= ARG_MAX) {
                    free(cmd_copy);
                    return ERR_CMD_OR_ARGS_TOO_BIG;
                }
                strcpy(clist->commands[clist->num].args, space);
            } else {
                clist->commands[clist->num].args[0] = '\0';
            }
        } else {
            // No arguments, just executable
            if (strlen(token) >= EXE_MAX) {
                free(cmd_copy);
                return ERR_CMD_OR_ARGS_TOO_BIG;
            }
            strcpy(clist->commands[clist->num].exe, token);
            clist->commands[clist->num].args[0] = '\0';
        }
        
        clist->num++;
        token = strtok(NULL, PIPE_STRING);
    }
    
    free(cmd_copy);
    
    if (clist->num == 0) {
        printf(CMD_WARN_NO_CMD);
        return WARN_NO_CMDS;
    }
    
    return OK;
}