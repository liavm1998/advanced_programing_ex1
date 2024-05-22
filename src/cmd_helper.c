#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include "shell.h"

int last_status = 0;
int stdin_revert = FALSE;
int stdout_revert = FALSE;
int stderr_revert = FALSE;

/**
This function get a command from sdtin in a dynamic way and retrieve it to the main program (the main free it all)
*/
char *input_command() {
    size_t size_lim = 100; // the initial limit size
    char *command = malloc(sizeof(char) * size_lim);
    if (command == NULL) {
        perror("malloc ERROR");
        exit(1);
    }
    char c = (char) my_getchar();
    size_t com_len = 0; // the actual command size
    while (c != ENTER_KEY) {

        if (c == SPECIAL_KEY_CODE) { // one of the arrows is pressed and brought history with him
            size_lim = 2;
            com_len = 0;
            command = realloc(command, sizeof(char) * size_lim);
            c = my_getchar();
            continue;
        }

        if (c == SPECIAL_BS_CODE) { // someone pressed backspace and remove a char
            if (com_len > 0)
                --com_len; // remove it also from the buffer
            c = my_getchar();
            continue;
        }


        command[com_len++] = c;

        if (com_len == size_lim) { // if the actual size meet the limitation increase by allocate *2
            command = realloc(command, sizeof(char) * (size_lim *= 2));
            if (command == NULL) {
                perror("realloc ERROR");
                exit(1);
            }
        }

        c = (char) my_getchar();
    }
    command[com_len++] = '\0';
    command = realloc(command, sizeof(char) * com_len); // shrink the command to his original size

    if (command == NULL) {
        perror("realloc ERROR");
        exit(1);
    }
    return command;
}


int my_read(char *name) {
    char buf[4096];
    int actual;
    if ((actual = read(0, buf, 4096)) == -1) return -1;
    buf[actual - 1] = '\0';
    return setenv(name, buf, 1);
}

int my_set_env(char *name_val) {
    size_t space = find_space(name_val);
    char name[512], val[512];
    strcpy(name, name_val);
    name[space] = '\0';
    strcpy(val, name_val + space + 3);
    return setenv(name, val, 1);
}

/**
 * This function execute echo with the support of printing environment variables
 * @param to_echo the string to echo
 */
int echo_cmd(char *to_echo) {
    for (int i = 0; i < strlen(to_echo); ++i) {
        if (to_echo[i] == '$') {
            char var[512];
            if (strlen(to_echo) == i + 1) {
                return -1;
            }
            ++i;
            for (int k = 0; to_echo[i] != ' ' && i < strlen(to_echo) && k < 512; ++i, ++k) {
                var[k] = to_echo[i];
            }
            if (var[0] == '?') printf("%d", last_status);
            else {
                char *val;
                if ((val = getenv(var)) != NULL) printf("%s", val);
                else perror("echo ERROR: variable not exist");
            }
        }
        printf("%c", to_echo[i]);
    }
    printf("\n");
    return 0;
}

/**
 * This function checks if some kind of a redirect is specify if yes - it will dup and which redirect code switch with the revert global vars
 * its the caller responsibility to reversed the dup of stdout/stdin later from the TEMP_STDOUT_FD/TEMP_STDIN_FD macro
 * @param command
 */
void redirect_apply(char *command) {
    int i;
    for (i = 0; i < strlen(command); ++i) {
        if (command[i] == '>') { // this one catch both > and >>
            int flag = (command[i + 1] == '>') ? O_APPEND | O_WRONLY : O_WRONLY;
            int move = (command[i + 1] == '>') ? 3 : 2;
            if (flag == O_WRONLY)
                unlink(command + i + move); // if the file exist and there is only one > remove the existing one
            int fd = open(command + i + move, flag | O_CREAT, 0644);
            dup2(STDOUT_FD, TEMP_STDOUT_FD);
            close(STDOUT_FD);
            dup(fd);
            close(fd);
            stdout_revert = TRUE;
            break;
        }
        if (command[i] == '<') {
            int fd = open(command + i + 2, O_RDONLY, 0644);
            dup2(STDIN_FD, TEMP_STDIN_FD);
            close(STDIN_FD);
            dup(fd);
            close(fd);
            stdin_revert = TRUE;
            break;
        }
        if (!strncmp(command + i, "2>", 2)) {
            int fd = open(command + i + 3, O_WRONLY | O_CREAT, 0644);
            dup2(STDERR_FD, TEMP_STDERR_FD);
            close(STDERR_FD);
            dup(fd);
            close(fd);
            stderr_revert = TRUE;
            break;
        }
    }
    command[i] = '\0';
}

void redirect_revert() {
    if (stdout_revert) {
        close(STDOUT_FD); // which currently is not the stdout
        dup(TEMP_STDOUT_FD);
        stdout_revert = FALSE;
    }
    if (stdin_revert) {
        close(STDIN_FD); // which currently is not the stdin
        dup(TEMP_STDIN_FD);
        stdin_revert = FALSE;
    }
    if (stderr_revert) {
        close(STDERR_FD);
        dup(TEMP_STDERR_FD);
        stderr_revert = FALSE;
    }
}

int if_session(char *statement) {
    char *then;
    then = input_command();
    if (strncmp("then", then, 4)) {
        printf("ERROR with if syntax (expected than got %s)\n", then);
        free(then);
        return -1;
    }
    free(then);
    char *if_command = input_command();
    char *else_command;

    char *fi_else = input_command();
    int f_else = 0;
    if (!strncmp("else", fi_else, 4)) {
        f_else = 1;
        else_command = input_command();
        free(fi_else);
        fi_else = input_command();
    }
    if (strncmp("fi", fi_else, 2)) {
        printf("ERROR with if syntax (expected fi or else got %s)\n", fi_else);
        free(fi_else);
        return -1;
    }

    free(fi_else);

    pipe_control(statement);

    if (last_status == 0) {
        pipe_control(if_command);
        free(if_command);
    } else if (f_else) {
        pipe_control(else_command);
        free(else_command);
    }
    return 0;
}

/**
 * This function responsible on the custom commands that we needed to implement
 * it will get a command and return the execute code if the command is a custom on
 * otherwise it will return SKIP_CODE for the simple_exec to know to activate execvp
 * @param command
 * @return
 */
int custom_cmd_handle(char *command) {
    if (*command == '$') {
        int ret = my_set_env(command + 1);
        if (ret) perror("ERROR in $ assigment");
        return ret;
    } else if (!strncmp(command, "read", 4)) {
        return my_read(command + 5);
    } else if (!strncmp(command, "prompt = ", 9)) {
        return setenv(PROMPT, command + 9, 1);
    } else if (!strncmp("echo ", command, 5)) {
        return echo_cmd(command + 5);
    } else if (!strncmp("cd", command, 2)) {
        int ret = chdir(command + 3);
        if (ret) perror("chdir ERROR");
        return ret;
    }
    return SKIP_CODE;
}

/**
 * This function checks if the command ends with & if yes it will remove the & from the command and return 1
 * otherwise it will return 0
 * @param command
 * @return
 */
int amper_check(char *command) {
    int i = strlen(command);
    while (i >= 0 && command[i] != '&') --i;
    if (command[i] == '&') {
        for (; i < strlen(command); ++i) command[i] = command[i + 1];
        command[++i] = '\0';
        return 1;
    }
    return 0;
}

/**
 * set the last status int to the exit code of pid
 * @param pid
 */
void set_last_status(pid_t pid) {
    int child_status;
    if (waitpid(pid, &child_status, 0) > 0) {
        last_status = WEXITSTATUS(child_status);
        if (last_status == 127) printf("ERROR: command not found\n");
    } else {
        perror("ERROR: waitpid failed in set_last_status\n");
    }
}

/**
 * This function is the main function to handle a single command (without pipes)
 * its have tow options
 * 1) running command trough a child process using execvp() and forK() (fork is optional in case of running from rec pipe)
 * 2) running commands using the custom commands handler
 * in both cases the last_status will update (unless its running through the pipe then its the pipe responsibility to update the last_status)
 * @param command - the command to run
 */
void simple_exec(char *command, int do_fork) {
    redirect_apply(command);
    int custom_commands_ret_code = custom_cmd_handle(command);
    if (custom_commands_ret_code != SKIP_CODE && !do_fork) {
        exit(custom_commands_ret_code);
    }
    if (custom_commands_ret_code != SKIP_CODE) {
        last_status = custom_commands_ret_code;
        redirect_revert();
        return;
    }

    unsigned int buff_size = count_chars(command, ' ') + 2;// +2 for the NULL
    char **splited_exec = (char **) malloc(sizeof(char *) * buff_size);
    parse_str(command, splited_exec, " ");
    splited_exec[buff_size - 1] = NULL;

    if (do_fork) {
        int pid = fork();
        if (pid < 0) {
            perror("ERROR with exec fork");
            exit(1);
        } else if (pid == 0) {
            execvp(splited_exec[0], splited_exec);
            exit(127); // in case execvp didnt succeed
        }
        set_last_status(pid);
        free(splited_exec);
        redirect_revert();
    } else {
        execvp(splited_exec[0], splited_exec);
        exit(127); // in case execvp failed
    }
}

/**
 * This is a recursive function to execute multiple commands using pipe line method
 * @param commands
 * @param size the size of the commands array
 * @return the output file descriptor of the command one before the last
 */
int rec_pipe(char *commands[], int size) {
    if (size == 0) {// if size == 0 then open pipe, run the first command (using simple_exec func)
        // and return the fd of the output (using dup2)
        int pipe1[2];

        if (pipe(pipe1) == -1) {
            perror("ERROR with pipe creation");
        }
        int pid = fork();
        if (pid < 0) {
            perror("ERROR with exec fork");
            exit(1);

        } else if (pid == 0) {
            // input from stdin (already done)
            // output to pipe1
            dup2(pipe1[1], 1);
            close(pipe1[0]);
            close(pipe1[1]);
            simple_exec(commands[0], FALSE);
        } else {
            set_last_status(pid);
            close(pipe1[1]);
            return pipe1[0];
        }
    } else {// if size != 0 then call again to rec_pipe with size-1 and when it returns the fd output
        // run your command (the size command) using simple_exec func and also return the pipe fd of the output
        int pipe1 = rec_pipe(commands, size - 1);
        int pipe2[2];
        if (pipe(pipe2) == -1) {
            perror("ERROR with pipe creation");
        }
        int pid = fork();
        if (pid < 0) {
            perror("ERROR with exec fork");
            exit(1);
        } else if (pid == 0) {
            // input from pipe1
            dup2(pipe1, 0);
            // output to pipe2
            dup2(pipe2[1], 1);
            // close fds
            close(pipe1);
            close(pipe2[0]);
            close(pipe2[1]);
            simple_exec(commands[size], FALSE);
        } else {
            set_last_status(pid);
            close(pipe1);
            close(pipe2[1]);
            return pipe2[0];
        }
    }
    return -1;
}

/**
 * This function is the main function to handle a command with & at the end
 * its exactly the same as simple_exec but it will run the command in a child process
 * without waiting for it to finish with the set_last_status function
 * (i didnt had power to make change to the simple_exec function to make it work with &)
 * @param command
 */
int amper_exec(char *command) {
    pid_t pid = fork();
    if (pid == 0) {
        if (custom_cmd_handle(command) == SKIP_CODE) {
            clean_spaces(&command, 1);
            unsigned int buff_size = count_chars(command, ' ') + 2;// +2 for the NULL
            char **splited_exec = (char **) malloc(sizeof(char *) * buff_size);
            parse_str(command, splited_exec, " ");
            splited_exec[buff_size - 1] = NULL;
            execvp(splited_exec[0], splited_exec);
            exit(127);
        }
        exit(0);
    } else if (pid > 0) {
        return 0;
    } else {
        perror("ERROR: fork failed in amper_exec");
        return -1;
    }
}

/**
 * This is the main pipe handling function it takes the command and check if it contain a pipeline
 * if no - just run the command using simple_exec func
 *, 0 if yes - parse by '|' the command to a commands array then call rec_pipe func on it
 *          and when it returns the one before the last output pipe fd
 *          then run the last command on this output (using simple_exec) and print it to stdout
 * @param command
 */
void pipe_control(char *command) {

    if (strncmp(command, "if ", 3) == 0) {
        if_session(command + 3);
        return;
    }

    int comm_size = count_chars(command, '|') + 1;

    int amper = amper_check(command);
    if (amper) {
        if (comm_size > 1){
            printf("ERROR: | syntax error (can't run background process with pipe)\n");
            last_status = -1;
        }
        else amper_exec(command);
        return;
    }

    if (comm_size - 1 == 0) {
        simple_exec(command, TRUE);
        return;
    }
    char **commands = (char **) malloc(sizeof(char *) * comm_size);

    parse_str(command, commands, "|");

    clean_spaces(commands, comm_size);

    int pipe = rec_pipe(commands, comm_size - 2);
    int pid = fork(); // just to make sure we dont run an empty command
    if (pid < 0) {
        perror("ERROR with exec fork");
        last_status = 127;
        return;
    } else if (pid == 0) {
        // input from pipe2
        dup2(pipe, 0);
        // output to stdout (already done)
        // close fds
        close(pipe);
        simple_exec(commands[comm_size - 1], TRUE);
        exit(0); // in case the exec failed or some
    }
    set_last_status(pid);
    close(pipe);
    free(commands);
}