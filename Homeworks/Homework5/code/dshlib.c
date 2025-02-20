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
    
    // Make a copy of cmd_line since strtok modifies the string
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
        
        // Find first space or quote
        char *args_start = token;
        int in_quotes = 0;
        
        // Find the executable name
        while (*args_start && (*args_start != SPACE_CHAR || in_quotes)) {
            if (*args_start == '"') in_quotes = !in_quotes;
            args_start++;
        }
        
        // Save the executable name
        size_t exe_len = args_start - token;
        if (exe_len >= EXE_MAX) {
            free(cmd_copy);
            return ERR_CMD_OR_ARGS_TOO_BIG;
        }
        strncpy(clist->commands[clist->num].exe, token, exe_len);
        clist->commands[clist->num].exe[exe_len] = '\0';
        
        // Skip spaces after executable
        while (*args_start == SPACE_CHAR) args_start++;
        
        // Copy remaining arguments preserving quotes
        if (*args_start) {
            if (strlen(args_start) >= ARG_MAX) {
                free(cmd_copy);
                return ERR_CMD_OR_ARGS_TOO_BIG;
            }
            strcpy(clist->commands[clist->num].args, args_start);
        } else {
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