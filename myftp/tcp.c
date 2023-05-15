#include <sys/stat.h>
#include "header.h"

int recv_header_packet(int sock1, char **pdata, struct myftph **ppkt_h) 
{
    int data_size;
    //struct myftph *pkt_h;
    mem_alloc(*ppkt_h, struct myftph, HEAD_SIZE, 1);
    if (recv(sock1, *ppkt_h, HEAD_SIZE, 0) < 0) {
        perror("recv");
        exit(1);
    }
    switch ((*ppkt_h)->type) {
        case CMD_ERR:
            switch((*ppkt_h)->code) {
                case SYNTAX:
                    printf("syntax error\n");
                    break;
                case UNDEFINED:
                    printf("undefined command\n");
                    break;
                case PROTOCOL:
                    printf("protocol error\n");
                    break;
            }
            return -1;
        case FILE_ERR:
            switch((*ppkt_h)->code) {
                case NOT_FOUND:
                    printf("No such file or directory\n");
                    break;
                case ACCESS:
                    printf("Permission denied\n");
                    break;
            }
            return -1;
        case UNKWN_ERR:
            printf("unkwon error\n");
            return -1;
    }
    data_size = ntohs((*ppkt_h)->length);
    if (data_size > 0) {
        mem_alloc(*pdata, char, data_size, 0);
        //memset(*pdata, 0, data_size + 1);
        if (recv(sock1, *pdata, data_size, 0) < 0) {
            perror("recv");
            exit(1);
        }
    }
    return data_size;
}
void send_header_packet(int sock1, struct myftph *pkt_h, int data_size)
{
    if (send(sock1, pkt_h, HEAD_SIZE + data_size, 0) < 0) {
        perror("send");
        close(sock1);
        exit(1);
    }
}
void send_file(int sock1, int fd)
{
    int data_size1, data_size2;
    char buf1[BUF_SIZE], buf2[BUF_SIZE];
    struct myftph *pkt_d;
    mem_alloc(pkt_d, struct myftph, HEAD_SIZE + DATA_SIZE, 1);
    //printf("fd in send_file : %d\n", fd);
    if ((data_size1 = read(fd, buf1, BUF_SIZE)) < 0) {
        perror("read");
        return;
    }
    //printf("data_size1 : %d\n", data_size1);
    while (1) {
        if ((data_size2 = read(fd, buf2, BUF_SIZE)) == 0) { // 終了処理
            pkt_d->type = DATA;
            pkt_d->code = LAST;
            pkt_d->length = htons(data_size1);
            memcpy(pkt_d->data, buf1, data_size1);
            //send_header_packet(sock1, pkt_d, data_size1);
            if (send(sock1, pkt_d, HEAD_SIZE + data_size1, 0) < 0) {
                perror("send");
                close(fd);
                close(sock1);
                exit(1);
            }
            //printf("LAST\n");
            //printf("pkt_d->code : %d\n", pkt_d->code);
            //printf("pkt_d->data : %s\n", pkt_d->data);
            break;
        } else if (data_size2 > 0) {
            pkt_d->type = DATA;
            pkt_d->code = MIDDLE;
            pkt_d->length = htons(data_size1);
            memcpy(pkt_d->data, buf1, data_size1);
            //send_header_packet(sock1, pkt_d, data_size1);
            if (send(sock1, pkt_d, HEAD_SIZE + data_size1, 0) < 0) {
                perror("send");
                close(fd);
                close(sock1);
                exit(1);
            }
            //printf("MIDDLE\n");
        } else {
            perror("read");
            return;
        }
        memcpy(buf1, buf2, data_size2);
        data_size1 = data_size2;
    }
    free(pkt_d);
}
int recv_file(int sock1, int fd) //recv_header_packet を書き直す
{
    int data_size;
    char *data;
    struct myftph *pkt_h;
    //mem_alloc(pkt_h, struct myftph, HEAD_SIZE, 1);
    //printf("entering recv_file\n");
    while (1) {
        /*
        if (recv(sock1, pkt_h, HEAD_SIZE, 0) < 0) {
            perror("recv");
            close(fd);
            close(sock1);
            exit(1);
        }
        data_size = ntohs(pkt_h->length);
        memset(data, 0, data_size);
        if (recv(sock1, data, data_size, 0) < 0) {
            perror("recv");
            close(fd);
            close(sock1);
            exit(1);
        }
        if (write(fd, data, data_size) < 0) {
            perror("write");
            return -1;
        }
        if (pkt_h->code == LAST)
            break;
        */
        if ((data_size = recv_header_packet(sock1, &data, &pkt_h)) < 0) {
            //printf("data_size : %d\n", data_size);
            close(fd);
            return -1;
        }
        /*
        printf("data_size : %d\n", data_size);
        printf("data : %s\n", data);
        */
        if (write(fd, data, data_size) < 0) {
            perror("write");
            return -1;
        }
        if (pkt_h->code == LAST)
            break;
    }
    return 0;
}