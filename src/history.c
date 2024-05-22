#include <stdio.h>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include "shell.h"

size_t curr_history = 0;
size_t char_count = 0;
extern char exact_history_path[PATH_MAX];

int update_history(char *command){
    int fd = open(exact_history_path, O_WRONLY | O_APPEND);
    write(fd, command, strlen(command) * sizeof (char));
    write(fd, "\n", sizeof (char));
    close(fd);
    return 0;
}

int init_history(char *command){
    if (command != NULL) update_history(command);
    int fd = open(exact_history_path, O_RDONLY);
    char c;
    size_t cnt = 0;
    while (read(fd, &c, sizeof (char)) > 0){
        if (c == '\n') ++cnt;
    }
    return cnt;
}

/**
 * get line from the bash history file
 * @param line_num - line number
 * @param line_buff - buffer to save the line
 * @return 0 on success, -1 otherwise
 */
int get_history_line(int line_num, char* line_buff){
    int fd = open(exact_history_path, O_RDONLY);
    int cnt = 0;
    char curr;
    while (cnt != line_num && read(fd, &curr, sizeof (char) * 1) > 0){
        if (curr == '\n')
            cnt++;
    }
    curr = ' ';
    int i = 0;
    while(curr != '\n' && read(fd, &curr, sizeof (char)) > 0){
        line_buff[i++] = curr;
    }
    line_buff[i] = '\0';
    close(fd);
    return 0;
}

int up_down_handle(char *which){
    char buff[4096];
    int ret = FALSE;
    if (!strcmp(which, "down") && curr_history == init_history(NULL) - 1) { // in case we in the end of history
        printf("\33[2K\r"); // delete line
        print_dynamic_prompt();
        return SPECIAL_KEY_CODE;
    }

    if (!strcmp(which, "down") && curr_history == init_history(NULL)) return ret; // illegal

    if (!strcmp(which, "up") && curr_history > 0) {
        --curr_history;
    }
    else if (!strcmp(which, "down")) {
        ++curr_history;
    }
    ret = SPECIAL_KEY_CODE;
    get_history_line(curr_history, buff);
    printf("\33[2K\r"); // delete line
    print_dynamic_prompt();

    for (int i = strlen(buff) - 1; i >= 0; --i) { // for us to get it with getchar later to the command
        if(buff[i] == '\n') continue;
        ungetc(buff[i], stdin);
    }
    return ret;
}


int sanity_check(char c){
    if (c == '\033'){
        getchar(); // skip the [ char
        switch(getchar()) {
            case 'A':    // key up
                return up_down_handle("up");;
            case 'B':    // key down
                return up_down_handle("down");;
            case 'C':    // key right
//                printf("\033[1C");
                return FALSE;
            case 'D':    // key left
//                printf("\033[1D");
                return FALSE; /* TODO: <======  need to implement */
        }
    }
    if (c == 127) {
        if (char_count > 0) {
            --char_count;
            write(0, "\b \b", sizeof(char) * 3); // in case of delete char
        }
        return SPECIAL_BS_CODE;
    }
    return TRUE;
}

/** This function responsible to get char from stdin
 * but before we return the char we check if this is some kind of a special key
 * and if it is we will hook it and run sanity_check handle
 * @return
 */
int my_getchar(){
    struct termios oldattr, newattr;
    int ch;
    int err;
    tcgetattr( 0, &oldattr); // get terminal setting
    newattr = oldattr;
    newattr.c_lflag &= ~(ICANON | ECHO);  // set terminal setting to no canonical and no echo
    err = tcsetattr(0, TCSANOW, &newattr); // set terminal setting to no canonical and no echo
    if (err == -1) {
        fprintf(stderr, "tcsetattr error when setting terminal to no canonical and no echo\n");
        exit(1);
    }
    ch = getchar(); // get the char (this one return immediately and dont wait for \n)
    int check;
    while(!(check = sanity_check(ch))){
        ch = getchar();
    }
    err = tcsetattr(0, TCSANOW, &oldattr); // retrieve the last terminal setting
    if (err == -1) {
        fprintf(stderr, "tcsetattr error when retrieving the previous terminal setting\n");
        exit(1);
    }
    if (check == SPECIAL_BS_CODE || check == SPECIAL_KEY_CODE) return check;
    ++char_count;
    printf("%c", ch);
    fflush(stdin);
    return ch; // return the char in case its not special char
}