#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include "cafe.h"

void handle_admin(int sock);
void print_welcome_msg();
void error_handling(char *msg);

void display_single_item(ITEM item);

void add_item(ADMIN_REQ_PACKET *);


int main(int argc, char*argv[]){
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

	//connect request to server (blocked until accept() is called) 
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == -1)
        error_handling("connect() error");

	// 내가 어드민이라는 신호를 소켓에 적용
	int me = ADMIN;
	write(sock,&me,sizeof(me));
	restore_menu();
	handle_admin(sock);
	close(sock);
}

void handle_admin(int sock){
	int cmd;
	ADMIN_REQ_PACKET req_packet;
	ADMIN_RES_PACKET res_packet;
	memset(&req_packet,0,sizeof(ADMIN_REQ_PACKET));
	memset(&res_packet,0,sizeof(ADMIN_RES_PACKET));
	while(1){
		print_welcome_msg();
		scanf("%d",&req_packet.cmd);
		switch(req_packet.cmd){
			
			case ADD_ITEM:
				add_item(&req_packet);
				break;



			case ADMIN_QUIT:
				write(sock,&req_packet, sizeof(ADMIN_REQ_PACKET));
				return;
		}


		//요청에 따라 만들어진 패킷을 전송
		write(sock,&req_packet, sizeof(ADMIN_REQ_PACKET));
		//서버 응답까지 대기
		read(sock,&res_packet, sizeof(ADMIN_RES_PACKET));

		if( (res_packet.cmd == ADD_ITEM) && (res_packet.result == 1) ){
			// 서버에 메뉴가 추가됐으니 동기화해야겠죠. 추가한 김에 카테고리 메뉴들도 좀 보여줍시다.
			restore_menu();

			printf("	========== Here's Changed Menu ==========\n");
			printf("category	key	name		stock	price\n");
			for(int i = 0 ; i < total_item_cnt ; i++){
				// 우리가 요청한 패킷의 카테고리와 같은 메뉴만 보여주면 되니까요.
				if(items[i].category == req_packet.item.category){
				display_single_item(items[i]);
				}
			}
			printf("\nPRESS ANY BUTTEN");
			getchar();getchar();	// 앞에서 \n 받고 아무거나 누르면 넘어가게 설정
		}
	}
}

void print_welcome_msg(){
	puts("		============ MAIN ===========");
	
	printf("	1: add item\n	2: show items\n	 3: update item,\n	4: delete item\n	5: get all customers\n	6: quit\n\ninput :");
}

// 한 ITEM 구조체를 [	카테고리	키	이름	수량	가격	] 형식으로 출력합니다. 각 속성 사이 공백은 탭입니다.
void display_single_item(ITEM item){
	printf("	%d	%d	%-8s	%d	%d\n", item.category,  item.key, item.name, item.stock, item.price);
}


void add_item(ADMIN_REQ_PACKET* req_packet){
	

	//ITEM new_menu 에 아이템 속성을 입력받아 추가함.

	puts("Which category do you want to add?");
	puts("1. Coffee	2. Tea");
	puts("3. juice	4. BRUNCH");
	while(1){
		scanf("%d",&req_packet->item.category);
		if( (req_packet->item.category < 1) || (req_packet->item.category > CATEGORY_SIZE) ){
			printf("	! Please Enter valid value ( 1 ~ %d ) !\n", CATEGORY_SIZE);
			continue;
		}
		break;
	}
	printf("Please insert the Name, stock, price of the menu\n [Usage : Name stock price] : ");
	while(1){
		scanf("%s %d %d", req_packet->item.name, &req_packet->item.stock, &req_packet->item.price);
		//error handling?
		break;
	}

	
	
	
}