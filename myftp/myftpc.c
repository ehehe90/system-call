/* getaddrinfo */
#include <netdb.h>

#include "header.h"

#define AV_NUM 32
void split_cmd(char buf[], int *ac, char *av[]);
void quit_proc(), pwd_proc(), cd_proc(), dir_proc(), lpwd_proc(), lcd_proc(),
     ldir_proc(), get_proc(), put_proc(), put_proc(), help_proc();

struct command_table {
    char *cmd;
    void (*func)(int, char *[], int);
} cmd_tbl[] = {
    {"quit", quit_proc},
    {"pwd", pwd_proc},
    {"cd", cd_proc},
    {"dir", dir_proc},
    {"lpwd", lpwd_proc},
    {"lcd", lcd_proc},
    {"ldir", ldir_proc},
    {"get", get_proc},
    {"put", put_proc},
    {"help", help_proc},
    {NULL, NULL}
};
int s;
void sigint_handler(int signum)
{
    if (signum == SIGINT) {
        close(s);
        exit(0);
    }
}
int main(int argc, char *argv[])
{
    struct sockaddr_in server_skt;
    struct in_addr server_addr;
    struct addrinfo hints, *res;
    int err, ac;
    char buf[BUF_SIZE], *av[AV_NUM];
    struct command_table *p;
    struct sigaction sigint;
    // 引数の処理
    if (argc != 2) {
        fprintf(stderr, "Usage : ./myftpc <Host name of the server>\n");
        exit(1);
    }
    memset(&hints, 0, sizeof(hints));
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_family = AF_INET;
    if ((err = getaddrinfo(argv[1], NULL, &hints, &res)) < 0) {
        fprintf(stderr, "getaddrinfo : %s\n", gai_strerror(err));
        exit(1);
    }
    server_addr.s_addr = ((struct sockaddr_in *)(res->ai_addr))->sin_addr.s_addr;
    /*
    printf("ip address : %s\n", inet_ntoa(server_addr));
    */
    // SIGINT の設定
    if (sigemptyset(&sigint.sa_mask) < 0) {
        perror("sigaction");
        exit(1);
    }
    sigint.sa_handler = sigint_handler;
    if (sigaction(SIGINT, &sigint, NULL) < 0) {
        perror("sigaction");
        exit(1);
    }
    // コネクション確立
    if ((s = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket");
        exit(1);
    }
    memset(&server_skt, 0, sizeof(server_skt));
    server_skt.sin_family = AF_INET;
    server_skt.sin_port = htons(PORT_NUM);
    server_skt.sin_addr = server_addr;
    if (connect(s, (struct sockaddr *)&server_skt, sizeof(server_skt)) < 0) {
        perror("connect");
        exit(1); //exit でよいのか？ connect できるまでループするとかも考えられる。
    }
    while (1) {
        printf("myFTP%% ");
        fgets(buf, BUF_SIZE, stdin);
        //printf("buf : %s!\n", buf);
        split_cmd(buf, &ac, av);
        for (p = cmd_tbl; p->cmd; p++) {
            if (strcmp(av[0], p->cmd) == 0) {
                (*p->func)(ac, av, s);
                break;
            }
        }
        if (p->cmd == NULL)
            fprintf(stderr, "unknown command : %s\n", av[0]);
        if (p->cmd == "quit" && ac == 1)
            break;
    }
    close(s);
    freeaddrinfo(res);
    return 0;
}

void split_cmd(char buf[], int *ac, char *av[])
{
    int i = 0, len = 0, count = 0;

    while (buf[i] == ' ')
        i++;
    for (*ac = 0; i < BUF_SIZE; i++, len++) 
        if (buf[i] == ' ') {
            mem_alloc(av[*ac], char, len + 1, 1);
            memset(av[*ac], 0, len + 1);
            strncpy(av[*ac], &buf[i - len], len);
            len = 0;
            (*ac)++;
            while (buf[i] == ' ')
                i++;
        } else if (buf[i] == '\n') {
            while (buf[i] == ' ')
                count++;
            mem_alloc(av[*ac], char, len - count + 1, 2);
            memset(av[*ac], 0, len + 1);
            strncpy(av[*ac], &buf[i - len], len - count);
            (*ac)++;
            break;
        }
}