#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include "cafe.h"
#define MAX_USER 10000
#define ADMIN_PWD 1234

int clnt_socks[MAX_USER];
int clnt_cnt; // (connected)
int waiting_clnt;

pthread_mutex_t mutex; // note that all (multi) threads are sharing this mutex variable

void make_menu(int item_category, int item_key, char *res_msg);
void backup();
void error_handling(char *msg);
void *handle_clnt(void *arg);
void remove_clnt(int clnt_sock);

int main(int argc, char *argv[])
{
	int serv_sock, clnt_sock;
	struct sockaddr_in serv_adr, clnt_adr;
	int clnt_adr_sz;
	pthread_t t_id;
	if (argc != 2)
	{
		printf("Usage : %s <port>\n", argv[0]);
		exit(1);
	}

	serv_sock = socket(PF_INET, SOCK_STREAM, 0);

	memset(&serv_adr, 0, sizeof(serv_adr));
	serv_adr.sin_family = AF_INET;
	serv_adr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_adr.sin_port = htons(atoi(argv[1]));

	if (bind(serv_sock, (struct sockaddr *)&serv_adr, sizeof(serv_adr)) == -1) // binding server address (ip, port)
		error_handling("bind() error");
	if (listen(serv_sock, 5) == -1) // after this function call, client could request to server (= server is ready to listen)
		error_handling("listen() error");
	puts("------------------------\nCafe Server Start\n------------------------");
	restore_menu();
	while (1)
	{
		clnt_adr_sz = sizeof(clnt_adr);
		clnt_sock = accept(serv_sock, (struct sockaddr *)&clnt_adr, &clnt_adr_sz);
		printf("Connected client IP: %s, clnt_sock:%d \n", inet_ntoa(clnt_adr.sin_addr), clnt_sock);
		clnt_socks[clnt_cnt++] = clnt_sock;

		pthread_create(&t_id, NULL, handle_clnt, (void *)&clnt_sock); // create thread per client, handle handle_clnt func
		pthread_detach(t_id);
	}
	pthread_mutex_destroy(&mutex);
	close(serv_sock);
}

void *handle_clnt(void *arg)
{
	int clnt_sock = *(int *)arg;
	REQ_PACKET req_packet;
	RES_PACKET res_packet;
	while (1)
	{
		memset(&req_packet, 0, sizeof(REQ_PACKET));
		memset(&res_packet, 0, sizeof(RES_PACKET));
		read(clnt_sock, &req_packet, sizeof(REQ_PACKET)); // blocked until write request is arrived by client
		res_packet.cmd = req_packet.cmd;
		switch (req_packet.cmd)
		{
		case QUIT:
			remove_clnt(clnt_sock);
			return NULL;
		case ORDER:
			// pthread_mutex_lock(&mutex);
			// waiting_clnt += 1;
			// pthread_mutex_unlock(&mutex);
			pthread_mutex_lock(&mutex); // needs to synchronize
			make_menu(req_packet.item_category, req_packet.item_key, res_packet.res_msg);
			write(clnt_sock, &res_packet, sizeof(RES_PACKET));
			// waiting_clnt -= 1;
			pthread_mutex_unlock(&mutex);
		}
	}
}

void make_menu(int item_category, int item_key, char *res_msg)
{
	char temp_msg[BUF_SIZE];
	switch (item_category)
	{
	case COFFEE:
		sleep(W_COFFEE);
		break;
	case TEA:
		sleep(W_TEA);
		break;
	case JUICE:
		sleep(W_JUICE);
		break;
	case BRUNCH:
		sleep(W_BRUNCH);
		break;
	}
	for (int i = 0; i < total_item_cnt; i++)
	{
		if (items[i].category == item_category && items[i].key == item_key)
		{
			sprintf(temp_msg, "Thank you for waiting! Your %s is now ready.", items[i].name);
			strcpy(res_msg, temp_msg);
			return;
		}
	}
	error_handling("could not find menu");
}

void remove_clnt(int clnt_sock)
{
	for (int i = 0; i < clnt_cnt; i++)
	{
		if (clnt_socks[i] == clnt_sock)
		{
			clnt_cnt--;
			printf("client %d removed\n", clnt_sock);
			for (int j = i; j < clnt_cnt; j++)
			{
				clnt_socks[j] = clnt_socks[j + 1];
			}
			break;
		}
	}
}

void error_handling(char *msg)
{
	puts(msg);
	exit(1);
}