#include <stdio.h>
#include <stdlib.h>
#include <string.h>
/* inet_aton のための   */
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
/* errno */
#include <errno.h>
/* signal 関係 */
#include <signal.h>
/* setitimer */
#include <sys/time.h>
/* pause */
#include <unistd.h>
/* オーダー変換 */
#include <netinet/in.h>

#define BUF_SIZE 1024
#define PORT_NUM 51230
#define mem_alloc(ptr, type, size, errno) \
do { \
	if ((ptr = (type *)malloc(sizeof(type) * size)) == NULL) { \
		fprintf(stderr, "Could not allocate %dbyte memory\n", sizeof(type) * size); \
		exit(errno); \
	}\
} while (0)

struct dhcph  {
	uint8_t type;
	uint8_t code;
	uint16_t ttl;
	in_addr_t address;
	in_addr_t netmask;
};
void print_message(struct dhcph *pdh)
{
	char *buf;
	struct in_addr addr, netmask;
	switch(pdh->type) {
		case 1:
			buf = "DISCOVER";
			break;
		case 2:
			buf = "OFFER";
			break;
		case 3:
			buf = "REQUEST";
			break;
		case 4:
			buf = "ACK";
			break;
		case 5:
			buf = "RELEASE";
			break;
	}
	addr.s_addr = pdh->address;
	netmask.s_addr = pdh->netmask;
	printf("(Type : %s, Code : %d, ttl : %d, IP address : %s, ", buf, pdh->code, ntohs(pdh->ttl), inet_ntoa(addr));
	printf("Nemask : %s)\n", inet_ntoa(netmask));
}