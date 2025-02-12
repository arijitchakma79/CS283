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

const rle_pair_t DREXEL_DRAGON_RLE[] = {
    {'\n', 1},
    {' ', 72},
    {'@', 1},
    {'%', 4},
    {' ', 24},
    {'\n', 1},
    {' ', 69},
    {'%', 6},
    {' ', 26},
    {'\n', 1},
    {' ', 68},
    {'%', 6},
    {' ', 27},
    {'\n', 1},
    {' ', 65},
    {'%', 1},
    {' ', 1},
    {'%', 7},
    {' ', 11},
    {'@', 1},
    {' ', 14},
    {'\n', 1},
    {' ', 64},
    {'%', 10},
    {' ', 8},
    {'%', 7},
    {' ', 11},
    {'\n', 1},
    {' ', 39},
    {'%', 7},
    {' ', 2},
    {'%', 4},
    {'@', 1},
    {' ', 9},
    {'%', 12},
    {'@', 1},
    {' ', 4},
    {'%', 6},
    {' ', 2},
    {'@', 1},
    {'%', 4},
    {' ', 8},
    {'\n', 1},
    {' ', 34},
    {'%', 22},
    {' ', 6},
    {'%', 28},
    {' ', 10},
    {'\n', 1},
    {' ', 32},
    {'%', 26},
    {' ', 3},
    {'%', 12},
    {' ', 1},
    {'%', 15},
    {' ', 11},
    {'\n', 1},
    {' ', 31},
    {'%', 29},
    {' ', 1},
    {'%', 19},
    {' ', 5},
    {'%', 3},
    {' ', 12},
    {'\n', 1},
    {' ', 29},
    {'%', 28},
    {'@', 1},
    {' ', 1},
    {'@', 1},
    {'%', 18},
    {' ', 8},
    {'%', 2},
    {' ', 12},
    {'\n', 1},
    {' ', 28},
    {'%', 33},
    {' ', 1},
    {'%', 22},
    {' ', 16},
    {'\n', 1},
    {' ', 28},
    {'%', 58},
    {' ', 14},
    {'\n', 1},
    {' ', 28},
    {'%', 50},
    {'@', 1},
    {'%', 6},
    {'@', 1},
    {' ', 14},
    {'\n', 1},
    {' ', 6},
    {'%', 8},
    {'@', 1},
    {' ', 11},
    {'%', 16},
    {' ', 8},
    {'%', 26},
    {' ', 6},
    {'%', 2},
    {' ', 16},
    {'\n', 1},
    {' ', 4},
    {'%', 13},
    {' ', 9},
    {'%', 2},
    {'@', 1},
    {'%', 12},
    {' ', 11},
    {'%', 11},
    {' ', 1},
    {'%', 12},
    {' ', 6},
    {'@', 1},
    {'%', 1},
    {' ', 16},
    {'\n', 1},
    {' ', 2},
    {'%', 10},
    {' ', 3},
    {'%', 3},
    {' ', 8},
    {'%', 14},
    {' ', 12},
    {'%', 24},
    {' ', 24},
    {'\n', 1},
    {' ', 1},
    {'%', 9},
    {' ', 7},
    {'%', 1},
    {' ', 9},
    {'%', 13},
    {' ', 13},
    {'%', 12},
    {'@', 1},
    {'%', 11},
    {' ', 23},
    {'\n', 1},
    {'%', 9},
    {'@', 1},
    {' ', 16},
    {'%', 1},
    {' ', 1},
    {'%', 13},
    {' ', 12},
    {'@', 1},
    {'%', 25},
    {' ', 20},
    {'\n', 1},
    {'%', 8},
    {'@', 1},
    {' ', 17},
    {'%', 2},
    {'@', 1},
    {'%', 12},
    {' ', 12},
    {'@', 1},
    {'%', 28},
    {' ', 17},
    {'\n', 1},
    {'%', 7},
    {'@', 1},
    {' ', 19},
    {'%', 15},
    {' ', 11},
    {'%', 33},
    {' ', 13},
    {'\n', 1},
    {'%', 10},
    {' ', 18},
    {'%', 15},
    {' ', 10},
    {'%', 35},
    {' ', 6},
    {'%', 4},
    {' ', 2},
    {'\n', 1},
    {'%', 9},
    {'@', 1},
    {' ', 19},
    {'@', 1},
    {'%', 14},
    {' ', 9},
    {'%', 12},
    {'@', 1},
    {' ', 1},
    {'%', 4},
    {' ', 1},
    {'%', 17},
    {' ', 3},
    {'%', 8},
    {'\n', 1},
    {'%', 10},
    {' ', 18},
    {'%', 17},
    {' ', 8},
    {'%', 13},
    {' ', 6},
    {'%', 18},
    {' ', 1},
    {'%', 9},
    {'\n', 1},
    {'%', 9},
    {'@', 1},
    {'%', 2},
    {'@', 1},
    {' ', 16},
    {'%', 16},
    {'@', 1},
    {' ', 7},
    {'%', 14},
    {' ', 5},
    {'%', 24},
    {' ', 2},
    {'%', 2},
    {'\n', 1},
    {' ', 1},
    {'%', 10},
    {' ', 18},
    {'%', 1},
    {' ', 1},
    {'%', 14},
    {'@', 1},
    {' ', 8},
    {'%', 14},
    {' ', 3},
    {'%', 26},
    {' ', 1},
    {'%', 2},
    {'\n', 1},
    {' ', 2},
    {'%', 12},
    {' ', 2},
    {'@', 1},
    {' ', 11},
    {'%', 18},
    {' ', 8},
    {'%', 40},
    {' ', 2},
    {'%', 3},
    {' ', 1},
    {'\n', 1},
    {' ', 3},
    {'%', 13},
    {' ', 1},
    {'%', 2},
    {' ', 2},
    {'%', 1},
    {' ', 2},
    {'%', 1},
    {'@', 1},
    {' ', 1},
    {'%', 18},
    {' ', 10},
    {'%', 37},
    {' ', 4},
    {'%', 3},
    {' ', 1},
    {'\n', 1},
    {' ', 4},
    {'%', 18},
    {' ', 1},
    {'%', 22},
    {' ', 11},
    {'@', 1},
    {'%', 31},
    {' ', 4},
    {'%', 7},
    {' ', 1},
    {'\n', 1},
    {' ', 5},
    {'%', 39},
    {' ', 14},
    {'%', 28},
    {' ', 8},
    {'%', 3},
    {' ', 3},
    {'\n', 1},
    {' ', 6},
    {'@', 1},
    {'%', 35},
    {' ', 18},
    {'%', 25},
    {' ', 15},
    {'\n', 1},
    {' ', 8},
    {'%', 32},
    {' ', 22},
    {'%', 19},
    {' ', 2},
    {'%', 7},
    {' ', 10},
    {'\n', 1},
    {' ', 11},
    {'%', 26},
    {' ', 27},
    {'%', 15},
    {' ', 2},
    {'@', 1},
    {'%', 9},
    {' ', 9},
    {'\n', 1},
    {' ', 14},
    {'%', 20},
    {' ', 11},
    {'@', 1},
    {'%', 1},
    {'@', 1},
    {'%', 1},
    {' ', 18},
    {'@', 1},
    {'%', 18},
    {' ', 3},
    {'%', 3},
    {' ', 8},
    {'\n', 1},
    {' ', 18},
    {'%', 15},
    {' ', 8},
    {'%', 10},
    {' ', 20},
    {'%', 15},
    {' ', 4},
    {'%', 1},
    {' ', 9},
    {'\n', 1},
    {' ', 16},
    {'%', 36},
    {' ', 22},
    {'%', 14},
    {' ', 12},
    {'\n', 1},
    {' ', 16},
    {'%', 26},
    {' ', 2},
    {'%', 4},
    {' ', 1},
    {'%', 3},
    {' ', 22},
    {'%', 10},
    {' ', 2},
    {'%', 3},
    {'@', 1},
    {' ', 10},
    {'\n', 1},
    {' ', 21},
    {'%', 19},
    {' ', 1},
    {'%', 6},
    {' ', 1},
    {'%', 2},
    {' ', 26},
    {'%', 13},
    {'@', 1},
    {' ', 10},
    {'\n', 1},
    {' ', 81},
    {'%', 7},
    {'@', 1},
    {' ', 10},
    {'\n', 1},
    {' ', 0}  
};


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
