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
int build_cmd_list(char *cmd_line, command_list_t *clist)
{
    /*Current command count = 0*/
    clist->num = 0;

    char *token = strtok(cmd_line, PIPE_STRING);
    
    while (token != NULL) {
        /*Check if we have reached the max number of commands*/
        if (clist->num >= CMD_MAX) {
            printf(CMD_ERR_PIPE_LIMIT, CMD_MAX);
            return ERR_TOO_MANY_COMMANDS;
        }
    
        /*Remove leading spaces*/
        while (*token == SPACE_CHAR) {
            token++;
        }
        
        /*Remove spaces from end*/
        char *end = token + strlen(token) - 1;
        while (end > token && *end == SPACE_CHAR) {
           *end-- = '\0';
        }

        /*Extract commands and arguments*/
        char *arg_start = strchr(token, SPACE_CHAR);
        if (arg_start != NULL) {
            *arg_start = '\0';
            arg_start++;

            if (strlen(token) >= EXE_MAX || strlen(arg_start) >= ARG_MAX) {
                return ERR_CMD_OR_ARGS_TOO_BIG;
            };

            strncpy(clist->commands[clist->num].exe, token, EXE_MAX - 1);
            strncpy(clist->commands[clist->num].args, arg_start, ARG_MAX - 1);
        } else {
            if (strlen(token) >= EXE_MAX) {
                return ERR_CMD_OR_ARGS_TOO_BIG;
            }
            strncpy(clist->commands[clist->num].exe, token, EXE_MAX - 1);
            clist->commands[clist->num].exe[EXE_MAX - 1] = '\0'; 
            clist->commands[clist->num].args[0] = '\0';
        }

        clist->num++;
        token = strtok(NULL, PIPE_STRING);
    }

    if (clist->num == 0) {
        printf(CMD_WARN_NO_CMD);
        return WARN_NO_CMDS;
    }

    return OK;
}

/*Function to decompress and print the Drexel Dragon*/
void print_dragon() {
    const rle_pair_t *ptr = DREXEL_DRAGON_RLE;
    while (ptr->count != 0) {  
        for (int i = 0; i < ptr->count; i++) {
            putchar(ptr->ch);  
        }
        ptr++; 
    }
}