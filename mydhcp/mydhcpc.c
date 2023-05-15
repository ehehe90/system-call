/* select */
#include <sys/select.h>
#include "header.h"


/* このファイルのみでつかいそうな define */
#define ERROR_EVENT 0
#define RECV_OFFER_OK_EVENT 1
#define RECV_TOUT_EVENT 2
#define RECV_OFFER_NG_EVENT 3
#define RECV_UNKN_EVENT 4
#define RECV_UNEX_EVENT 5
#define RECV_ACK_OK_EVENT 6
#define RECV_ACK_NG_EVENT 7
#define HALF_TTL_PASS 8
#define RECV_SIGHUP_EVENT 9
#define INIT_STATE 21
#define WAIT_OFFER_STATE 22
#define OFFER_TOUT_STATE 23
#define WAIT_ACK_STATE 24
#define ACK_TOUT_STATE 25
#define IN_USE_STATE 26
#define WAIT_ACK_EXT_STATE 27
#define ACK_EXT_TOUT_STATE 28
void f_act1(), f_act2(), f_act3(), f_act4(), f_act5(), f_act6(), f_act7(), f_act8(); 
struct proctable {
	int state;
	int event;
	void (*func)(int s, struct sockaddr_in *pskt, struct dhcph *receive_pdh);
} ptab[] = {
	{WAIT_OFFER_STATE, RECV_OFFER_OK_EVENT, f_act1}, // 2
	{WAIT_OFFER_STATE, RECV_TOUT_EVENT, f_act2}, // 6
	{WAIT_OFFER_STATE, RECV_OFFER_NG_EVENT, f_act3}, // 12-1
	{WAIT_OFFER_STATE, RECV_UNKN_EVENT, f_act3}, // 12-2
	{WAIT_OFFER_STATE, RECV_UNEX_EVENT, f_act3}, // 12-3
	{OFFER_TOUT_STATE, RECV_OFFER_OK_EVENT,f_act2}, // 7
	{OFFER_TOUT_STATE, RECV_OFFER_NG_EVENT, f_act3}, // 13-1
	{OFFER_TOUT_STATE, RECV_UNKN_EVENT, f_act3}, // 13-2
	{OFFER_TOUT_STATE, RECV_UNEX_EVENT, f_act3}, // 13-3
	{OFFER_TOUT_STATE, RECV_TOUT_EVENT, f_act3}, // 13-4
	{WAIT_ACK_STATE, RECV_ACK_OK_EVENT, f_act4}, // 3
	{WAIT_ACK_STATE, RECV_TOUT_EVENT, f_act5}, // 8
	{WAIT_ACK_STATE, RECV_ACK_NG_EVENT, f_act3}, // 14-1
	{WAIT_ACK_STATE, RECV_UNKN_EVENT, f_act3}, // 14-2
	{WAIT_ACK_STATE, RECV_UNEX_EVENT, f_act3}, // 14-3
	{ACK_TOUT_STATE, RECV_ACK_OK_EVENT, f_act4}, //9
	{ACK_TOUT_STATE, RECV_ACK_NG_EVENT, f_act3}, // 15-1
	{ACK_TOUT_STATE, RECV_UNKN_EVENT, f_act3}, // 15-2
	{ACK_TOUT_STATE, RECV_UNEX_EVENT, f_act3}, // 15-3
	{ACK_TOUT_STATE, RECV_TOUT_EVENT, f_act3}, // 15-4
	{IN_USE_STATE, HALF_TTL_PASS, f_act6}, // 4
	{IN_USE_STATE, RECV_SIGHUP_EVENT, f_act8}, // 18
	{WAIT_ACK_EXT_STATE, RECV_ACK_OK_EVENT, f_act4}, // 5
	{WAIT_ACK_EXT_STATE, RECV_TOUT_EVENT, f_act7}, // 11
	{WAIT_ACK_EXT_STATE, RECV_ACK_NG_EVENT, f_act3}, // 17-1
	{WAIT_ACK_EXT_STATE, RECV_UNKN_EVENT, f_act3}, // 17-2
	{WAIT_ACK_EXT_STATE, RECV_UNEX_EVENT, f_act3}, // 17-3
	{ACK_EXT_TOUT_STATE, RECV_ACK_OK_EVENT, f_act4}, // 10
	{ACK_EXT_TOUT_STATE, RECV_ACK_NG_EVENT, f_act3}, // 16-1
	{ACK_EXT_TOUT_STATE, RECV_UNKN_EVENT, f_act3}, // 16-2
	{ACK_EXT_TOUT_STATE, RECV_UNEX_EVENT, f_act3}, // 16-3
	{ACK_EXT_TOUT_STATE, RECV_TOUT_EVENT, f_act3}, // 16-4
	{0, 0, NULL}
};
struct statetable {
	int state;
	char *p;
} stab[] = {
	{INIT_STATE, "INIT_STATE"},
	{WAIT_OFFER_STATE, "WAIT_OFFER_STATE"},
	{OFFER_TOUT_STATE, "OFFER_TOUT_STATE"},
	{WAIT_ACK_STATE, "WAIT_ACK_STATE"},
	{ACK_TOUT_STATE, "ACK_TOUT_STATE"},
	{IN_USE_STATE, "IN_USE_STATE"},
	{WAIT_ACK_EXT_STATE, "WAIT_ACK_EXT_STATE"},
	{ACK_EXT_TOUT_STATE, "ACK_EXT_TOUT_STATE"},
	{0, NULL}
};
int state, sigalrm_flag, sighup_flag;
void sigalrm_handler(int sig_num)
{
	if (sig_num == SIGALRM)
		sigalrm_flag++;
}
void sighup_handler(int sig_num)
{
	if (sig_num == SIGHUP)
		sighup_flag++;
}
int identify_signal()
{
	if (sighup_flag > 0) {
		sighup_flag = 0;
		printf("----------------------------------------\n");
		printf("Received SIGHUP\n");
		return RECV_SIGHUP_EVENT;
	}
	if (sigalrm_flag > 0) {
		sigalrm_flag = 0;
		printf("----------------------------------------\n");
		printf("Half of ttl has just passed\n");
		return HALF_TTL_PASS;
	}
	return ERROR_EVENT;
}
void set_timer(int value, int interval)
{
	struct itimerval timer;
	timer.it_value.tv_sec = value;
	timer.it_value.tv_usec = 0;
	timer.it_interval.tv_sec = interval;
	timer.it_interval.tv_usec = 0;
	if (setitimer(ITIMER_REAL, &timer, NULL) < 0) {
		perror("setitimer");
		exit(1);
	}
}
void send_message(int s, struct dhcph *pdh, struct sockaddr_in *psend_skt)
{
	if (sendto(s, pdh, sizeof(struct dhcph), 0, (struct sockaddr *)psend_skt, sizeof(struct sockaddr_in)) < 0) {
		perror("sendto");
		exit(1);
	}
	printf("----------------------------------------\n");
	printf("Sending message to the server (IP address : %s, port : %d)\n", inet_ntoa(psend_skt->sin_addr), ntohs(psend_skt->sin_port));
	print_message(pdh);
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
	printf("Changing state from %s ", st->p);
	for (st = stab; st->state; st++) {
		if (st->state == after_state) {
			break;
		}
	}
	printf("to %s\n", st->p);
}
void f_act1(int s, struct sockaddr_in *psend_skt, struct dhcph *receive_pdh)
{
	/*
	2, 7
	WAIT_OFFER_STATE, OFFER_TOUT_STATE で offer (ok) を受信した場合．
	request (alloc) を送って，WAIT_ACK_STATE に遷移．setitimer を 10 秒に設定.
	*/
	struct dhcph send_dh;
	send_dh.type = 3;
	send_dh.code = 2;
	send_dh.ttl = receive_pdh->ttl;
	send_dh.address = receive_pdh->address;
	send_dh.netmask = receive_pdh->netmask;
	send_message(s, &send_dh, psend_skt);
	//set_timer(10, 0);
	print_state_change(state, WAIT_ACK_STATE);
	state = WAIT_ACK_STATE;
}
void f_act2(int s, struct sockaddr_in *psend_skt, struct dhcph *receive_pdh)
{
	/*
	6
	WAIT_OFFER_STATE で受信タイムアウトした場合。
	offer を再送して、OFFER_TOUT_STATE に遷移。 setitimer を 10 秒に設定。
	*/
	struct dhcph send_dh;
	send_dh.type = 1;
	send_dh.code = 0;
	send_dh.ttl = htons(0);
	send_dh.address = htonl(0);
	send_dh.netmask = htonl(0);
	send_message(s, &send_dh, psend_skt);
	//set_timer(10, 0);
	print_state_change(state, OFFER_TOUT_STATE);
	state = OFFER_TOUT_STATE;
}
void f_act3(int s, struct sockaddr_in *psend_skt, struct dhcph *receive_pdh)
{
	/*
	12 ~ 17
	様々な場合。
	プログラムを終了させる。
	*/
	printf("----------------------------------------\n");
	printf("Terminating this process\n");
	exit(0);
}
void f_act4(int s, struct sockaddr_in *psend_skt, struct dhcph *receive_pdh)
{
	/*
	3, 5, 9, 10
	様々な状態で ack (ok) を受信した場合。
	IN_USE_STATE に遷移。setitimer を ack 中の ttl の半分に設定。
	*/
	int ttl;
	struct in_addr addr, netmask;
	if (state == WAIT_ACK_STATE) {
		addr.s_addr = receive_pdh->address;
		netmask.s_addr = receive_pdh->netmask;
		printf("----------------------------------------\n");
		printf("Allocated IP address : %s, ", inet_ntoa(addr));
		printf("Netmask : %s, ttl : %d\n", inet_ntoa(netmask), ntohs(receive_pdh->ttl));
	}
	ttl = ntohs(receive_pdh->ttl);
	set_timer(ttl / 2, 0);
	print_state_change(state, IN_USE_STATE);
	state = IN_USE_STATE;
}
void f_act5(int s, struct sockaddr_in *psend_skt, struct dhcph *receive_pdh)
{
	/*
	8
	WAIT_ACK_STATE で受信タイムアウトした場合。
	request (alloc) を再送して、ACK_TOUT_STATE に遷移。setitimer を 10 秒に設定。
	*/
	struct dhcph send_dh;
	send_dh.type = 3;
	send_dh.code = 2;
	send_dh.ttl = receive_pdh->ttl;
	send_dh.address = receive_pdh->address;
	send_dh.netmask = receive_pdh->netmask;
	send_message(s, &send_dh, psend_skt);
	//set_timer(10, 0);
	print_state_change(state, ACK_TOUT_STATE);
	state = ACK_TOUT_STATE;
}
void f_act6(int s, struct sockaddr_in *psend_skt, struct dhcph *receive_pdh)
{
	/*
	4
	IN_USE_STATE で ttl の半分が過ぎた場合。
	request (ext) を送信して、WAIT_ACK_EXT_STATE に遷移。setitimer を 10 秒に設定。
	*/
	struct dhcph send_dh;\
	send_dh.type = 3;
	send_dh.code = 3;
	send_dh.ttl = receive_pdh->ttl;
	send_dh.address = receive_pdh->address;
	send_dh.netmask = receive_pdh->netmask;
	send_message(s, &send_dh, psend_skt);
	//set_timer(10, 0);
	print_state_change(state, WAIT_ACK_EXT_STATE);
	state = WAIT_ACK_EXT_STATE;
}
void f_act7(int s, struct sockaddr_in *psend_skt, struct dhcph *receive_pdh)
{
	/*
	11
	WAIT_ACK_EXT_STATE で受信タイムアウト場合。
	request (ext) を再送して、ACK_EXT_TOUT_STATE に遷移。setitimer を 10 秒に設定。
	*/
	struct dhcph send_dh;\
	send_dh.type = 3;
	send_dh.code = 3;
	send_dh.ttl = receive_pdh->ttl;
	send_dh.address = receive_pdh->address;
	send_dh.netmask = receive_pdh->netmask;
	send_message(s, &send_dh, psend_skt);
	//set_timer(10, 0);
	print_state_change(state, ACK_EXT_TOUT_STATE);
	state = ACK_EXT_TOUT_STATE;
}
void f_act8(int s, struct sockaddr_in *psend_skt, struct dhcph *receive_pdh)
{
	/*
	18
	IN_USE_STATE で SIGHUP を受信した場合。
	release を送信して、終了。
	*/
	struct dhcph send_dh;
	send_dh.type = 5;
	send_dh.code = 0;
	send_dh.ttl = ntohs(0);
	send_dh.address = receive_pdh->address;
	send_dh.netmask = htonl(0);
	send_message(s, &send_dh, psend_skt);
	printf("----------------------------------------\n");
	printf("Terminating this process\n");
	exit(0);
}
int wait_event(int s, struct dhcph *receive_pdh) 
{
	fd_set rdfds;
	struct timeval timer;
	int select_return, data_size, type, code;
	//struct dhcph receive_dh;
	struct sockaddr_in skt;
	socklen_t sktlen = sizeof(skt);
	if (state == IN_USE_STATE) {
		pause();
		return identify_signal();
	} else {
		timer.tv_sec = 10;
		timer.tv_usec = 0;
		FD_ZERO(&rdfds);
		//FD_SET(0, &rdfds);
		FD_SET(s, &rdfds);
		if ((select_return = select(s + 1, &rdfds, NULL, NULL, &timer)) < 0) { // select がエラーの場合
			if (errno == EINTR) { // エラーの原因がシグナル受信である場合
				return identify_signal();
			} else { // 他のエラー原因の場合
				perror("select");
				exit(1);
			}
		} else if (select_return == 0) { // タイムアウトの場合
			printf("Receving timeout\n");
			return RECV_TOUT_EVENT;
		} else { // s が読み込み可能になった場合
			if (data_size = recvfrom(s, receive_pdh, sizeof(struct dhcph), 0, (struct sockaddr *)&skt, &sktlen) < 0) {
				perror("recvfrom");
				exit(1);
			} else {
				type = (int)(receive_pdh->type);
				code = (int)(receive_pdh->code);
				printf("----------------------------------------\n");
				printf("Recieved message from the server (IP address : %s, port : %d)\n", inet_ntoa(skt.sin_addr), ntohs(skt.sin_port)); //ここまで
				print_message(receive_pdh);
				switch (type) {
					case 1:
						printf("Values in the message is unexpected\n");
						return RECV_UNEX_EVENT;
						break;
					case 2:
						if (state == WAIT_OFFER_STATE || state == OFFER_TOUT_STATE) {
							if (code == 0) {
								return RECV_OFFER_OK_EVENT; // 2, 7
							} else if (code == 1) {
								return RECV_OFFER_NG_EVENT; // 12-1, 13-1
							} else {
								printf("Don't match to DHCP message format\n");
								return RECV_UNKN_EVENT; // 12-2, 13-2
							}
						} else {
							printf("Values in the message is unexpected\n");
							return RECV_UNEX_EVENT; // 14-2, 15-2, 17-2, 16-2
						}
						break;
					case 3:
						printf("Values in the message is unexpected\n");
						return RECV_UNEX_EVENT;
						break;
					case 4:
						if (state == WAIT_ACK_STATE || state == ACK_TOUT_STATE || state == WAIT_ACK_EXT_STATE || state == ACK_EXT_TOUT_STATE) {
							if (code == 0) {
								return RECV_ACK_OK_EVENT; // 3, 9, 5, 10
							} else if (code == 4) {
								return RECV_ACK_NG_EVENT; // 14-1, 15-1, 17-1, 16-1
							} else {
								printf("Don't match to DHCP message format\n");
								return RECV_UNKN_EVENT; // 14-2, 15-2, 17-2, 16-2
							}
						} else {
							printf("Values in the message is unexpected\n");
							return RECV_UNEX_EVENT; // 12-3, 13-3
						}
						break;
					case 5:
						printf("Values in the message is unexpected\n");
						return RECV_UNEX_EVENT;
						break;
					default:
						printf("Don't match to DHCP message format\n");
						return RECV_UNKN_EVENT;
						break;
				}
			}
		}
	}
}

int main(int argc, char *argv[])
{
	struct dhcph receive_dh, send_dh;
	int event, s;
	struct sockaddr_in send_skt;
	struct sigaction hup, alrm;
	struct proctable *pt;
	/* プログラムの初期化 */
	// 引数の確認
	if (argc != 2) {
		fprintf(stderr, "Usage: ./mydhcpc <server-IP-address>\n");
		exit(1);
	}
	// ソケット関係の設定
	if ((s = socket(PF_INET, SOCK_DGRAM, 0)) < 0) {
		perror("socket");
		exit(1);
	}
	memset(&send_skt, 0, sizeof(send_skt));
	send_skt.sin_family = AF_INET;
	send_skt.sin_port = htons(PORT_NUM);
	if (inet_aton(argv[1], &(send_skt.sin_addr)) == 0) {
		fprintf(stderr, "%s is invalid\n", argv[1]);
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
	// SIGHUP の設定
	if (sigemptyset(&hup.sa_mask) < 0) {
		perror("sigemptyset");
		exit(1);
	}
	hup.sa_handler = sighup_handler;
	if (sigaction(SIGHUP, &hup, NULL) < 0) {
		perror("sigaction");
		exit(1);
	}
	// 初期状態の設定
	state = INIT_STATE;
	/* プログラムの初期化終わり */
	while (1) {
		if (state == INIT_STATE) {
			// discover 送信して、WAIT_OFFER_STATE に遷移
			send_dh.type = 1;
			send_dh.code = 0;
			send_dh.ttl = htons(0);
			send_dh.address = htonl(0);
			send_dh.netmask = htonl(0);
			send_message(s, &send_dh, &send_skt);
			state = WAIT_OFFER_STATE;
		}
		event = wait_event(s, &receive_dh);
		for (pt = ptab; pt->state; pt++) {
			if ((pt->state == state) && (pt->event == event)) {
				(*pt->func)(s, &send_skt, &receive_dh);
				break;
			}
		}
	}
}

