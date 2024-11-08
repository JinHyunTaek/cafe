#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include "cafe.h"

void *handle_customer(void *arg);
void print_welcome_msg();
int print_and_return_menu_by_category(int category);
void error_handling(char *msg);

int main(int argc, char *argv[])
{
	int sock;
	struct sockaddr_in serv_addr;
	pthread_t t_id;
	if (argc != 3)
	{
		printf("Usage : %s <IP> <port>\n", argv[0]);
		exit(1);
	}
	sock = socket(PF_INET, SOCK_STREAM, 0);

	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = inet_addr(argv[1]);
	serv_addr.sin_port = htons(atoi(argv[2]));

	// connect request to server (blocked until accept() is called)
	if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == -1)
		error_handling("connect() error");
	restore_menu();
	pthread_create(&t_id, NULL, handle_customer, (void *)&sock);
	pthread_join(t_id, NULL);
	close(sock);
}

void *handle_customer(void *arg)
{
	int sock = *(int *)arg;
	REQ_PACKET req_packet;
	RES_PACKET res_packet;
	while (1)
	{
		memset(&req_packet, 0, sizeof(REQ_PACKET));
		memset(&res_packet, 0, sizeof(RES_PACKET));
		print_welcome_msg();
		scanf("%d", &req_packet.cmd);
		if (req_packet.cmd < 1 || req_packet.cmd > 3)
			continue;
		switch (req_packet.cmd)
		{
		case ORDER:
			puts("===========Menu Categories==============");
			printf("1. coffee, 2. tea, 3. juice, 4. brunch, 5. exit: ");
			scanf("%d", &req_packet.item_category);
			if (req_packet.item_category < 1 || req_packet.item_category > CATEGORY_SIZE)
				continue;
			req_packet.item_key = print_and_return_menu_by_category(req_packet.item_category);
			write(sock, &req_packet, sizeof(REQ_PACKET));
			read(sock,&res_packet,sizeof(RES_PACKET));
			puts(res_packet.res_msg);
			puts("");
			break;
		case QUIT:
			write(sock, &req_packet, sizeof(REQ_PACKET));
			return NULL;
		}
	}
}

int print_and_return_menu_by_category(int category)
{
	int item_key;
	switch (category)
	{
	case COFFEE:
		puts("====Coffee menu====");
		for (int i = 0; i < total_item_cnt; i++)
		{
			if (items[i].category == COFFEE)
			{
				printf("menu name: %s, menu number: %d\n", items[i].name, items[i].key);
			}
		}
		printf("Enter your choice: (1~%d)\n", coffee_cnt);
		scanf("%d", &item_key);
		if (item_key < 1 || item_key > coffee_cnt)
		{
			print_and_return_menu_by_category(category);
		}
		return item_key;
	case TEA:
		puts("====Tea menu====");
		for (int i = 0; i < total_item_cnt; i++)
		{
			if (items[i].category == TEA)
			{
				printf("menu name: %s, menu number: %d\n", items[i].name, items[i].key);
			}
		}
		printf("Enter your choice: (1~%d)\n", tea_cnt);
		scanf("%d", &item_key);
		if (item_key < 1 || item_key > tea_cnt)
		{
			print_and_return_menu_by_category(category);
		}
		return item_key;
	case JUICE:
		puts("====Coffee menu====");
		for (int i = 0; i < total_item_cnt; i++)
		{
			if (items[i].category == JUICE)
			{
				printf("menu name: %s, menu number: %d\n", items[i].name, items[i].key);
			}
		}
		printf("Enter your choice: (1~%d)\n", juice_cnt);
		scanf("%d", &item_key);
		if (item_key < 1 || item_key > juice_cnt)
		{
			print_and_return_menu_by_category(category);
		}
		return item_key;
	case BRUNCH:
		puts("====Brunch menu====");
		for (int i = 0; i < total_item_cnt; i++)
		{
			if (items[i].category == BRUNCH)
			{
				printf("menu name: %s, menu number: %d\n", items[i].name, items[i].key);
			}
		}
		printf("Enter your choice: (1~%d)\n", brunch_cnt);
		scanf("%d", &item_key);
		if (item_key < 1 || item_key > brunch_cnt)
		{
			print_and_return_menu_by_category(category);
		}
		return item_key;
	}
}

void print_welcome_msg()
{
	puts("============Welcome to Cafe000===========");
	printf("1: order, 2: cancel, 3: quit:");
}
 
void error_handling(char *msg)
{
	puts(msg);
	exit(1);
}