#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>

#define CMD_NUM 16
#define AV_NUM 16
#define NUM 16
#define BUF_SIZE 256
#define TKN_NORMAL 0
#define TKN_REDIR_IN 1
#define TKN_REDIR_OUT 2
#define TKN_PIPE 3
#define TKN_BG 4
#define TKN_EOL 5
#define TKN_EOF 6
#define AFTER_PIPE 7
#define NOT_AFTER_PIPE 8

int origin_proc(char *cmd, int token, int is_after_pipe, char *in_files[], char *out_files[], int pfd_before[], int pfd_after[], int cmd_pos, int *p);
int cd_proc(char *av[], int ac);
int pwd_proc(char *av[], int ac);
int exit_proc();
int exe_proc(char *av[], int ac, int token, int is_after_pipe, char *in_files[], char *out_files[], int pfd_before[], int pfd_after[], int cmd_pos, int *p);
