#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <signal.h>
#include "cafe.h"

void order_service(int sock);
void print_welcome_msg();
int print_and_return_menu_by_category(int category);

int main(int argc, char *argv[])
{
	int sock;
	struct sockaddr_in serv_addr;
	if (argc != 3)
	{
		printf("Usage : %s <IP> <port>\n", argv[0]);
		exit(1);
	}
	signal(SIGINT, backup_warning); // signal handler 등록
	sock = socket(PF_INET, SOCK_STREAM, 0);

	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = inet_addr(argv[1]);
	serv_addr.sin_port = htons(atoi(argv[2]));

	// connect request to server (blocked until accept() is called)
	if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == -1)
		error_handling("connect() error");
	int me = CLIENT;
	write(sock, &me, sizeof(me));

	order_service(sock);
	close(sock);
}

void order_service(int sock)
{
	char dummy = 0; // just exists write needs some data
	int pay=0, is_continue;
	RECENT_MENU recent_menu;
	REQ_PACKET req_packet;
	RES_PACKET res_packet;
	while (1)
	{
		write(sock, &dummy, sizeof(dummy));
		if (read(sock, &recent_menu, sizeof(RECENT_MENU)) < sizeof(RECENT_MENU))
			error_handling("read()");
		do
		{
			clear_terminal();
			is_continue = 0;
			memset(&req_packet, 0, sizeof(REQ_PACKET));
			memset(&res_packet, 0, sizeof(RES_PACKET));
			print_welcome_msg();
			scanf("%d", &req_packet.cmd);
			if (req_packet.cmd < ORDER || req_packet.cmd > QUIT){
				is_continue = 1;
				continue;
			}
			switch (req_packet.cmd)
			{
			case ORDER:
				puts("===========Menu Categories==============");
				printf("1 coffee, 2. tea, 3. juice, 4. brunch, else. exit: ");
				scanf("%d", &req_packet.item_category);
				if (req_packet.item_category < 1 || req_packet.item_category > CATEGORY_SIZE){
					is_continue = 1;
					continue;
				}
				initialize_item_info(recent_menu);
				while ((req_packet.item_key = print_and_return_menu_by_category(req_packet.item_category)) == -1)
					;
				int item_idx = find_item_idx_by_category_and_key(req_packet.item_category, req_packet.item_key);
				// 현재 남아 있는 잔액이 선택한 상품의 가격보다 낮은 경우 잔액 충전
				while (1)
				{
					printf("Pay same amout of money with item (item price : %d): ", items[item_idx].price);
					scanf("%d", &pay);
					if(pay != items[item_idx].price){
						printf("Pay again please. (Item price : %d, Your input : %d)\n",items[item_idx].price,pay);
					}
					else{
						break;
					}
				}
				write(sock, &req_packet, sizeof(REQ_PACKET));
				read(sock, &res_packet, sizeof(RES_PACKET));
				puts(res_packet.res_msg);
				return_main();
				break;
			case QUIT:
				write(sock, &req_packet, sizeof(REQ_PACKET));
				return;
			}
		} while (is_continue);
	}
}

int print_and_return_menu_by_category(int category)
{
	int item_key;
	switch (category)
	{
	case COFFEE:
		puts("===========Coffee menu===========");
		for (int i = 0; i < total_item_cnt; i++)
			if (items[i].category == COFFEE)
				printf("menu name: %s, choice : %d, price: %d, \n", items[i].name, items[i].price, items[i].price);
		printf("Enter your choice: (1~%d): ",coffee_cnt);
		scanf("%d", &item_key);
		if (item_key < 1 || item_key > coffee_cnt)
			return -1;
		return item_key;
	case TEA:
		puts("===========Tea menu===========");
		for (int i = 0; i < total_item_cnt; i++)
			if (items[i].category == TEA)
				printf("menu name: %s, choice : %d, price: %d, \n", items[i].name, items[i].price, items[i].price);
		printf("Enter your choice: (1~%d): ",tea_cnt);

		scanf("%d", &item_key);
		if (item_key < 1 || item_key > tea_cnt)
			return -1;
		return item_key;
	case JUICE:
		puts("===========Juice menu===========");
		for (int i = 0; i < total_item_cnt; i++)
			if (items[i].category == JUICE)
				printf("menu name: %s, choice : %d, price: %d, \n", items[i].name, items[i].price, items[i].price);

		printf("Enter your choice: (1~%d): ", juice_cnt);

		scanf("%d", &item_key);
		if (item_key < 1 || item_key > juice_cnt)
			return -1;
		return item_key;
	case BRUNCH:
		puts("===========Brunch menu===========");
		for (int i = 0; i < total_item_cnt; i++)
			if (items[i].category == BRUNCH)
				printf("menu name: %s, choice : %d, price: %d, \n", items[i].name, items[i].price, items[i].price);
		printf("Enter your choice: (1~%d): ", brunch_cnt);

		scanf("%d", &item_key);
		if (item_key < 1 || item_key > brunch_cnt)
			return -1;
		return item_key;
	}
}

void print_welcome_msg()
{
	puts("============Welcome to Cafe000===========");
	printf("1. order, 2. quit: ");
}