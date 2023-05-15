#include <stdio.h>
#include <stdlib.h>
#include <string.h>
/* inet_ntoa */
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
/* errno */
#include <errno.h>
/* signal */
#include <signal.h>

/* type */
#define QUIT                0x01
#define PWD                 0x02
#define CWD                 0x03
#define LIST                0x04
#define RETR                0x05
#define STOR                0x06
#define OK                  0x10
#define CMD_ERR             0x11
#define FILE_ERR            0x12
#define UNKWN_ERR           0x13
#define DATA                0x20
/* code */
#define PLANE_OK            0x00
#define SERVER_TO_CLIENT_OK 0x01
#define CLIENT_TO_SERVER_OK 0x02
#define SYNTAX              0x01
#define UNDEFINED           0x02
#define PROTOCOL            0x03
#define NOT_FOUND           0x00
#define ACCESS              0x01
#define UNKWN               0x05
#define LAST                0x00
#define MIDDLE              0x01

#define PORT_NUM 50021
#define BUF_SIZE  1024
#define DATA_SIZE 1024
#define HEAD_SIZE 4

#define mem_alloc(p, type, size, errno)                                        \
    do {                                                                       \
        if ((p = (type *)malloc(size)) == NULL) {                              \
            fprintf(stderr, "Could not allocate %lu memory\n", (size));        \
            exit(errno);                                                       \
        }                                                                      \
    } while (0)

struct myftph {
    uint8_t type;
    uint8_t code;
    uint16_t length;
    char data[0];
};
