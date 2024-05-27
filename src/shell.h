#ifndef _SHELL_
#define _SHELL_

#define TRUE 1
#define FALSE 0
#define ENTER_KEY '\n'
#define STDIN_FD 0
#define STDOUT_FD 1
#define STDERR_FD 2
#define TEMP_STDIN_FD 777
#define TEMP_STDOUT_FD 888
#define TEMP_STDERR_FD 999
#define EMPTY_COMMAND 55
#define KEY_UP 65
#define KEY_DOWN 66
#define DEL_LINE "\x1b[2K"
#define PROMPT "very_unique111"
#define SPECIAL_KEY_CODE 3
#define SPECIAL_BS_CODE 127
#define SKIP_CODE 55555

int compareStrings(const char *str1, const char *str2);
char *input_command();
void pipe_control(char *command);
void print_dynamic_prompt();
int init_history(char *command);
int my_getchar();
size_t find_space(char *paths);
int count_chars(char *str, char to_count);
void parse_str(char *str, char **splited, char *parse_by);
void clean_spaces(char **str_s, size_t size);
char *trim(char* command);
char *dynamic_cpy(char *str);

#endif