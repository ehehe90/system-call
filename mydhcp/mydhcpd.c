#include "header.h"
/* このファイルのみでつかいそうな define */
#define CONTINUE 0
#define RECV_DISC_EVENT 1
#define NO_IP_EVENT 2
#define RECV_REQU_ALLOC_EVENT 3
#define RECV_TOUT_EVENT 4
#define RECV_UNKN_EVENT 5
#define RECV_UNEX_EVENT 6
#define RECV_REQU_EXT_EVENT 7
#define RECV_RELE_EVENT 8
#define USE_TOUT_EVENT 9
#define RECV_WRONG_REQU_ALLOC_EVENT 10
#define INIT_STATE 21
#define WAIT_REQU_STATE 22
#define IN_USE_STATE 23
#define REQU_TOUT_STATE 24
//#define TERM_STATE 25 // 多分いらない
#define HEAD 30
#define TAIL 31
/* 両方のファイルで使いそうな define  (struct dhcph を両方で使いそう)*/
struct addr {
	struct addr *fp;
	struct addr *bp;
	struct in_addr addr;
	struct in_addr netmask;
	//int is_used;
};
struct addr addr_head;
struct client {
	struct client *fp; // ホストオーダ
	struct client *bp; // ホストオーダ
	int state; // ホストオーダ
	int ttlcounter; // ホストオーダ
	struct in_addr id;
	struct in_addr addr;
	struct in_addr netmask;
	in_port_t port;
	uint16_t ttl;
	struct addr *pa; // ホストオーダ
};
struct client client_head, *pc;
int ttl;
void f_act1(), f_act2(), f_act3(), f_act4(), f_act5(), f_act6(), f_act7();
struct proctable {
	int state;
	int event;
	void (*func)(int s);
} ptab[] = {
	{INIT_STATE, RECV_DISC_EVENT, f_act1}, // 1
	{INIT_STATE, NO_IP_EVENT, f_act2}, //18
	{WAIT_REQU_STATE, RECV_REQU_ALLOC_EVENT, f_act3}, // 2
	{WAIT_REQU_STATE, RECV_TOUT_EVENT, f_act4}, //5
	{WAIT_REQU_STATE, RECV_WRONG_REQU_ALLOC_EVENT, f_act7}, // 8
	{WAIT_REQU_STATE, RECV_UNKN_EVENT, f_act5}, // 9
	{WAIT_REQU_STATE, RECV_UNEX_EVENT, f_act5}, // 10
	{IN_USE_STATE, RECV_REQU_EXT_EVENT, f_act6}, // 3
	{IN_USE_STATE, RECV_RELE_EVENT, f_act5}, // 4
	{IN_USE_STATE, USE_TOUT_EVENT, f_act5}, // 4
	{IN_USE_STATE, RECV_UNKN_EVENT, f_act5}, // 12
	{IN_USE_STATE, RECV_UNEX_EVENT, f_act5}, // 13
	{REQU_TOUT_STATE, RECV_REQU_ALLOC_EVENT, f_act3}, // 6
	{REQU_TOUT_STATE, RECV_WRONG_REQU_ALLOC_EVENT, f_act7}, // 14
	{REQU_TOUT_STATE, RECV_TOUT_EVENT, f_act5}, // 15
	{REQU_TOUT_STATE, RECV_UNKN_EVENT, f_act5}, // 16
	{REQU_TOUT_STATE, RECV_UNEX_EVENT, f_act5}, // 17
	{0, 0, NULL}
};
struct statetable {
	int state;
	char *p;
} stab[] = {
	{INIT_STATE, "INIT_STATE"},
	{WAIT_REQU_STATE, "WAIT_REQU_STATE"},
	{IN_USE_STATE, "IN_USE_STATE"},
	{REQU_TOUT_STATE, "REQU_TOUT_STATE"},
	{0, NULL}
};
void sigalrm_handler(int sig_num)
{
	struct client *pc;
	if (sig_num == SIGALRM) 
		for (pc = client_head.fp; pc != &client_head; pc = pc->fp)
			pc->ttlcounter -= 1;
}
void insert_addr_list(struct addr *p, int where)
{
	struct addr *h = &addr_head;
	if (where == HEAD) {
		h->fp->bp = p;
		p->fp = h->fp;
		p->bp = h;
		h->fp = p;
	} else {
		h->bp->fp = p;
		p->fp = h;
		p->bp = h->bp;
		h->bp = p;
	}
}
void del_addr_from_list(struct addr *p)
{
	p->fp->bp = p->bp;
	p->bp->fp = p->fp;
	p->fp = NULL;
	p->bp = NULL;
}
void insert_client_list(struct client *p, int where)
{
	struct client *h = &client_head;
	if (where == HEAD) {
		h->fp->bp = p;
		p->fp = h->fp;
		p->bp = h;
		h->fp = p;
	} else {
		h->bp->fp = p;
		p->fp = h;
		p->bp = h->bp;
		h->bp = p;
	}
}
struct client *search_client(in_addr_t client_addr, in_port_t client_port) 
{
	struct client *p;
	for (p = client_head.fp; p != &client_head; p = p->fp)
		if (p->id.s_addr == client_addr && p->port == client_port)
			return p;
	return NULL;
}
void send_message(int s, struct dhcph *pdh)
{
	struct sockaddr_in skt;
	memset(&skt, 0, sizeof(skt));
	skt.sin_family = AF_INET;
	skt.sin_port = pc->port;
	skt.sin_addr.s_addr = pc->id.s_addr;
	printf("----------------------------------------\n");
	printf("Sending message to the server (IP address : %s, port : %d)\n", inet_ntoa(skt.sin_addr), ntohs(skt.sin_port));
	print_message(pdh);
	if (sendto(s, pdh, sizeof(struct dhcph), 0, (struct sockaddr *)&skt, sizeof(struct sockaddr_in)) < 0) {
		perror("sendto");
		exit(1);
	}
}
void print_state_change(int before_state, int after_state)
{
	struct statetable *st;
	for (st = stab; st->state; st++) {
		if (st->state == before_state) {
			break;
		}
	}
	printf("----------------------------------------\n");
	printf("Changing state of the client (IP address : %s, port :%d) from %s ", inet_ntoa(pc->id), ntohs(pc->port), st->p);
	for (st = stab; st->state; st++) {
		if (st->state == after_state) {
			break;
		}
	}
	printf("to %s\n", st->p);
}
void terminate(struct client *pc)
{
	pc->bp->fp = pc->fp;
	pc->fp->bp = pc->bp;
	if (pc->pa != NULL) {
		insert_addr_list(pc->pa, TAIL);
		printf("----------------------------------------\n");
		printf("Releasing IP address : %s, ", inet_ntoa(pc->addr));
		printf("Netmask : %s\n", inet_ntoa(pc->netmask));
		printf("----------------------------------------\n");
		printf("Terminating process for the client (IP address : %s, port : %d)\n", inet_ntoa(pc->id), ntohs(pc->port));
	}
	free(pc);
}
void f_act1(int s) 
{
	/* 
	1
	INIT_STATE で discovery を受信し，IP が残っている場合に呼び出される関数．
	offer (OK) を送る
	*/
	struct dhcph send_dh;
	send_dh.type = 2;
	send_dh.code = 0;
	send_dh.ttl = pc->ttl;
	send_dh.address = pc->addr.s_addr;
	send_dh.netmask = pc->netmask.s_addr;
	send_message(s, &send_dh);
	print_state_change(pc->state, WAIT_REQU_STATE);
	pc->state = WAIT_REQU_STATE;
	//printf("Changing state of the client (IP address : %s, port : %d) to WAIT_REQU_STATE\n", inet_ntoa(pc->id), ntohs(pc->port));
}
void f_act2(int s)
{
	/*
	18 
	IINIT_STATE で discovery を受信し，IP が残ってない場合に呼び出される関数.
	offer (NG) を送って，クライアントを terminate に遷移させる．
	*/
	struct dhcph send_dh;
	send_dh.type = 2;
	send_dh.code = 1;
	send_dh.ttl = ntohs(0);
	send_dh.address = ntohl(0);
	send_dh.netmask = ntohl(0);
	send_message(s, &send_dh);
	printf("----------------------------------------\n");
	printf("Terminating process for the client (IP address : %s, port : %d)\n", inet_ntoa(pc->id), ntohs(pc->port));
	free(pc);
}
void f_act3(int s)
{
	/*
	2, 6
	WAIT_REQU_STATE,REQU_TOUT_STATE で request を受信し，code が 割り当て要求の場合に呼び出される関数．
	ACK (OK) を送って，クライアントを IN_USE_STATE に遷移させる．
	*/
	struct dhcph send_dh;
	send_dh.type = 4;
	send_dh.code = 0;
	send_dh.ttl = pc->ttl;
	send_dh.address = pc->addr.s_addr;
	send_dh.netmask = pc->netmask.s_addr;
	send_message(s, &send_dh);
	print_state_change(pc->state, IN_USE_STATE);
	pc->ttlcounter = ttl;
	pc->state = IN_USE_STATE;
	//printf("Changing state of the client (IP address : %s, port : %d) to IN_USE_STATE\n", inet_ntoa(pc->id), ntohs(pc->port));
}
void f_act4(int s)
{
	/*
	5
	WAIT_REQU_STATE で request 受信がタイムアウトした場合に呼び出される関数．
	offer (OK) を再送して，クライアントを REQU_TOUT_STATE に遷移させる
	*/
	struct dhcph send_dh;
	send_dh.type = 2;
	send_dh.code = 0;
	send_dh.ttl = pc->ttl;
	send_dh.address = pc->addr.s_addr;
	send_dh.netmask = pc->netmask.s_addr;
	send_message(s, &send_dh);
	print_state_change(pc->state, REQU_TOUT_STATE);
	pc->state = REQU_TOUT_STATE;
	pc->ttlcounter = ttl / 2;
	//printf("Changing state of the client (IP address : %s, port : %d) to REQU_TOUT_STATE\n", inet_ntoa(pc->id), ntohs(pc->port));
}
void f_act5(int s)
{
	/*
	4, 9, 10, 12, 13, 15, 16, 17
	クライアントを終了．
	*/
	terminate(pc);
}
void f_act6(int s)
{
	/*
	3
	IN_USE_STATE で request を受信し，code が使用時間延長要求の場合．
	ack (OK) を送信して，ttlcounter を　reset
	*/
	struct dhcph send_dh;
	send_dh.type = 4;
	send_dh.code = 0;
	send_dh.ttl = pc->ttl;
	send_dh.address = pc->addr.s_addr;
	send_dh.netmask = pc->netmask.s_addr;
	send_message(s, &send_dh);
	pc->ttlcounter = ttl;
}
void f_act7(int s)
{
	/*
	8, 14
	request を受信したが，メッセージ内の IP フィールドの値が offer で送った IP と違う場合．
	ack (NG) を送信して，クライアントを終了．
	*/
	struct dhcph send_dh;
	send_dh.type = 4;
	send_dh.code = 4;
	send_dh.ttl = ntohs(0);
	send_dh.address = pc->addr.s_addr;
	send_dh.netmask = pc->netmask.s_addr;
	send_message(s, &send_dh);
	terminate(pc);
}
int wait_event(int s, struct dhcph *receive_pdh)
{
	int data_size, type, code;
	struct sockaddr_in skt;
	socklen_t sktlen = sizeof(skt);
	struct addr *pa;

	printf("----------------------------------------\n");
	printf("Waiting message\n");
	while (1) {
		memset(receive_pdh, 0, sizeof(struct dhcph));
		if (data_size = recvfrom(s, receive_pdh, sizeof(struct dhcph), MSG_DONTWAIT, (struct sockaddr *)&skt, &sktlen) < 0) { // エラーの場合
			if (errno == EAGAIN) { // メッセージを受信しないエラーの場合
				pause();
				for (pc = client_head.fp; pc != &client_head; pc = pc->fp) { // ttl timeout してないかの判定 
					if (pc->ttlcounter < 0) {
						if (pc->state == WAIT_REQU_STATE) {
							printf("----------------------------------------\n");
							printf("Receving timeout for the client (IP address : %s, port : %d)\n", inet_ntoa(pc->id), htons(pc->port));
							return RECV_TOUT_EVENT; // 5
						} else if (pc->state == IN_USE_STATE) {
							printf("----------------------------------------\n");
							printf("ttl of IP address for the client (IP address : %s, port : %d) has just passed\n", inet_ntoa(pc->id), htons(pc->port));
							return USE_TOUT_EVENT; //4
						} else if (pc->state == REQU_TOUT_STATE) {
							printf("----------------------------------------\n");
							printf("Receving timeout for the client (IP address : %s, port : %d\n", inet_ntoa(pc->id), htons(pc->port));
							return RECV_TOUT_EVENT; // 15
						}
					}
				}
			} else { // 他のエラーの場合
				perror("recvfrom");
				exit(1);
			}
		} else { // エラーではない (メッセージを正しく受信した) 場合
			type = (int)(receive_pdh->type);
			code = (int)(receive_pdh->code);
			printf("----------------------------------------\n");
			printf("Recieved message ");
			if ((pc = search_client(skt.sin_addr.s_addr, skt.sin_port)) == NULL) { //新しいクライアントの場合
				printf("from a new client (IP address : %s, port : %d)\n", inet_ntoa(skt.sin_addr), ntohs(skt.sin_port));
				print_message(receive_pdh);
				if (type == 1) { // メッセージが discover の場合はクライアントの作成・設定 + WAIT_REQU_STATE に遷移 (途中)
					pa = addr_head.fp;
					if (pa == &addr_head) { // 使われてない addr がない場合
						mem_alloc(pc, struct client, 1, 1);
						pc->pa = NULL;
						pc->state = INIT_STATE;
						pc->id = skt.sin_addr;
						pc->port = skt.sin_port;
						return NO_IP_EVENT; // 18 f_act2
					} else {
						mem_alloc(pc, struct client, 1, 1);
						insert_client_list(pc, TAIL);
						pc->state = INIT_STATE;
						pc->ttlcounter = ttl / 2;
						pc->id = skt.sin_addr;
						pc->addr = pa->addr;
						pc->netmask = pa->netmask;
						pc->port = skt.sin_port;
						pc->ttl = htons(ttl);
						pc->pa = pa;
						del_addr_from_list(pa);
						printf("----------------------------------------\n");
						printf("Allocating IP address : %s, ", inet_ntoa(pc->addr));
						printf("Netmask : %s, ttl : %d to the client ", inet_ntoa(pc->netmask), ntohs(pc->ttl));
						printf("(IP address : %s, port : %d)\n", inet_ntoa(skt.sin_addr), ntohs(skt.sin_port));
						return RECV_DISC_EVENT; // 1 f_act1
					}
				} else if (type == 2 || type == 3 || type == 4 || type == 5) { //メッセージが discover じゃない場合はなにもしない
					printf("Message type is not discover\n");
					printf("----------------------------------------\n");
					printf("Terminating process for this client\n");
					return CONTINUE;
				} else {
					printf("Don't match to DHCP message format\n");
					printf("----------------------------------------\n");
					printf("Terminating process for this client\n");
					return CONTINUE;
				}
			} else {
				printf("from the already existing client (IP address : %s, port :%d)\n", inet_ntoa(skt.sin_addr), ntohs(skt.sin_port));
				print_message(receive_pdh);
				switch (pc->state) { // (途中)
					case INIT_STATE: // 基本的に起きないはず
						printf("----------------------------------------\n");
						fprintf(stderr, "Something wrong is in state change\n");
						fprintf(stderr, "Terminating proceess for this client\n");
						terminate(pc);
						return CONTINUE;
						break;
					case WAIT_REQU_STATE:
						if (type == 3) { // メッセージが request の場合
							if (code == 2) { // 割り当て要求の場合
								if ((pc->addr.s_addr == receive_pdh->address) && (pc->netmask.s_addr == receive_pdh->netmask)) { // offer で送った IP と request 中の IP が同じ場合
									if (htons(receive_pdh->ttl) <= ttl) {
										return RECV_REQU_ALLOC_EVENT; // 2 f_act3
									} else {
										printf("ttl in REQUEST message is larger than ttl in OFFER message\n");
										printf("Request denied\n");
										return RECV_WRONG_REQU_ALLOC_EVENT; // 8
									}
								} else {
									if (pc->addr.s_addr != receive_pdh->address) {
										printf("IP address in REQUEST message is not the same as that in OFFER message\n");
									} else  if (pc->netmask.s_addr != receive_pdh->netmask) {
										printf("Netmask in REQUEST message is not the same as that in OFFER message\n");
									}
									printf("Request denied\n");
									return RECV_WRONG_REQU_ALLOC_EVENT; // 8
								}
							} else {
								printf("Values in the message is unexpected\n");
								return RECV_UNEX_EVENT; // 10
							}
						} else if (type == 1 || type == 2 || type == 4 || type == 5) { // メッセージが request ではない場合
							printf("Values in the message is unexpected\n");
							return RECV_UNEX_EVENT; // 10
						} else {
							printf("Don't match to DHCP message format\n");
							return RECV_UNKN_EVENT;
						}
						break;
					case IN_USE_STATE:
						if (type == 3) { // メッセージが request の場合
							if (code == 3) { // 使用期間延長要求の場合
								return RECV_REQU_EXT_EVENT; // 3
							} else {
								printf("Values in the message is unexpected\n");
								return RECV_UNEX_EVENT; // 13
							}
						} else if (type == 5) { // メッセージが release の場合
							if (receive_pdh->address == pc->addr.s_addr) { // IP フィールドの値がそのクライアントが使用している IP の場合
								return RECV_RELE_EVENT; // 4
							} else { // そうでない場合
								printf("The value in IP field is not the same as this client uses\n");
								return CONTINUE; // 7
							}
						} else if (type == 1 || type == 2 || type == 4){ // メッセージが request ではない場合
							printf("Values in the message is unexpected\n");
							return RECV_UNEX_EVENT; // 13
						} else {
							printf("Don't match to DHCP message format\n");
							return RECV_UNKN_EVENT;
						}
						break;
					case REQU_TOUT_STATE:
						if (type == 3) { // メッセージが request の場合
							if (code == 2) { // 割り当て要求の場合
								if ((pc->addr.s_addr == receive_pdh->address) && (pc->netmask.s_addr == receive_pdh->netmask)) { // offer で送った IP と request 中の IP が同じ場合
									if (htons(receive_pdh->ttl) <= ttl) {
										return RECV_REQU_ALLOC_EVENT; // 6
									} else {
										printf("ttl in REQUEST message is larger than ttl in OFFER message\n");
										printf("Request denied\n");
										return RECV_WRONG_REQU_ALLOC_EVENT; // 14
									}
								} else {
									if (pc->addr.s_addr != receive_pdh->address) {
										printf("IP address in REQUEST message is not the same as that in OFFER message\n");
									} else if (pc->netmask.s_addr != receive_pdh->netmask) {
										printf("Netmask in REQUEST message is not the same as that in OFFER message\n");
									}
									printf("Request denied\n");
									return RECV_WRONG_REQU_ALLOC_EVENT; // 14
								}
							} else {
								printf("Values in the message is unexpected\n");
								return RECV_UNEX_EVENT; //17
							} 
						} else if (type == 1|| type == 2 || type == 4 || type == 5) {
							printf("Values in the message is unexpected\n");
							return RECV_UNEX_EVENT; // 17
						} else {
							printf("Don't match to DHCP message format\n");
							return RECV_UNKN_EVENT;
						}
						break;
					default:
						return RECV_UNKN_EVENT; // 9, 12, 16
						break;
				}
			}
		}
	}
}
int main(int argc, char *argv[])
{
	struct proctable *pt;
	struct addr *pa;
	int event, i, addr_num = 0, s;
	FILE *fp;
	char buf[BUF_SIZE];
	struct sockaddr_in myskt;
	struct dhcph receive_dh;
	struct sigaction alrm;
	struct itimerval timer;

	/* プログラム自体の初期化 */
	// リストの初期化
	client_head.fp = &client_head;
	client_head.bp = &client_head;
	addr_head.fp = &addr_head;
	addr_head.bp = &addr_head;
	// 引数の確認
	if (argc != 2) {
		fprintf(stderr, "Usage: mydhcps <config-filename>\n");
		exit(1);
	}
	if ((fp = fopen(argv[1], "r")) == NULL) {
		fprintf(stderr, "Cannot open file (%s)\n", argv[1]);
		exit(1);
	}
	// 設定フィルの読み込み
	fgets(buf, BUF_SIZE, fp);
	sscanf(buf, "%d", &ttl);
	printf("IP address's time-to-live : %d\n", ttl);
	while (fgets(buf, BUF_SIZE, fp) != NULL) {
		char tmp1[BUF_SIZE], tmp2[BUF_SIZE];
		sscanf(buf, "%s %s", tmp1, tmp2);
		//printf("%d : IP address : %s, netmask : %s\n", addr_num, tmp1, tmp2);
		printf("%d---IP address : %s, netmask : %s---\n", addr_num, tmp1, tmp2);
		mem_alloc(pa, struct addr, 1, 1);
		if (inet_aton(tmp1, &(pa->addr)) == 0) {
			fprintf(stderr, "IP address in %s:%d is invalid\n", argv[1], addr_num + 1);
			exit(1);
		}
		if (inet_aton(tmp2, &(pa->netmask)) == 0) {
			fprintf(stderr, "Netmask in %s:%d is not invalid\n", argv[1], addr_num + 1);
			exit(1);
		}
		insert_addr_list(pa, TAIL);
		addr_num++;
	}
	// ソケット関係の設定
	if ((s = socket(PF_INET, SOCK_DGRAM, 0)) < 0) {
		perror("socket");
		exit(1);
	}
	memset(&myskt, 0, sizeof(myskt));
	myskt.sin_family = AF_INET;
	myskt.sin_port = htons(PORT_NUM);
	myskt.sin_addr.s_addr = htonl(INADDR_ANY);
	if (bind(s, (struct sockaddr *)&myskt, sizeof(myskt)) < 0) {
		perror("socket");
		exit(1);
	}
	// SIGALRM の設定
	if (sigemptyset(&alrm.sa_mask) < 0) {
		perror("sigemptyset");
		exit(1);
	}
	alrm.sa_handler = sigalrm_handler;
	if (sigaction(SIGALRM, &alrm, NULL) < 0) {
		perror("sigaction");
		exit(1);
	}
	// タイマーの設定
	timer.it_value.tv_sec = 1;
    timer.it_value.tv_usec = 0;
    timer.it_interval.tv_sec = 1;
    timer.it_interval.tv_usec = 0;
	if (setitimer(ITIMER_REAL, &timer, NULL) < 0) {
		perror("setitimer");
		exit(1);
	}
	/* プログラム自体の初期化終了 */
	while (1) {
		event = wait_event(s, &receive_dh);
		if (event == CONTINUE)
			continue;
		for (pt = ptab; pt->state; pt++) {
			if (pt->state == pc->state && pt->event == event) {
				(*pt->func)(s); 
				break;
			}
		}
		if (pt->state == 0) { 
			fprintf(stderr, "A unexpected event was happened\n");
			exit(1);
		}
	}
}