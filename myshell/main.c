#include "header.h"

int is_continue;

void print_new_line(int signum)
{
	printf("\n");
	is_continue = 1;
}

int main(int argc, char *argv[], char *evnp[])
{
	char  *cmds[CMD_NUM], *in_files[NUM], *out_files[NUM];
	int i, cmd_pos, token_pos, in_files_pos, out_files_pos, pfd_pos, pipe_count, tokens[CMD_NUM], is_after_pipe, pfds[NUM][2], is_blank, firstpid = 0, *p;
	struct sigaction ign, new_line, dfl;

	new_line.sa_handler = print_new_line;
	if (sigemptyset(&new_line.sa_mask) < 0) {
		perror("sigemptyset");
		exit(1);
	}
	if (sigaction(SIGINT, &new_line, NULL) < 0) {
		perror("sigaction");
		exit(1);
	}
	ign.sa_handler = SIG_IGN;
	if (sigemptyset(&ign.sa_mask) < 0)  {
		perror("sigemptyset");
		exit(1);
	}
	dfl.sa_handler = SIG_DFL;
	if (sigemptyset(&dfl.sa_mask) < 0) {
		perror("sigemptyset");
		exit(1);
	}
	p = &firstpid;
	/*
	if (sigaction(SIGINT, &ign, NULL) < 0) {
		perror("sigaction");
		exit(1);
	}
	*/
	if (sigaction(SIGTTOU, &ign, NULL) < 0) {
		perror("sigaction");
		exit(1);
	}
	while (1) {
		is_continue = 0;
		if (sigemptyset(&new_line.sa_mask) < 0) {
			perror("sigemptyset");
			exit(1);
		}
		if (sigaction(SIGINT, &new_line, NULL) < 0) {
			perror("sigaction");
			exit(1);
		}
		/*
		if (sigaction(SIGCHLD, &dfl, NULL) < 0) {
			perror("sigaction");
			exit(1);
		}
		*/
		for (i = 0; i < CMD_NUM; i++) {
			cmds[i] = NULL;
		}
		is_after_pipe = NOT_AFTER_PIPE;
		printf("$ ");
		i = 0;
		while (1) {
			if (i == CMD_NUM) {
				fprintf(stderr, "Too many commands\n");
				exit(1);
			}
			if ((tokens[i] = gettoken(&cmds[i])) == TKN_EOL)
				break;
			if (is_continue)
				break;	
			i++;
		}
		if (is_continue)
			continue;
		for (cmd_pos = 0, is_continue = 0; cmd_pos < CMD_NUM && cmds[cmd_pos] != NULL; cmd_pos++) { // トークンの間に空白文字しかなかった場合の対処
			for (i = 0, is_blank = 1; cmds[cmd_pos][i] != '\0'; i++) {
				if (cmds[cmd_pos][i] != ' ') {
					is_blank = 0;
					break;
				}
			}
			if (cmd_pos > 0 && tokens[cmd_pos - 1] == TKN_BG) {
				is_blank = 0;
			}
			if (is_blank == 1) {
				is_continue = 1;
				break;
			}
			/*
			if ((is_blank == 1) && (tokens[cmd_pos] == TKN_EOF))
				is_blank = 0;
			*/
		}
		if (is_continue == 1) {
			fprintf(stderr, "syntax error\n");
			continue;
		}
		for (cmd_pos = 0, token_pos = 0, pfd_pos = 1; cmd_pos < CMD_NUM && cmds[cmd_pos] != NULL;) { //cmd の要素ごとにパイプ，リダイレクト，execveの処理
			//in_files, otut_files の設定
			for (i = 0; i < NUM; i++) {
				in_files[i] = NULL;
				out_files[i] = NULL;
			}
			in_files_pos = 0;
			out_files_pos = 0;
			while (tokens[token_pos] == TKN_REDIR_IN || tokens[token_pos] == TKN_REDIR_OUT) {
				if (tokens[token_pos] == TKN_REDIR_IN) {
					if (in_files_pos == NUM) {
						fprintf(stderr, "Too many files\n");
						exit(1);
					}
					in_files[in_files_pos++] = cmds[++token_pos];
				} else {
					if (out_files_pos == NUM) {
						fprintf(stderr, "Too many files\n");
						exit(1);
					}
					out_files[out_files_pos++] = cmds[++token_pos];
				}
			}
			//in_files, out_files の設定終わり
			//printf("cmd_pos : %d\n", cmd_pos);
			//printf("token_pos : %d\n", token_pos);
			if (tokens[token_pos] == TKN_PIPE) {
				//printf("pipe! pfd_pos : %d\n", pfd_pos);
				if (pipe(pfds[pfd_pos]) < 0) {
					perror("pipe");
					break;//change
				}/*
				printf("main\n");
				printf("pfds[%d][0] : %d\n", pfd_pos, pfds[pfd_pos][0]);
				printf("pfds[%d][1] : %d\n", pfd_pos, pfds[pfd_pos][1]);
				*/
			}
			/*
			for (i = 0, is_blank = 1; cmds[cmd_pos][i] != '\0'; i++)
				if (cmds[cmd_pos][i] != ' ')
					is_blank = 0;
			if (is_blank && (tokens[token_pos] != TKN_EOF)) {
				fprintf(stderr, "syntax error\n");
				break;
			}
			*/
			//printf("cmds[%d] : %s, is_after_pipe : %d\n", cmd_pos, cmds[cmd_pos], is_after_pipe);
			//printf("firstpid in main : %d\n", firstpid);
			if (origin_proc(cmds[cmd_pos], tokens[token_pos], is_after_pipe, in_files, out_files, pfds[pfd_pos], pfds[pfd_pos - 1], cmd_pos, p) < 0)
				break;
			/*
			if (is_after_pipe == AFTER_PIPE)
				pfd_pos++;
			*/
			if (tokens[token_pos] == TKN_PIPE) {
				is_after_pipe = AFTER_PIPE;
				pfd_pos++;
			}
			else
				is_after_pipe = NOT_AFTER_PIPE;
			//printf("origin_proc!\n");
			cmd_pos = ++token_pos;
		}
		for (cmd_pos = 0; cmd_pos < NUM && cmds[cmd_pos] != NULL; cmd_pos++) {
			free(cmds[cmd_pos]);
		}
	}
	return 0;
}		
int gettoken(char **cmd)
{
	char c, *buf;
	int pos = 0, i = 0, real_buf_size = BUF_SIZE;
	if ((buf = (char *)malloc(sizeof(char) * real_buf_size)) == NULL) {
		fprintf(stderr, "Could not allocate %d byte memory\n", sizeof(char) * BUF_SIZE);
		exit(1);
	}
	while (1) {
		c = getc(stdin);
		switch (c) {
			case ('<'):
				buf[pos] = '\0';
				*cmd = buf;
				return TKN_REDIR_IN;
				break;
			case ('>'):
				buf[pos] = '\0';
				*cmd = buf;
				return TKN_REDIR_OUT;
				break;
			case ('|'):
				buf[pos] = '\0';
				*cmd = buf;
				return TKN_PIPE;
				break;
			case ('&'):
				buf[pos] = '\0';
				*cmd = buf;
				return TKN_BG;
				break;
			case ('\n'):
				buf[pos] = '\0';
				*cmd = buf;
				return TKN_EOL;
				break;
			case (EOF):
				buf[pos] = '\0';
				*cmd = buf;
				return TKN_EOF;
				break;
			default:
				if (i == BUF_SIZE - 1) {
					real_buf_size += BUF_SIZE;
					if (realloc(buf, real_buf_size) == NULL) {
						fprintf(stderr, "Could not allocate %d byte memory\n", sizeof(char) * real_buf_size);
						exit(1);
					}
					i = 0;
				}
				buf[pos] = c;
				i++;
				pos++;
		}
	}
}

