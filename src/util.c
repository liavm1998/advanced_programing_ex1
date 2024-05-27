/**
 * This is the utilities File for the program
 */

#include <stdio.h>
#include <limits.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include "shell.h"

extern size_t char_count;

/**
 * This function is printing the prompt of the shell with special colors
 *  including the path of the current working directory
 */
void print_prompt() { // this is a cool prompt printer

    char cwd[PATH_MAX]; // PATH_MAX taken from limits.h
    if (getcwd(cwd, PATH_MAX) != NULL) {
        printf( "\033[1;35m");
        printf( "λ");
        printf("\033[1;36m");
        printf(":%s$ ", cwd);
        printf("\033[0;37m");
    } else {
        perror("cwd ERROR");
        exit(1);
    }
}

void print_dynamic_prompt(){

    if (getenv(PROMPT)){
        printf("%s ", getenv(PROMPT));
    }
    else print_prompt();
    char_count = 0;
}

/**
 *
 * @param paths is a string represent file path
 * @return the first space char found in path
 */
size_t find_space(char *paths) {
    for (size_t i = 0; i < strlen(paths); ++i) {
        if (paths[i] == ' ')
            return i;
    }

    return -1;
}

/**
 *
 * @param str
 * @param to_count
 * @return return the occurrences number of the to_count char in str
 */
int count_chars(char *str, char to_count) {
    int count = 0;
    for (int i = 0; i < strlen(str); ++i) {
        if (str[i] == to_count)
            ++count;
    }
    return count;
}

/**
 *This function take a string and split it to separate string according to the parse_by char
 * @param str the origin string
 * @param splited - the returned separated strings
 * @param parse_by
 */
void parse_str(char *str, char **splited, char *parse_by) {
    int index = 0;
    char *temp;
    temp = strtok(str, parse_by);

    while (temp != NULL) {
        splited[index++] = temp;
        temp = strtok(NULL, parse_by);
    }

}

/***
 * This function take array of strings and clean spaces from all the strings from the prefix and suffix
 * @param str_s - the array of strings
 * @param size - the size of the array
 */
void clean_spaces(char **str_s, size_t size){
    for (int i = 0; i < size; ++i) {
        if (str_s[i] == NULL) continue;
        for (int j = 0; j < strlen(str_s[i]); ++j) {
            if (str_s[i][j] == ' '){
                for (int k = j; k < strlen(str_s[i]); ++k) {
                    str_s[i][k] = str_s[i][k + 1];
                }
                --j;
            }
            else break;
        }
    }
    for (int i = 0; i < size; ++i) {
        if (str_s[i] == NULL) continue;
        for (int j = strlen(str_s[i]) - 1; j >= 0; --j) {
            if (str_s[i][j] == ' '){
                str_s[i][j] = '\0';
            }
            else break;
        }
    }
}

char *dynamic_cpy(char *str){
    char *ret = (char *) malloc(sizeof (char) * (strlen(str) + 1));
    strcpy(ret, str);
    return ret;
}