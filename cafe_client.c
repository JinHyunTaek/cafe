#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include "cafe.h"

#define MAX_MENU_MSG 30

void *handle_client(void *arg);
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
	pthread_create(&t_id, NULL, handle_client, (void *)&sock);
	pthread_join(t_id, NULL);
	close(sock);
}

void *handle_client(void *arg)
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
			puts("		=========== Menu Categories ==============");
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

/* 목록 인덱스를 넘어서는 값을 처음에 입력하면 자꾸 서버가 죽는데, 제 생각엔 이게 재귀호출을 하는 과정에서
 지역변수가 바뀌지 않는 문제가 있어 발생하는 것같아서 재귀호출 하지 않게 코드를 좀 수정했습니다. */
int print_and_return_menu_by_category(int category)
{
	int item_key = -1;

	while(1)
	{
		/* 현재 item 배열에 아이템이 커피, 티, 주스, 브런치 순으로 저장되어있으니 cafe.c에서 사용한 cnt를 사용해
		각각의 개수만큼만 출력하게 하는 로직입니다.*/
		char * menu_char_buf[CATEGORY_SIZE] = {"Coffee", "Tea", "Juice", "Brunch"};
		int cnt_per_menu[CATEGORY_SIZE] = {coffee_cnt, tea_cnt, juice_cnt, brunch_cnt};
		char menu_msg[MAX_MENU_MSG];
		snprintf(menu_msg, MAX_MENU_MSG, "		======= %s =======", menu_char_buf[category-1]);
		puts(menu_msg);
		
		int start_point = 0;
		// 예컨데 브런치라면 앞의 커피, 티, 주스 만큼의 인덱스를 더한 주소에서 시작하는겁니다
		for ( int i = 0 ; i < category-1 ; i++)
		{
			start_point += cnt_per_menu[i];
		}
		for( int i = 0 ; i < cnt_per_menu[category-1]; i++){
			printf("	menu name : %s		menu number : %d\n", items[start_point+i].name, items[start_point+i].key);
		}
		printf("Enter your choice: (1~%d)\n", cnt_per_menu[category-1]);
		scanf("%d", &item_key);
		
		// 메뉴 번호가 유효하지 않으면 반복문 처음으로 돌아가 다시 입력 받을겁니다.
		if( (item_key > cnt_per_menu[category-1]) || (item_key < 1) ){
			puts("Menu number is not valid!");
			continue;
		}
		//문제가 없다면 아이템 키를 반환합니다.
		return item_key;
	}

	// 이전 버전의 코드입니다.
	// switch (category)
	// {
	// case COFFEE:
	// 	puts("====Coffee menu====");
	// 	for (int i = 0; i < total_item_cnt; i++)
	// 	{
	// 		if (items[i].category == COFFEE)
	// 		{
	// 			printf("menu name: %s, menu number: %d\n", items[i].name, items[i].key);
	// 		}
	// 	}
	// 	printf("Enter your choice: (1~%d)\n", coffee_cnt);
	// 	scanf("%d", &item_key);
	// 	if (item_key < 1 || item_key > coffee_cnt)
	// 	{
	// 		print_and_return_menu_by_category(category);
	// 	}
	// 	return item_key;
	// case TEA:
	// 	puts("====Tea menu====");
	// 	for (int i = 0; i < total_item_cnt; i++)
	// 	{
	// 		if (items[i].category == TEA)
	// 		{
	// 			printf("menu name: %s, menu number: %d\n", items[i].name, items[i].key);
	// 		}
	// 	}
	// 	printf("Enter your choice: (1~%d)\n", tea_cnt);
	// 	scanf("%d", &item_key);
	// 	if (item_key < 1 || item_key > tea_cnt)
	// 	{
	// 		print_and_return_menu_by_category(category);
	// 	}
	// 	return item_key;
	// case JUICE:
	// 	puts("====Coffee menu====");
	// 	for (int i = 0; i < total_item_cnt; i++)
	// 	{
	// 		if (items[i].category == JUICE)
	// 		{
	// 			printf("menu name: %s, menu number: %d\n", items[i].name, items[i].key);
	// 		}
	// 	}
	// 	printf("Enter your choice: (1~%d)\n", juice_cnt);
	// 	scanf("%d", &item_key);
	// 	if (item_key < 1 || item_key > juice_cnt)
	// 	{
	// 		print_and_return_menu_by_category(category);
	// 	}
	// 	return item_key;
	// case BRUNCH:
	// 	puts("====Brunch menu====");
	// 	for (int i = 0; i < total_item_cnt; i++)
	// 	{
	// 		if (items[i].category == BRUNCH)
	// 		{
	// 			printf("menu name: %s, menu number: %d\n", items[i].name, items[i].key);
	// 		}
	// 	}
	// 	printf("Enter your choice: (1~%d)\n", brunch_cnt);
	// 	scanf("%d", &item_key);
	// 	if (item_key < 1 || item_key > brunch_cnt)
	// 	{
	// 		print_and_return_menu_by_category(category);
	// 	}
	// 	return item_key;
	// }

	// in case that user insert wrong value
	return -1;
}

void print_welcome_msg()
{
	puts("		============ Welcome to Cafe000 ===========");
	printf("1: order, 2: cancel, 3: quit:");
}
 
void error_handling(char *msg)
{
	puts(msg);
	exit(1);
}