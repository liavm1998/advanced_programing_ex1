/**
 * The main file of the Shell
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <limits.h>
#include "shell.h"



extern size_t curr_history;
char exact_history_path[PATH_MAX];
pid_t main_pid;
void SIGINT_handler (int sig) {
    if (main_pid != getpid()) exit(0);
    printf("You typed Control-C!\n");
}


int main(){
    main_pid = getpid();
    signal(SIGINT, SIGINT_handler);

    if (getcwd(exact_history_path, PATH_MAX) == NULL){
        perror("init history path error");
        exit(1);
    }
    strcat(exact_history_path, "/.history");

    curr_history = init_history(NULL);
    char *command = NULL, *last_command = NULL;

    while (TRUE){
        print_dynamic_prompt();
        command = input_command();

        if (!strncmp("quit", command, 4)){
            free(command);
            if (last_command != NULL) free(last_command);
            exit(0);
        }
        else if (strcmp(command, "echo $?") == 0) {
            printf("%s\n", last_command);
            curr_history = init_history(command);
            continue;
        }
        else if (!strncmp(command, "!!", 2) && last_command != NULL){
            free(command);
            command = dynamic_cpy(last_command);
        }
        if (strlen(command) > 0){
            if (last_command != NULL) {
                free(last_command);
            }
            last_command = dynamic_cpy(command);

            pipe_control(command);
            curr_history = init_history(last_command);
        } else {
            curr_history = init_history(NULL);
        }
        free(command);
    }
}