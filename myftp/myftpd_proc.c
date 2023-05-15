/* getcwd chdir */
#include <unistd.h>
/* open */
#include <fcntl.h>

#include "header.h"

void quit_proc(), pwd_proc(), cwd_proc(), list_proc(), retr_proc(), stor_proc();
struct type_table {
    int type;
    void (*func)(int, int);
} type_tbl[] = {
    {QUIT, quit_proc},
    {PWD, pwd_proc},
    {CWD, cwd_proc},
    {LIST, list_proc},
    {RETR, retr_proc},
    {STOR, stor_proc},
    {0, NULL}
};
void origin_proc(int sock1)
{
    int type;
    struct myftph *pkt_h;
    struct type_table *p;
    mem_alloc(pkt_h, struct myftph, HEAD_SIZE, 1);
    
    while (1) {
        if (recv(sock1, pkt_h, HEAD_SIZE, 0) < 0) {
            perror("recv");
            exit(1);
        }
        type = pkt_h->type;
        printf("type in origin_proc : %d\n", type);
        printf("code : %d, length : %d\n", pkt_h->code, ntohs(pkt_h->length));
        for (p = type_tbl; p->type; p++) {
            if (p->type == type) {
                (*p->func)(sock1, ntohs(pkt_h->length));
                break;
            }
        }
        if (p->type == 0) {
            fprintf(stderr, "unknown type : 0x%x\n", type);
            // なんか送信？
        }
    }
    free(pkt_h);
}

void quit_proc(int sock1, int data_size)
{
    /*
    type QUIT を受信した場合。
    終了処理を行う。
    */
    printf("quit_proc!\n");
    close(sock1);
    exit(0);
}
void pwd_proc(int sock1, int data_size)
{
    /*
    type PWD を受信した場合。
    type : OK, CODE : PLANE_OK, DATA : ワーキングディレクトリ を送信．
    */
    struct myftph *pkt_d;
    int i;
    char buf[BUF_SIZE], *data;
    printf("pwd_proc\n");
    // ワーキングディレクトリの取得
    if (getcwd(buf, BUF_SIZE) == NULL) {
        perror("getcwd");
        // 未定義のエラーを送信 or アクセス権限エラーを送信
        exit(1);
    }
    //printf("buf : %s\n", buf);
    for (i = 0; buf[i] != '\0'; i++) {}
    data_size = i;
    //printf("data_size : %d\n", data_size);
    // type : OK, CODE : PLANE_OK, DATA : ワーキングディレクトリ　を送信．
    mem_alloc(pkt_d, struct myftph, HEAD_SIZE + data_size, 1);
    pkt_d->type = OK;
    pkt_d->code = PLANE_OK;
    pkt_d->length = htons(data_size);
    for (i = 0; i < data_size; i++) {
        pkt_d->data[i] = buf[i];
    }
    /*
    if (send(sock1, pkt_d, HEAD_SIZE + data_size + 1, 1) < 0) {
        perror("send");
        exit(1);
    }
    */
    send_header_packet(sock1, pkt_d, data_size);
    //mem_alloc(data, char, data_size, 1);
    //send_message(sock1, OK, PLANE_OK, data_size, buf); // こいつは悪くない
    /*
    mem_alloc(pkt_d, struct myftph, HEAD_SIZE + data_size, 1);
    pkt_d->type = OK;
    pkt_d->code = PLANE_OK;
    pkt_d->length = htons(data_size);
    for (i = 0; i < data_size; i++) {
        pkt_d->data[i] = buf[i];
    }
    send_message(pkt_d, data_size, sock1);
    */
    free(pkt_d);
}
void cwd_proc(int sock1, int data_size)
{
    /*
    origin_proc で受信しなかったデータフィールドを受信。
    ディレクトリを変更
    type : OK, code : PLANE_OK を送信
    */
    char *data;
    struct myftph *pkt_h;
    printf("cwd_proc\n");
    // origin_proc で受信しなかったデータフィールドを受信。
    mem_alloc(data, char, data_size + 1, 1);
    memset(data, 0, data_size + 1);
    if (recv(sock1, data, data_size, 0) < 0) {
        perror("recv");
        exit(1);
    }
    // ディレクトリの変更
    mem_alloc(pkt_h, struct myftph, HEAD_SIZE, 1);
    if (chdir(data) < 0) {
        perror("chdir");
        if (errno == ENOENT) {
            pkt_h->type = FILE_ERR;
            pkt_h->code = NOT_FOUND;
        } else if (errno == EACCES) {
            pkt_h->type = FILE_ERR;
            pkt_h->code = ACCESS;
        } else if (errno == ENOTDIR) {
            pkt_h->type = CMD_ERR;
            pkt_h->code = SYNTAX;
        } else {
            pkt_h->type = UNKWN_ERR;
            pkt_h->type = UNKWN;
        }
    } else { // type : OK, code : PLANE_OK を送信
        pkt_h->type = OK;
        pkt_h->code = PLANE_OK;
    }
    pkt_h->length = htons(0);
    send_header_packet(sock1, pkt_h, 0);
    free(pkt_h);
}
void list_proc(int sock1, int data_size)
{
    char *data, **array_string;
    struct myftph *pkt_d, *pkt_h;
    int file_num, i, j;
    printf("list_proc\n");
    mem_alloc(data, char, data_size + 1, 1);
    memset(data, 0, data_size + 1);
    if (recv(sock1, data, data_size, 0) < 0) {
        perror("recv");
        exit(1);
    }
    mem_alloc(pkt_h, struct myftph, HEAD_SIZE, 1);
    pkt_h->length = htons(0);
    if ((file_num = print_files(data, &array_string)) < 0) {
        // errno に合わせてエラー処理
        if (errno == EACCES) {
            pkt_h->type = FILE_ERR;
            pkt_h->code = ACCESS;
        } else if (errno = ENOENT) {
            pkt_h->type = FILE_ERR;
            pkt_h->code = NOT_FOUND;
        } else if (errno == ENOTDIR) {
            pkt_h->type = CMD_ERR;
            pkt_h->code = SYNTAX;
        }
        send_header_packet(sock1, pkt_h, 0);
        free(pkt_h);
        return;
    } else {
        pkt_h->type = OK;
        pkt_h->code = SERVER_TO_CLIENT_OK;
    }
    send_header_packet(sock1, pkt_h, 0);
    mem_alloc(pkt_d, struct myftph, HEAD_SIZE + DATA_SIZE, 1);
    for (i = 0; i < file_num; i++) {
        pkt_d->type = DATA;
        if (i == file_num -1) {
            pkt_d->code = LAST;
        } else {
            pkt_d->code = MIDDLE;
        }
        for (j = 0; (*array_string)[j] != '\0'; j++) {
            pkt_d->data[j] = (*array_string)[j];
        }
        pkt_d->length = htons(j);
        //printf("pkd_d->data : %s\n", pkt_d->data);
        send_header_packet(sock1, pkt_d, j);
        *array_string++;
    }
    free(pkt_h);
}
void retr_proc(int sock1, int data_size)
{
    /*
    origin_proc で受信しなかったデータフィールドを受信。
    ヘッダパケットを送信
    ファイルを送信。
    */
    char *data;
    int fd;
    struct myftph *pkt_h;
    printf("retr_proc\n");
    mem_alloc(data, char, data_size + 1, 1);
    memset(data, 0, data_size + 1);
    if (recv(sock1, data, data_size, 0) < 0) {
        perror("recv");
        close(sock1);
        exit(1);
    }
    //printf("data : %s\n", data);
    // ヘッダパケットを送信。
    mem_alloc(pkt_h, struct myftph, HEAD_SIZE, 1);
    pkt_h->length = htons(0);
    if ((fd = open(data, O_RDONLY)) < 0) {
        perror("open");
        if (errno == ENOENT) {
            pkt_h->type = FILE_ERR;
            pkt_h->code = NOT_FOUND;
        } else if (errno == EACCES) {
            pkt_h->type = FILE_ERR;
            pkt_h->code = ACCESS;
        } else if (errno == EISDIR) {
            pkt_h->type = CMD_ERR;
            pkt_h->code = SYNTAX;
        } else {
            pkt_h->type = UNKWN_ERR;
            pkt_h->code = UNKWN;
        }
        send_header_packet(sock1, pkt_h, 0);
        close(fd);
        free(pkt_h);
        free(data);
        return;
    }
    pkt_h->type = OK;
    pkt_h->code = SERVER_TO_CLIENT_OK;
    send_header_packet(sock1, pkt_h, 0);
    // ファイルを送信
    send_file(sock1, fd);
    close(fd);
    free(pkt_h);
    free(data);
}
void stor_proc(int sock1, int data_size)
{
    /*
    origin_proc で受信しなかったデータフィールドを受信。
    ヘッダパケットを送信
    ファイルを受信。
    */
    char *data;
    int fd;
    struct myftph *pkt_h;
    printf("stor_proc\n");
    mem_alloc(data, char, data_size + 1, 1);
    memset(data, 0, data_size + 1);
    if (recv(sock1, data, data_size, 0) < 0) {
        perror("recv");
        close(sock1);
        exit(1);
    }
    // ヘッダパケットを送信
    mem_alloc(pkt_h, struct myftph, HEAD_SIZE, 1);
    pkt_h->length = htons(0);
    if ((fd = open(data, O_WRONLY | O_CREAT)) < 0) {
        perror("open");
        if (errno == ENOENT) {
            pkt_h->type = FILE_ERR;
            pkt_h->code = NOT_FOUND;
        } else if (errno == EACCES) {
            pkt_h->type = FILE_ERR;
            pkt_h->code = ACCESS;
        } else if (errno == EISDIR) {
            pkt_h->type = CMD_ERR;
            pkt_h->code = SYNTAX;
        } else {
            pkt_h->type = UNKWN_ERR;
            pkt_h->code = UNKWN;
        }
        send_header_packet(sock1, pkt_h, 0);
        close(fd);
        free(data);
        free(pkt_h);
        return;
    }
    pkt_h->type = OK;
    pkt_h->code = CLIENT_TO_SERVER_OK;
    send_header_packet(sock1, pkt_h, 0);
    // ファイルを受信
    if (recv_file(sock1, fd) < 0) {
        close(fd);
        return;
    }
    close(fd);
    if(chmod(data, S_IRUSR | S_IWUSR | S_IXUSR |
                       S_IRGRP | S_IXGRP |
                       S_IROTH | S_IXOTH) < 0) {
        perror("chmod");
    }
    free(pkt_h);
    free(data);
}