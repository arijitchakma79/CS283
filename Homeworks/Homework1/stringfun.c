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
        return -2;  // Invalid input string
    }

    char *temp = user_str;
    int str_length = 0;
    while (*temp != '\0') {
        str_length++;
        temp++;
    }
    if (str_length > len - 1) {
        return -1;  // String too long
    }

    char *src = user_str;
    char *dest = buff;
    int count = 0;
    int space_flag = 0;
    int actual_length = 0;

    while (*src != '\0') {
        if (*src == ' ' || *src == '\t') {
            if (!space_flag) {
                *dest = ' ';
                dest++;
                count++;
                space_flag = 1;
            }
        } else {
            *dest = *src;
            dest++;
            count++;
            space_flag = 0;
            actual_length = count; 
        }

        if (count >= len) {
            return -1;
        }
        src++;
    }

    // Fill remainder with dots
    while (count < len) {
        *dest = '.';
        dest++;
        count++;
    }

    return actual_length;  
}

void print_buff(char *buff, int len){
    printf("Buffer:  ");
    for (int i=0; i<len; i++){
        putchar(*(buff+i));
    }
    putchar('\n');
}

void usage(char *exename){
    printf("usage: %s [-h|c|r|w|x] \"string\" [other args]\n", exename);

}

int count_words(char *buff, int len, int str_len) {
    char *ptr = buff;
    int count = 0;
    int in_word = 0;

    for (int i = 0; i < str_len; i++) {
        if (*ptr != ' ' && *ptr != '.') {  // Found a valid character
            if (!in_word) { 
                count++;  // Start of a new word
                in_word = 1;
            }
        } else {
            in_word = 0;  // Word ended
        }
        ptr++;
    }

    return count; 
}


//ADD OTHER HELPER FUNCTIONS HERE FOR OTHER REQUIRED PROGRAM OPTIONS

void reverse_string(char *buff, int len, int str_len) {
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
    printf("Reversed String: ");
    for (int i = 0; i < str_len; i++) {
        putchar(*(buff + i));
    }
    putchar('\n');
}


void word_print(char *buff, int len, int str_len) {
    printf("Word Print\n");
    printf("----------\n");

    char *ptr = buff;
    int word_count = 1;
    int char_count = 0;

    while (ptr < buff + str_len) {
        if (*ptr != ' ' && *ptr != '.') {
            if (char_count == 0) {
                printf("%d. ", word_count);
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

    if (char_count > 0) { // Last word case
        printf(" (%d)\n", char_count);
    }
}

void replace_string(char *buff, int len, int str_len, char *find, char *replace) {
    // First calculate lengths of find and replace strings without strlen
    char *ptr;
    int find_len = 0;
    int replace_len = 0;
    
    // Get find length
    ptr = find;
    while (*ptr != '\0') {
        find_len++;
        ptr++;
    }
    
    // Get replace length
    ptr = replace;
    while (*ptr != '\0') {
        replace_len++;
        ptr++;
    }
    
    // Find the substring in buffer
    char *start = buff;
    char *end = buff + str_len;
    char *match_start = NULL;
    int found = 0;
    
    while (start < end && !found) {
        // Try to match the find string at current position
        char *curr = start;
        char *find_ptr = find;
        int matched = 1;
        
        while (*find_ptr != '\0' && curr < end) {
            if (*curr != *find_ptr) {
                matched = 0;
                break;
            }
            curr++;
            find_ptr++;
        }
        
        if (matched && *find_ptr == '\0') {
            match_start = start;
            found = 1;
        } else {
            start++;
        }
    }
    
    if (!found) {
        printf("error: Search string not found\n");
        return;
    }
    
    // Calculate new string length after replacement
    int new_len = str_len - find_len + replace_len;
    
    // Check if new string would exceed buffer
    if (new_len > len) {
        printf("error: Replacement would exceed buffer size\n");
        return;
    }
    
    // Shift the rest of the string if replace_len != find_len
    if (replace_len != find_len) {
        char *src = match_start + find_len;
        char *dest = match_start + replace_len;
        
        if (replace_len < find_len) {
            // Shift left
            while (src < buff + str_len) {
                *dest = *src;
                dest++;
                src++;
            }
        } else {
            // Shift right
            // Move from end to avoid overwriting
            src = buff + str_len - 1;
            dest = buff + str_len + (replace_len - find_len) - 1;
            
            while (src >= match_start + find_len) {
                *dest = *src;
                dest--;
                src--;
            }
        }
    }
    
    // Copy replacement string
    char *replace_ptr = replace;
    ptr = match_start;
    while (*replace_ptr != '\0') {
        *ptr = *replace_ptr;
        ptr++;
        replace_ptr++;
    }
    
    // Fill remainder with dots
    ptr = buff + new_len;
    while (ptr < buff + len) {
        *ptr = '.';
        ptr++;
    }
    
    // Print the modified string
    printf("Modified String: ");
    for (int i = 0; i < new_len; i++) {
        putchar(*(buff + i));
    }
    printf("\n");
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

    switch (opt){
        case 'c':
            rc = count_words(buff, BUFFER_SZ, user_str_len);  //you need to implement
            if (rc < 0){
                printf("Error counting words, rc = %d", rc);
                free(buff);
                exit(3);
            }
            printf("Word Count: %d\n", rc);
            break;

        //TODO:  #5 Implement the other cases for 'r' and 'w' by extending
        //       the case statement options
        case 'r':
            reverse_string(buff, BUFFER_SZ, user_str_len);
            break;

        case 'w':
            word_print(buff, BUFFER_SZ, user_str_len);
            break;
        case 'x':
            if (argc != 5) {
                printf("error: -x requires exactly 2 additional arguments\n");
                printf("Usage: %s -x \"string\" \"find\" \"replace\"\n", argv[0]);
                free(buff);
                exit(1);
            }
            replace_string(buff, BUFFER_SZ, user_str_len, argv[3], argv[4]);
            break;
        default:
        printf("error: Invalid option\n");
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