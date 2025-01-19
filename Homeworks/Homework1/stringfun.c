
/*
Name: Arijit Chakma
Homework 1: CS283
Purpose: 

*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>


#define BUFFER_SZ 50

//prototypes
void usage(char *);
void print_buff(char *, int);
int  setup_buff(char *, char *, int);

//prototypes for functions to handle required functionality
int  count_words(char *, int, int);
//add additional prototypes here
void reverse_string(char *, int, int);
void replace_string(char *buff, int len, int str_len, char *find, char *replace);
void word_print(char *, int, int);


int setup_buff(char *buff, char *user_str, int len) {
    if (user_str == NULL || *user_str == '\0') {
        return -2;
    }

    char *src = user_str;
    char *dest = buff;
    int count = 0;
    int space_flag = 1;  // Start as 1 to handle leading spaces
    int actual_length = 0;

    // Skips leading spaces
    while (*src == ' ' || *src == '\t') {
        src++
    };

    while (*src != '\0') {
        if (*src == ' ' || *src == '\t') {
            if (!space_flag && *(src + 1) != '\0') {  // Adds space only if not at end
                if (count >= len) return -1;
                *dest = ' ';
                dest++;
                count++;
                actual_length = count;
                space_flag = 1;
            }
        } else {
            if (count >= len) return -1;
            *dest = *src;
            dest++;
            count++;
            actual_length = count;
            space_flag = 0;
        }
        src++;
    }

    // Fills remainder with dots
    while (count < len) {
        *dest = '.';
        dest++;
        count++;
    }

    return actual_length;
}


void print_buff(char *buff, int len) {
    printf("Buffer:  [");
    for (int i = 0; i < len; i++) {
        putchar(*(buff + i));
    }
    printf("]\n"); 
}


void usage(char *exename){
    printf("usage: %s [-h|c|r|w|x] \"string\" [other args]\n", exename);

}

int count_words(char *buff, int len, int str_len) {
    (void)len;
    char *ptr = buff;
    int count = 0;
    int in_word = 0;

    for (int i = 0; i < str_len; i++) {
        if (*ptr != ' ' && *ptr != '.') {  
            if (!in_word) { 
                count++;  
                in_word = 1;
            }
        } else {
            in_word = 0;  
        }
        ptr++;
    }

    return count; 
}



//ADD OTHER HELPER FUNCTIONS HERE FOR OTHER REQUIRED PROGRAM OPTIONS

void reverse_string(char *buff, int len, int str_len) {
    (void)len;
    char *start = buff;
    char *end = buff + str_len - 1;
    char temp;

    while (start < end) {
        temp = *start;
        *start = *end;
        *end = temp;
        start++;
        end--;
    }
}


void word_print(char *buff, int len, int str_len) {
    (void)len;
    printf("Word Print\n----------\n");

    char *ptr = buff;
    int word_count = 1;
    int char_count = 0;
    int total_words = 0;

    while (ptr < buff + str_len) {
        if (*ptr != ' ' && *ptr != '.') {
            if (char_count == 0) {
                printf("%d. ", word_count);
                total_words++;
            }
            putchar(*ptr);
            char_count++;
        } else {
            if (char_count > 0) {
                printf(" (%d)\n", char_count);
                word_count++;
                char_count = 0;
            }
        }
        ptr++;
    }

    if (char_count > 0) {
        printf(" (%d)\n", char_count);
    }
    printf("\nNumber of words returned: %d\n", total_words);
}


void replace_string(char *buff, int len, int str_len, char *find, char *replace) {
    char *ptr;
    int find_len = 0, replace_len = 0;
    
    // Calculates find length
    ptr = find;
    while (*ptr != '\0') {
        find_len++;
        ptr++;
    }
    
    // Calculates replace length
    ptr = replace;
    while (*ptr != '\0') {
        replace_len++;
        ptr++;
    }
    
    // Finds substring in buffer
    char *match_start = NULL;
    ptr = buff;
    
    while (ptr < buff + str_len) {
        if (*ptr == *find) {
            char *temp_find = find;
            char *check = ptr;
            int matched = 1;
            
            while (*temp_find != '\0' && check < buff + str_len) {
                if (*check != *temp_find) {
                    matched = 0;
                    break;
                }
                temp_find++;
                check++;
            }
            
            if (matched) {
                match_start = ptr;
                break;
            }
        }
        ptr++;
    }

    if (!match_start) {
        printf("error: Search string not found\n");
        exit(3);  
    }

    int new_len = str_len - find_len + replace_len;

    if (new_len > len) {
        printf("error: Replacement would exceed buffer size\n");
        exit(3);  
    }

    // Moves characters manually
    char *src = match_start + find_len;
    char *dest = match_start + replace_len;
    int shift = replace_len - find_len;
    
    if (shift < 0) { // Shifts left
        while (src < buff + str_len) {
            *dest = *src;
            dest++;
            src++;
        }
    } else if (shift > 0) { // Shifts right
        char *end = buff + str_len - 1;
        char *new_end = buff + new_len - 1;
        
        while (end >= src) {
            *new_end = *end;
            new_end--;
            end--;
        }
    }

    // Copies replacement string
    char *replace_ptr = replace;
    ptr = match_start;
    while (*replace_ptr != '\0') {
        *ptr = *replace_ptr;
        ptr++;
        replace_ptr++;
    }

    // Fills remainder with dots
    while (ptr < buff + len) {
        *ptr = '.';
        ptr++;
    }
}



int main(int argc, char *argv[]){

    char *buff;             //placehoder for the internal buffer
    char *input_string;     //holds the string provided by the user on cmd line
    char opt;               //used to capture user option from cmd line
    int  rc;                //used for return codes
    int  user_str_len;      //length of user supplied string

    //TODO:  #1. WHY IS THIS SAFE, aka what if arv[1] does not exist?
    //      PLACE A COMMENT BLOCK HERE EXPLAINING
    
    /*
        Ans: The if and else statement written below ensures that the program does not access and invalid memory address.
        If argc < 2 or argv[1] does not exist then accessing argv[1] could lead to segmentation fault due to missing argv[1].
        The condition (*argv[1] != '-') ensures that the first argument after the program is a valid flag. If the user does not provide it
        then the program will print the usage message and exit.
    */

    if ((argc < 2) || (*argv[1] != '-')){
        usage(argv[0]);
        exit(1);
    }

    opt = (char)*(argv[1]+1);   //get the option flag

    //handle the help flag and then exit normally
    if (opt == 'h'){
        usage(argv[0]);
        exit(0);
    }

    //WE NOW WILL HANDLE THE REQUIRED OPERATIONS

    //TODO:  #2 Document the purpose of the if statement below
    //      PLACE A COMMENT BLOCK HERE EXPLAINING
    /*
    Ans: The if statement below checks if the option flag is 'c' or 'r' or 'w' or 'x'. 
    Without this check, attempting to access argv[2]  could result in undefined behavior if the user does not provide enough arguments.
    */
    if (argc < 3){
        usage(argv[0]);
        exit(1);
    }

    input_string = argv[2]; //capture the user input string

    //TODO:  #3 Allocate space for the buffer using malloc and
    //          handle error if malloc fails by exiting with a 
    //          return code of 99
    // CODE GOES HERE FOR #3

    buff = (char *)malloc(BUFFER_SZ * sizeof(char));

    if (buff == NULL) {
        printf("Memory allocation failed\n");
        exit(2);
    }

    user_str_len = setup_buff(buff, input_string, BUFFER_SZ);     //see todos
    
    if (user_str_len == -1) {
        printf("Error: Provided input string is too long\n");
        free(buff);
        exit(3);
    } else if (user_str_len == -2) {
        printf("Error: Empty input string\n");
        free(buff);
        exit(2);
    }

    switch (opt) {
        case 'c':
            rc = count_words(buff, BUFFER_SZ, user_str_len);
            if (rc < 0) {
                printf("Error counting words, rc = %d", rc);
                free(buff);
                exit(3);
            }
            printf("Word Count: %d\n", rc);
            print_buff(buff, BUFFER_SZ);
            free(buff);
            exit(0);
            break;

        case 'r':
            reverse_string(buff, BUFFER_SZ, user_str_len);
            print_buff(buff, BUFFER_SZ);
            free(buff);
            exit(0);
            break;

        case 'w':
            word_print(buff, BUFFER_SZ, user_str_len);
            print_buff(buff, BUFFER_SZ);
            free(buff);
            exit(0);
            break;

        case 'x':
            if (argc != 5) {
                printf("error: -x requires exactly 2 additional arguments\n");
                printf("Usage: %s -x \"string\" \"find\" \"replace\"\n", argv[0]);
                free(buff);
                exit(1);
            }
            replace_string(buff, BUFFER_SZ, user_str_len, argv[3], argv[4]);
            print_buff(buff, BUFFER_SZ);
            free(buff);
            exit(0);
            break;

        default:
            usage(argv[0]);
            free(buff);
            exit(1);
    }

    //TODO:  #6 Dont forget to free your buffer before exiting
    print_buff(buff,BUFFER_SZ);
    free(buff);
    exit(0);
}

//TODO:  #7  Notice all of the helper functions provided in the 
//          starter take both the buffer as well as the length.  Why
//          do you think providing both the pointer and the length
//          is a good practice, after all we know from main() that 
//          the buff variable will have exactly 50 bytes?
//  
//          PLACE YOUR ANSWER HERE

/*
Ans:
Providing both the buffer pointer and the length is a good practice because it allows the function to be have safe memory access. 
It helps to avoids buffer overflow by explicitly knowing the limit. It also prevents unexpected memory corruption. 
*/