#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include "header.h"

char *strip(char *s)
{
	char *result;
	while (*s == ' ')
		s++;
	result = s;
	while (*s != ' ' && *s != '\0')
		s++;
	*s = '\0';
	return result;
}	
int get_path(char *path_array[])
{
	char *buf;
	int i, j, pos = 0, real_buf_size = BUF_SIZE;
	if ((buf = getenv("PATH")) == NULL) {
		fprintf(stderr, "A enviroment variable name of which is PATH was not found\n");
		exit(1); //change
	}
	for (i = 0; buf[pos] != '\0'; i++) {
		real_buf_size = BUF_SIZE;
		if ((path_array[i] = (char *)malloc(sizeof(char) * real_buf_size)) == NULL) {
			fprintf(stderr, "Could not allocate %d byte memoery\n", sizeof(char) * real_buf_size);
			exit(1); //change
		}
		for (j = 0; buf[pos] != '\0';j++) {
			if (j == BUF_SIZE - 1) {
				real_buf_size += BUF_SIZE;
				if (realloc(buf, real_buf_size) == NULL) {
					fprintf(stderr, "Could not allocate %d byte memory\n", sizeof(char) *real_buf_size);
					exit(1); //change
				}
			}
			if (buf[pos] == ':') {
				path_array[i][j] = '\0';
				pos++;
				break;
			} else {
				path_array[i][j] = buf[pos++];
			}
		}
	}
	return i;
}
void bg_handler(int signum)
{
	int pid, status;
	struct sigaction dfl;
	extern int is_continue;
	dfl.sa_handler = SIG_DFL;
	if (sigemptyset(&dfl.sa_mask) < 0) {
		perror("sigemptyset");
		exit(1);
	}
	pid = waitpid(-1, &status, WNOHANG);
	printf("\nback-ground process just have done\n");
	is_continue = 1;
	if (sigaction(SIGCHLD, &dfl, NULL) < 0) {
		perror("sigaction");
		exit(1);
	}
}
//こいつで構文解析第2段やって，そこからcd_procとかに飛ばす
int origin_proc(char *cmd, int token, int is_after_pipe, char *in_files[], char *out_files[], int pfd_before[], int pfd_after[], int cmd_pos, int *p)
{
	char *av[AV_NUM];
	int i;
	//struct command_table *p;
	if (cmd[0] == '\0')  {
		return 0;
	}
	for (i = 0; i < AV_NUM; i++)
		av[i] = NULL;
	for (i = 0; *cmd != '\0'; i++) {
		if (i == AV_NUM - 1) { //最後はNULLにしたいからAN_NUM-1
			fprintf(stderr, "Too many arguments\n");
			return -1; 
		}
		while (*cmd == ' ')
			cmd++;
		av[i] = cmd;
		while (*cmd != ' ') { //タブの処理を追加
			if (*cmd == '\0')
				break;
			cmd++;
		}
		while (*cmd == ' ') {
			*cmd = '\0';
			cmd++;
		}
	}/*
	for (i = 0; i < AV_NUM && av[i] != NULL; i++) {
		printf("av[%d] : %s\n", i, av[i]);
	}*/
	// i+1が引数の数になる多分
	if (strcmp(av[0], "cd") == 0) {
		return cd_proc(av, i);
	} else if (strcmp(av[0], "pwd") == 0) {
		return pwd_proc(av, i);
	} else if (strcmp(av[0], "exit") == 0) {
		exit_proc();
	} else {
		return exe_proc(av, i, token, is_after_pipe, in_files, out_files, pfd_before, pfd_after, cmd_pos, p);
	}
}
int cd_proc(char *av[], int ac)
{
	char *home_path;
	//printf("ac : %d\n", ac);
	if (ac == 1) {
		if ((home_path = getenv("HOME")) == NULL) {
			fprintf(stderr, "A enviroment variable name of which is HOME was not found\n");
			return -1;
		}
		if (chdir(home_path) < 0) {
			perror("chdir");
			return -1;
		}
	} else {
		if (chdir(av[1]) < 0) {
			perror("chdir");
			return -1;
		}
	}
	return 0;
}
int pwd_proc(char *av[], int ac)
{
	char buf[BUF_SIZE];
	int i;
	if (getcwd(buf, BUF_SIZE) == NULL) {
		perror("getcwd");
		return -1;
	}
	printf("%s\n", buf);
	return 0;
}
int exit_proc()
{
	exit(0);
}
int exe_proc(char *av[], int ac, int token, int is_after_pipe, char *in_files[], char *out_files[], int pfd_before[], int pfd_after[], int cmd_pos, int *p)
{
	int i, j, k, path_num, *status, pid, fd, chpid, fd2, pgid;
	extern char **environ;
	char *path_array[CMD_NUM], *in_file, *out_file;
	struct sigaction ign, dfl, bg;

	if ((status = malloc(sizeof(int))) == NULL) {
		fprintf(stderr, "Could not allocate %d byte memory\n", sizeof(int));
		return -1;
	}
	ign.sa_handler = SIG_IGN;
	if (sigemptyset(&ign.sa_mask) < 0) {
		perror("sigemptyset");
		return -1;
	}
	if (sigaction(SIGINT, &ign, NULL) < 0) {
		perror("sigaction");
		return -1;
	}
	dfl.sa_handler = SIG_DFL;
	if (sigemptyset(&dfl.sa_mask) < 0) {
		perror("sigemptyset");
		return -1;
	}
	bg.sa_handler = bg_handler;
	if (sigemptyset(&bg.sa_mask) < 0) {
		perror("sigemptyset");
		return -1;
	}
	/*
	for (i = 0; av[i] != NULL; i++) {
		printf("av[%d] : %s\n", i, av[i]);
	}
	/*
	for (i = 0; in_files[i] != NULL; i++) {
		printf("in_files[%d] : %s!\n", i, in_files[i]);
	}
	for (i = 0; out_files[i] != NULL; i++) {
		printf("out_files[%d] : %s!\n", i, out_files[i]);
	}*/
	/*
	printf("path_num : %d\n", path_num);
	for (i = 0; i < path_num; i++) {
		printf("path_array[%d] : %s\n", i, path_array[i]);
	}*/
	if ((pid = fork()) < 0) {
		perror("fork");
		return -1;
	} else if (pid == 0) {
		// pgid の設定
		if (cmd_pos == 0) {
			if (setpgid(getpid(), getpid()) < 0) {
				perror("setpgid0");
				exit(1);
			}
		}
		//tpgid の設定
		if (token != TKN_BG) {
			if ((pgid = getpgid(getpid())) < 0) {
				perror("getpgid");
				exit(1);
			}
			if ((fd2 = open("/dev/tty", O_RDWR)) < 0) {
				perror("open");
				exit(1);
			}
			if (tcsetpgrp(fd2, pgid) < 0) {
				perror("tcsetpgrp");
				exit(1);
			}
			// Ctrl-c に対する設定
			if (sigaction(SIGINT, &dfl, NULL) < 0) {
				perror("sigaction");
				exit(1);
			}
		} else {
			if (sigaction(SIGINT, &ign, NULL) < 0) {
				perror("sigactino");
				exit(1);
			}
		}
		for (i = 0; i < NUM && in_files[i] != NULL; i++) { //redir_in の設定
			in_file = strip(in_files[i]);
			if ((fd = open(in_file, O_RDONLY, 0644)) < 0) {
				perror("open");
				exit(1); //change
			}
			if (i == NUM - 1 || in_files[i + 1] == NULL) {
				//printf("dup2(fd, 0)\n");
				if (dup2(fd, 0) < 0) {
					perror("dup");
					exit(1);
				}
				if (close(fd) < 0) {
					perror("close");
					exit(1);
				}
				break;
			}
		}
		for (i = 0; i < NUM && out_files[i] != NULL; i++) { //redir_out の設定
			//printf("1\n");
			if (i == NUM - 1 || out_files[i + 1] == NULL) {
				out_file = strip(out_files[i]);
				//printf("out_files[%d].strip:%s!\n", i, out_file);
				if ((fd = open(out_file, O_WRONLY | O_CREAT | O_TRUNC, 0644)) < 0) {
					perror("open");
					exit(1);
				}
				if (dup2(fd, 1) < 0) {
					perror("dup");
					exit(1);
				}
				if (close(fd) < 0) {
					perror("close");
					exit(1);
				}
				break;
			} else {
				out_file = strip(out_files[i]);
				//printf("suru-\n");
				if (open(out_file, O_WRONLY | O_CREAT | O_TRUNC, 0644) < 0) {
					perror("open");
					exit(1);
				}
			}
		}
		//pipe 処理
		/*
		printf("exe_proc\n");
		printf("pfd[0] : %d, pfd[1] : %d\n", pfd[0], pfd[1]);
		printf("av[0] : %s, is_after_pipe : %d\n", av[0], is_after_pipe);
		*/
		if (token == TKN_PIPE) { //パイプの前のプロセスの場合
			//printf("before pipe\n");
			if (dup2(pfd_before[1], 1) < 0) {
				perror("dup2");
				exit(1);
			}
			if (close(pfd_before[0]) < 0) {
				perror("close_before");
				exit(1);
			}
			if (close(pfd_before[1]) < 0) {
				perror("close_before");
				exit(1);
			}
		}
		if (is_after_pipe == AFTER_PIPE) { //パイプの後のプロセスの場合
			//printf("after pipe\n");
			if (dup2(pfd_after[0], 0) < 0) {
				perror("dup2");
				exit(1);
			}
			if (close(pfd_after[0]) < 0) {
				perror("close");
				exit(1);
			}
			if (close(pfd_after[1]) < 0) {
				perror("close");
				exit(1);
			}
			/*
			printf("setpgid in after, firstpid : %d\n", *p);
			if (setpgid(getpid(), *p) < 0) {
				perror("setpgid");
				exit(1);
			}*/
		}
		path_num = get_path(path_array);
		for (i = 0; i < path_num; i++) { // 実行できるまで，全部のパスをたどってexecve
			if (strlen(path_array[i]) + strlen(av[0]) + 2 > sizeof(path_array[i])) {
				if (realloc(path_array[i], sizeof(path_array[i]) + BUF_SIZE) == NULL) {
					fprintf(stderr, "Could not allocate %d byte memory\n", sizeof(path_array[i]) + BUF_SIZE);
					exit(1);
				}
			}
			for (j = 0; j < strlen(path_array[i]) + 1; j++) {
				if (path_array[i][j] == '\0') {
					path_array[i][j] = '/';
					for (k = 0; k < strlen(av[0]); k++) {
						path_array[i][j + k + 1] = av[0][k];
					}
					path_array[i][j + k + 1] = '\0';
					break;
				}
			}
			//printf("path_array[%d].rev : %s\n", i, path_array[i]);
			if (execve(path_array[i], av, environ) < 0) {
				if (i == (path_num - 1)) {
					perror("execve");
					exit(1);
				} else {
					continue;
				}
			}
			if (sigaction(SIGINT, &dfl, NULL) < 0) {
				perror("sigaction");
				exit(1);
			}
			exit(0);
		}
		for (i = 0; i < path_num; i++) {
			free(path_array[i]);
		}
	} else {
		//sigaction(SIGINT, &ign, NULL);
		*p = pid;
		if (is_after_pipe == AFTER_PIPE) {
			//printf("parent av[0] : %s\n", av[0]);
			if (close(pfd_after[0]) < 0) {
				perror("close");
				return -1;
			}
			if (close(pfd_after[1]) < 0) {
				perror("close");
				return -1;
			}
		}
		if (token == TKN_BG) {
			printf("process is in back-ground\n");
			if (sigaction(SIGCHLD, &bg, NULL) < 0) {
				perror("sigaction");
				return -1;
			}
			return 0; //added
		} else {
			if (wait(status) < 0) {
				perror("wait");
				return -1;
			}
		}
		if ((fd = open("/dev/tty", O_RDWR)) < 0) {
			perror("open");
			return -1;
		}
		if ((pgid == getpgid(getpid())) < 0) {
			perror("getpgid");
			return -1;
		}
		if (tcsetpgrp(fd, getpid()) < 0) {
			perror("tcsetpgrp");
			return -1;
		}
		if (*status == 0) {
			free(status);
			return 0;
		} else {
			free(status);
			return -1;
		}
		/*
		int tmp, *p_tmp;
		p_tmp = &tmp;
			if (wait(p_tmp) < 0) {
			perror("wait2");
			exit(1);
		}*/
	}
	return 0;
}

