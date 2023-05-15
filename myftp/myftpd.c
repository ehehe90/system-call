/* fork, daemon */
#include <unistd.h>

#include "header.h"

int main(int argc, char *argv[])
{
    int sock0, sock1, pid, id = 0;
    struct sockaddr_in myskt, client_skt;
    socklen_t client_len;
    char buf[BUF_SIZE];
    //引数の処理
    // コネクション確立
    if ((sock0 = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket");
        exit(1);
    }
    memset(&myskt, 0, sizeof(myskt));
    myskt.sin_family = AF_INET;
    myskt.sin_port = htons(PORT_NUM);
    myskt.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind(sock0, (struct sockaddr *)&myskt, sizeof(myskt)) < 0) {
        perror("bind");
        exit(1);
    }
    if (listen(sock0, 5) != 0) {
        perror("listen");
        exit(1);
    }
    client_len = sizeof(struct sockaddr_in);
    while (1) {
        //printf("waiting connection\n");
        if ((sock1 = accept(sock0, (struct sockaddr *)&client_skt, &client_len)) < 0) {
            perror("accept");
            exit(1);
        }
        //printf("before_fork\n");
        if ((pid = fork()) < 0) {
            perror("fork");
            exit(1);
        } else if (pid == 0) { // 子プロセス
            daemon(1, 0);
            //printf("entering child proccess\n");
            while (1) {
                origin_proc(sock1);
                /*
                printf("%s (IP address : %s, port : %d)\n", buf, inet_ntoa(client_skt.sin_addr), ntohs(client_skt.sin_port));
                if (strcmp(buf, "exit") == 0)
                    exit(0);
                if (send(sock1, "Received!", 10, 0) < 0) {
                    perror("send");
                    exit(1);
                }
                */
            }
            /*
            printf("end of loop\n");
            close(sock1);
            exit(0);
            */
        } else { // 親プロセス
            //printf("entering parent proccess\n");
            close(sock1); // コメントアウトする？
            continue;
        }
    }
    close(sock0);
}