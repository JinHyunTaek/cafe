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

void print_welcome_msg();
void print_category();
void print_menu(int);

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
	int pay = 0, is_continue;
	int order_proceed;
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
			// 언제든 -1 입력 시 메뉴로 돌아감을 표기
			print_centered("\033[1;37;44mInsert -1 anytime to go Menu\033[0m\n\n");
			print_welcome_msg();
			scanf("%d", &req_packet.cmd);
			if (req_packet.cmd < ORDER || req_packet.cmd > QUIT)
			{
				is_continue = 1;
				continue;
			}
			switch (req_packet.cmd)
			{
			case ORDER:
				print_category();
				printf("\nSelect Category: ");
				scanf("%d", &req_packet.item_category);
				if (req_packet.item_category < 1 || req_packet.item_category > CATEGORY_SIZE)
				{
					is_continue = 1;
					continue;
				}
				initialize_item_info(recent_menu);
				while ((req_packet.item_key = print_and_return_menu_by_category(req_packet.item_category)) == -1)
				{
					// 입력값이 비정상적이면 경고문 출력
					if (req_packet.item_key == -1)
						puts("Wrong intput!");
					if (req_packet.item_key == 0) // go back to the menu
						break;
				}
				if (req_packet.item_key == 0) // go back to the menu
				{
					is_continue = 1;
					continue;
				}

				int item_idx = find_item_idx_by_category_and_key(req_packet.item_category, req_packet.item_key);
				// 현재 남아 있는 잔액이 선택한 상품의 가격보다 낮은 경우 잔액 충전
				while (1)
				{
					while (1)
					{
						printf("\nQuantity : ");
						scanf("%d", &req_packet.quantity);
						if (req_packet.quantity == -1) // go back to the menu
							break;
						if (req_packet.quantity < 1)
						{
							puts("\nYou should input at least 1 quantity.");
						}
						else
						{
							break;
						}
					}
					if (req_packet.quantity == -1) // go back to the menu
						break;
					printf("\n[Pay] ₩%d: ", items[item_idx].price * req_packet.quantity);
					scanf("%d", &pay);
					if (pay == -1) // go back to the menu
							break;
					if (pay != items[item_idx].price * req_packet.quantity)
					{
						printf("[Pay] again please ₩%d, Your input : %d\n", items[item_idx].price * req_packet.quantity, pay);
					}
					else
					{
						break;
					}
				}

				if (req_packet.quantity == -1 || pay == -1) // go back to the menu
				{
					is_continue = 1;
					continue;
				}

				// 주문 확인
				printf("\nHere is your order details.\n");
				printf("-----------------------------\n");
				printf(" Menu Name: %s\n", items[item_idx].name);
				printf(" Quantity: %d\n", req_packet.quantity);
				printf(" Total Price: %d\n", pay);
				printf("-----------------------------\n");
				while (1) {
					printf("Shall we proceed with the order? (1:yes/0:no): ");
					scanf("%d", &order_proceed);
					if (order_proceed == 1 || order_proceed == 0 || order_proceed == -1)
						break;
				}
				if (order_proceed == 0 || order_proceed == -1) // go back to the menu
				{
					printf("Order has been canceled\nPay has been returned.\n");
					return_main();
					is_continue = 1;
					continue;
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
		print_centered("============ Menu ============\n");
		for (int i = 0; i < total_item_cnt; i++)
			if (items[i].category == COFFEE)
				print_menu(i);
		printf("\nSelect Menu (1~%d): ", coffee_cnt);

		scanf("%d", &item_key);
		if (item_key == -1) // go back to the menu
			return 0;
		if (item_key < 1 || item_key > coffee_cnt)
			return -1;
		return item_key;

	case TEA:
		print_centered("============ Menu ============\n");
		for (int i = 0; i < total_item_cnt; i++)
			if (items[i].category == TEA)
				print_menu(i);
		printf("\nSelect Menu (1~%d): ", tea_cnt);

		scanf("%d", &item_key);
		if (item_key == -1) // go back to the menu
			return 0;
		if (item_key < 1 || item_key > tea_cnt)
			return -1;
		return item_key;

	case JUICE:
		print_centered("============ Menu ============\n");
		for (int i = 0; i < total_item_cnt; i++)
			if (items[i].category == JUICE)
				print_menu(i);

		printf("\nSelect Menu (1~%d): ", juice_cnt);

		scanf("%d", &item_key);
		if (item_key == -1) // go back to the menu
			return 0;
		if (item_key < 1 || item_key > juice_cnt)
			return -1;
		return item_key;

	case BRUNCH:
		print_centered("============ Menu ============\n");
		for (int i = 0; i < total_item_cnt; i++)
			if (items[i].category == BRUNCH)
				print_menu(i);
		printf("\nSelect Menu (1~%d): ", brunch_cnt);

		scanf("%d", &item_key);
		if (item_key == -1) // go back to the menu
			return 0;
		if (item_key < 1 || item_key > brunch_cnt)
			return -1;
		return item_key;
	}
}

void print_welcome_msg()
{
	print_centered("========== Welcome! ==========\n");
	print_centered("1. Order\n");
	print_centered("2. Quit\n");
	print_centered("\nSelect Option: ");
}

void print_category()
{
	print_centered("========== Category ==========\n");
	print_centered("1. Coffee\n");
	print_centered("2. Tea\n");
	print_centered("3. Juice\n");
	print_centered("4. Brunch\n");
}

void print_menu(int x)
{
	char buffer[256];
	snprintf(buffer, sizeof(buffer), "%-3d%-16s %7d ₩\n", items[x].key, items[x].name, items[x].price);

	print_centered(buffer);
}