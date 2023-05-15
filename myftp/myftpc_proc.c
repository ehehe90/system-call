/* getcwd chdir */
#include <unistd.h>
/* stat */
//#include <sys/stat.h>
/* opendir */
#include <sys/types.h>
#include <dirent.h>
/* open */
#include <fcntl.h>
#include "header.h"


void quit_proc(int ac, char *av[], int sock1) // done
{
    /*
    type : QUIT を送信。
    終了処理はこの関数ではなく、main で行う。
    */
    struct myftph *pkt_h;
    // 引数の確認
    //printf("quit_proc!\n");
    if (ac != 1) {
        fprintf(stderr, "Usage : quit\n");
        return;
    }
    // type : QUIT を送信。
    mem_alloc(pkt_h, struct myftph, HEAD_SIZE, 1);
    pkt_h->type = QUIT;
    pkt_h->code = 0;
    pkt_h->length = htons(0);
    send_header_packet(sock1, pkt_h, 0);
    free(pkt_h);
    //close(sock1);
    //exit(1);
}
void pwd_proc(int ac, char *av[], int sock1) // done
{
    /*
    type : PWD を送信．
    サーバから送られてくるメッセージを受信，
    */
    struct myftph *pkt_h;
    char *data, path[DATA_SIZE + 1]; //, **pdata;
    int data_size;
    // 引数の確認
    //printf("pwd_proc!\n");
    if (ac != 1) {
        fprintf(stderr, "Usage : pwd\n");
        return;
    }
    // type : PWD を送信
    //mem_alloc(pkt_h, struct myftph, HEAD_SIZE, 1);
    if ((pkt_h = (struct myftph *)malloc(HEAD_SIZE)) == NULL) {
        fprintf(stderr, "malloc error\n");
        exit(1);
    }
    pkt_h->type = PWD;
    pkt_h->code = 0;
    pkt_h->length = htons(0);
    send_header_packet(sock1, pkt_h, 0);
    // ヘッダのみ受信
    /*
    if (recv(sock1, pkt_h, HEAD_SIZE, 0) < 0) {
        perror("recv");
        exit(1);
    }
    data_size = ntohs(pkt_h->length);
    */
    // ヘッダパケットを受信。
    if ((data_size = recv_header_packet(sock1, &data, &pkt_h)) < 0) {
        free(pkt_h);
        free(data);
        return;
    }
    /*
    mem_alloc(data, char, data_size + 1, 1);
    memset(data, 0, data_size + 1);
    // データを受信
    if (recv(sock1, data, data_size, 0) < 0) {
        perror("recv");
        exit(1);
    }
    */
    memset(path, 0, DATA_SIZE + 1);
    memcpy(path, data, data_size);
    printf("%s\n", path); 
    free(pkt_h);
    free(data);
}
void cd_proc(int ac, char *av[], int sock1) //done
{
    /*
    type : CWD, code : 0, DATA : パス名 を送信。
    サーバからのパケットを受信。
    */
    int data_size = 0;
    struct myftph *pkt_d;
    // 引数の確認
    //printf("cd_proc!\n");
    if (ac != 2) {
        fprintf(stderr, "Usage : cd path\n");
        return;
    }
    mem_alloc(pkt_d, struct myftph, HEAD_SIZE + DATA_SIZE, 1);
    while (*av[1] != '\0') {
        pkt_d->data[data_size] = *(av[1]++);
        data_size++;
    }
    /*
    printf("pkt_d->data : %s\n", pkt_d->data);
    printf("data_size : %d\n", data_size);
    */
    // type : CWD, code : 0, DATA : パス名を送信
    pkt_d->type = CWD;
    pkt_d->code = 0;
    pkt_d->length = htons(data_size);
    send_header_packet(sock1, pkt_d, data_size);
    // サーバからのパケットを受信
    recv_header_packet(sock1, NULL, &pkt_d);
    free(pkt_d);
}
void dir_proc(int ac, char *av[], int sock1)
{
    /*
    */
    //printf("dir_proc!\n");
    char *path, *data, *s;
    int data_size = 0;
    struct myftph *pkt_d;
    if (ac == 1) {
        path = ".";
    } else if (ac == 2) {
        path = av[1];
    } else {
        fprintf(stderr, "Usage : dir [path]\n");
        return;
    }
    mem_alloc(pkt_d, struct myftph, HEAD_SIZE + DATA_SIZE, 1);
    while (*path != '\0') {
        pkt_d->data[data_size] = *(path++);
        data_size++;
    }
    pkt_d->type = LIST;
    pkt_d->code = 0;
    pkt_d->length = htons(data_size);
    send_header_packet(sock1, pkt_d, data_size);
    if (recv_header_packet(sock1, NULL, &pkt_d) < 0) {
        free(pkt_d);
        return;
    }
    mem_alloc(s, char, DATA_SIZE + 1, 1);
    while (1) {
        if ((data_size = recv_header_packet(sock1, &data, &pkt_d)) < 0) {
            free(pkt_d);
            free(s);
            free(data);
            return;
        }
        memset(s, 0, DATA_SIZE + 1);
        memcpy(s, data, data_size);
        printf("%s\n", s);
        if (pkt_d->code == LAST) {
            break;
        }
    }
    free(pkt_d);
    free(s);
    free(data);
}
void lpwd_proc(int ac, char *av[], int sock1)
{
    char buf[BUF_SIZE];
    //printf("lpwd_proc!\n");
    if (ac != 1) {
        fprintf(stderr, "Usage : lpwd\n");
        return;
    }
    if (getcwd(buf, BUF_SIZE) == NULL) {
        perror("getcwd");
        return;
    }
    printf("%s\n", buf);
}
void lcd_proc(int ac, char *av[], int sock1)
{
    //printf("lcd_proc!\n");
    if (ac != 2) {
        fprintf(stderr, "Usage : lcd path\n");
        return;
    }
    if (chdir(av[1]) < 0) {
        perror("chdir");
        return;
    }
}
void ldir_proc(int ac, char *av[], int sock1)
{
    char *path, **string_array;
    int file_num, i;
    //printf("ldir_proc!\n");
    if (ac == 1) {
        path = ".";
    } else if (ac == 2) {
        path = av[1];
    } else {
        fprintf(stderr, "Usage : ldir path\n");
        return;
    }
    if ((file_num = print_files(path, &string_array)) < 0) {
        return;
    }
    for (i = 0; i < file_num; i++) {
        printf("%s\n", *string_array);
        *string_array++;
    } 
}
void get_proc(int ac, char *av[], int sock1)
{
    /*
    type : RETR, code : 0, DATA : ソースのパス名を送信
    ヘッダパケットの受信
    サーバからのファイルを受信
    */
    char *src_path, *dst_path;
    int data_size = 0, fd;
    struct myftph *pkt_d;
    //printf("get_proc!\n");
    // 引数の確認
    if (ac == 2) {
        src_path = dst_path = av[1];
    } else if (ac == 3) {
        src_path = av[1];
        dst_path = av[2];
    } else {
        fprintf(stderr, "Usage : get path [path]\n");
        return;
    }
    mem_alloc(pkt_d, struct myftph, HEAD_SIZE + DATA_SIZE, 1);
    while (*src_path != '\0') {
        pkt_d->data[data_size] = *(src_path++);
        data_size++;
    }
    // type : RETR, code : 0, DATA : ソースのパス名を送信
    pkt_d->type = RETR;
    pkt_d->code = 0;
    pkt_d->length = htons(data_size);
    send_header_packet(sock1, pkt_d, data_size);
    // ヘッダパケットの受信
    if (recv_header_packet(sock1, NULL, &pkt_d) < 0) {
        //close(fd);
        free(pkt_d);
        return;
    }
    if ((fd = open(dst_path, O_WRONLY|O_CREAT)) < 0) {
        perror("open");
        free(pkt_d);
        return;
    }
    // さーばからのファイルを受信
    if (recv_file(sock1, fd) < 0) {
        close(fd);
        free(pkt_d);
        return;
    }
    close(fd);
    free(pkt_d);
    if(chmod(dst_path, S_IRUSR | S_IWUSR | S_IXUSR |
                       S_IRGRP | S_IXGRP |
                       S_IROTH | S_IXOTH) < 0) {
        perror("chmod");
    }
}
void put_proc(int ac, char *av[], int sock1)
{
    /*
    type : STOR, code : 0, DATA : デストのパス名 を送信
    ヘッダパケットの受信
    ファイルの送信
    */
    char *src_path, *dst_path;
    int data_size = 0, fd;
    struct myftph *pkt_d;
    //printf("put_proc!\n");
    // 引数の確認
    if (ac == 2) {
        src_path = dst_path = av[1];
    } else if (ac == 3) {
        src_path = av[1];
        dst_path = av[2];
    } else {
        fprintf(stderr, "Usage : put path [path]\n");
        return;
    }
    if ((fd = open(src_path, O_RDONLY)) < 0) {
        perror("open");
        return;
    }
    mem_alloc(pkt_d, struct myftph, HEAD_SIZE + DATA_SIZE, 1);
    while (*dst_path != '\0') {
        pkt_d->data[data_size] = *(dst_path++);
        data_size++;
    }
    /*
    printf("src_path : %s\n", src_path);
    printf("fd in main : %d\n", fd);
    */
    // type : STOR, code : 0, DATA : デストのパス名 を送信
    pkt_d->type = STOR;
    pkt_d->code = 0;
    pkt_d->length = htons(data_size);
    send_header_packet(sock1, pkt_d, data_size);
    //printf("1\n");
    // ヘッダパケットの受信
    if (recv_header_packet(sock1, NULL, &pkt_d) < 0) {
        //close(fd);
        free(pkt_d);
        return;
    }
    // ファイルの送信
    send_file(sock1, fd);
    close(fd);
    free(pkt_d);
}
void help_proc(int ac, char *av[], int sock1)
{
    if (ac != 1) {
        fprintf(stderr, "Usage : help\n");
        return;
    }
    printf("NAME\n      help\n");
    printf("SYNOPSIS\n      help\n");
    printf("DESCRIPTION\n      help formats and displays the manual page\n");
    printf("----------------------------------------------\n");

    printf("NAME\n      quit\n");
    printf("SYNOPSIS\n      quit\n");
    printf("DESCRIPTION\n      quit quits this program\n");
    printf("----------------------------------------------\n");
    
    printf("NAME\n      pwd\n");
    printf("SYNOPSIS\n      pwd\n");
    printf("DESCRIPTION\n      pwd prints the full path of the current working directory of the server\n");
    printf("----------------------------------------------\n");
    
    printf("NAME\n      cd\n");
    printf("SYNOPSIS\n      cd path\n");
    printf("DESCRIPTION\n      cd changes the current working directory of the server\n");
    printf("----------------------------------------------\n");
    
    printf("NAME\n      dir\n");
    printf("SYNOPSIS\n      dir [path]\n");
    printf("DESCRIPTION\n      dir prints all the files in path of the server\n");
    printf("----------------------------------------------\n");
    
    printf("NAME\n      lpwd\n");
    printf("SYNOPSIS\n      lpwd\n");
    printf("DESCRIPTION\n      lpwd prints the full path of the current working directory of the client\n");
    printf("----------------------------------------------\n");
    
    printf("NAME\n      lcd\n");
    printf("SYNOPSIS\n      lcd path\n");
    printf("DESCRIPTION\n      lcd changes the current working directory of the client\n");
    printf("----------------------------------------------\n");
    
    printf("NAME\n      ldir\n");
    printf("SYNOPSIS\n      ldir [path]\n");
    printf("DESCRIPTION\n      ldir prints all the files in path of the client\n");
    printf("----------------------------------------------\n");
    
    printf("NAME\n      get\n");
    printf("SYNOPSIS\n      get path [path]\n");
    printf("DESCRIPTION\n      get transport file of the server to the client\n");
    printf("----------------------------------------------\n");
    
    printf("NAME\n      put\n");
    printf("SYNOPSIS\n      put path [path]\n");
    printf("DESCRIPTION\n      put transport file of the client to the server\n");
    printf("----------------------------------------------\n");
}