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

void synchronize_server(ADMIN_RES_PACKET );

void add_item(ADMIN_REQ_PACKET *);
void show_item(ADMIN_REQ_PACKET *);
void update_item(ADMIN_REQ_PACKET *);
void delete_item(ADMIN_REQ_PACKET *);

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
			case SHOW_ITEM:
				show_item(&req_packet);
				break;
			case UPDATE_ITEM:
				update_item(&req_packet);
				break;
			case DELETE_ITEM:
				delete_item(&req_packet);
				break;
			case SHOW_CUSTOMER:
				// 구현해주세요!
				break;

			case ADMIN_QUIT:
				write(sock,&req_packet, sizeof(ADMIN_REQ_PACKET));
				return;
			default:
				puts("Something didnt go well in cmd...");
		}


		//요청에 따라 만들어진 패킷을 전송
		write(sock,&req_packet, sizeof(ADMIN_REQ_PACKET));
		//서버 응답까지 대기
		int read_size;
		(read_size = read(sock,&res_packet, sizeof(ADMIN_RES_PACKET)));
		printf("read_size = %d, admin_res_packet_size = %ld\n",read_size,sizeof(ADMIN_RES_PACKET));

		// 먼저 받아온 items 와 cnts를 동기화 시킵니다.
		synchronize_server( res_packet );
		printf("!! %d  %d %s!!\n", res_packet.cmd,res_packet.result, res_packet.items[total_item_cnt-1].name );

		// 결과가 성공일 때만 수행합니다.
		if( res_packet.result == 1 ){
			
			// 추가 요청 처리 완료 후 처리
			if( res_packet.cmd == ADD_ITEM ){
				

				printf("	========== Here's Changed Menu ==========\n");
				printf("category	key	name		stock	price\n");
				for(int i = 0 ; i < total_item_cnt ; i++){
					// 우리가 요청한 패킷의 카테고리와 같은 메뉴만 보여주면 되니까요.
					if(items[i].category == req_packet.item.category){
					display_single_item(items[i]);
					}
				}
				printf("\nPRESS Enter");
				getchar();getchar();	// 앞에서 \n 받고 enter 로 넘어감
			}
			// SHOW 요청 처리 완료 후 처리 
			else if(res_packet.cmd == SHOW_ITEM){
				int category = -1;
				puts("\n\t ========== What items do you want to see ITEM? ==========");
				puts("\t1. coffee");
				puts("\t2. tea");
				puts("\t3. juice");
				puts("\t4. brunch");
				puts("\t5. all");

				while(1){
					printf("input : ");
					scanf("%d", &category);
					if( (category > 0 ) && (category < 6)) break;
					puts("Please input valid value ( 1 ~ 5 ) : ");
				}

				puts("\n\t ========== Here's the list! ==========\n");
				for(int i = 0 ; i < total_item_cnt ; i++)
				{
					if( (items[i].category == category) || (category == 5) ) display_single_item(items[i]);
				}

			}
			// update 요청 처리 완류 후 처리
			else if(res_packet.cmd == UPDATE_ITEM){
				//업데이트 된 메뉴만 보여줍시다. res_packet에 우리가 넣은 카테고리와 키가 없는 이유로 그냥 req_packet을 사용합니다.
				int idx = find_item_idx_by_category_and_key(req_packet.item.category, req_packet.item.key);
				puts("\n ========== UPDATED! ==========");
				puts("category	key	name		stock	price");
				display_single_item(items[idx]);
			}
			// delete 요청 처리 완료 후 처리
			else if(res_packet.cmd == DELETE_ITEM){
				printf("	========== Here's Changed Menu ==========\n");
				printf("category	key	name		stock	price\n");
				for(int i = 0 ; i < total_item_cnt ; i++){
					// 우리가 요청한 패킷의 카테고리와 같은 메뉴만 보여주면 되니까요.
					if(items[i].category == req_packet.item.category){
					display_single_item(items[i]);
					}
				}
				printf("\nPRESS Enter");
				getchar();getchar();	// 앞에서 \n 받고 enter 로 넘어감
			}

		}
		else{
			puts("Oops, your request has been denied");
		}

	}
}

void print_welcome_msg(){
	puts("		============ MAIN ===========");
	
	printf("	1: add item\n	2: show items\n	3: update item\n	4: delete item\n	5: show all customers\n	6: quit\n\ninput :");
}

// 한 ITEM 구조체를 [	카테고리	키	이름	수량	가격	] 형식으로 출력합니다. 각 속성 사이 공백은 탭입니다.
void display_single_item(ITEM item){
	printf("	%d	%d	%-8s	%d	%d\n", item.category,  item.key, item.name, item.stock, item.price);
}

// 각 요청이 끝난 뒤, 받아온 server의 items와 cnts를 동기화 시키는 작업입니다.
void synchronize_server(ADMIN_RES_PACKET res_packet){
	total_item_cnt = res_packet.cnts[0];	//total_cnts
	coffee_cnt = res_packet.cnts[COFFEE];
	tea_cnt = res_packet.cnts[TEA];
	juice_cnt = res_packet.cnts[JUICE];
	brunch_cnt = res_packet.cnts[BRUNCH];
	for(int i = 0 ; i < total_item_cnt ; i++){
		items[i] = res_packet.items[i];
	}
}


// 아래는 요청에 따른 처리 함수들인데, 공통적으로 요청 패킷에 요청에 필요한 값들을 넘겨주기만 합니다.
// 사용자 목록 불러오는 건 좀 다를 수도 있겠네요. 잘 구현해주세요!

void add_item(ADMIN_REQ_PACKET* req_packet){
	

	//ITEM new_menu 에 아이템 속성을 입력받아 추가함.

	puts("\n\t ========== ADD MENU ==========");
	puts("\tWhich category do you want to add?");
	puts("\t1. Coffee	2. Tea");
	puts("\t3. juice	4. BRUNCH");
	printf(" input : ");
	while(1){
		scanf("%d",&req_packet->item.category);
		if( (req_packet->item.category < 1) || (req_packet->item.category > CATEGORY_SIZE) ){
			printf("	< Please Enter valid value ( 1 ~ %d ) > : ", CATEGORY_SIZE);
			continue;
		}
		break;
	}
	puts("Please insert the Name, stock, price of the menu\n");

	// 하나씩 입력하는게 직관적일 거 같아서 바꿨습니다.
	while(1){
		printf("[Menu Name] : ");
		scanf("%s", req_packet->item.name);

		printf("[Item Stock] : ");
		scanf("%d", &req_packet->item.stock);

		printf("[Item Price] : ");
		scanf("%d", &req_packet->item.price);

		//error handling?
		break;
	}

}

void show_item(ADMIN_REQ_PACKET * req_packet){

	// 통일성 유지용 함수

}

void update_item(ADMIN_REQ_PACKET * req_packet){

	puts("\n\t ========== UPDATE MENU ==========");

	// 수정할 카테고리와 키를 입력하는 과정
	puts("\tWhich category do you want to update?");
	puts("\t1. Coffee	2. Tea");
	puts("\t3. juice	4. BRUNCH");
	printf(" input : ");
	while(1){
		scanf("%d",&req_packet->item.category);
		if( (req_packet->item.category < 1) || (req_packet->item.category > CATEGORY_SIZE) ){
			printf("	< Please Enter valid value ( 1 ~ %d ) > : ", CATEGORY_SIZE);
			continue;
		}
		break;
	}

	// 잘못된 입력에 대한 건 나중에 생각하겠습니다

	printf("Which key do you want to modify? :");
	scanf("%d",&req_packet->item.key);

	
	
	int modi = 0;
	printf("What value you want to modify? 1. stock / 2. price : ");
	scanf("%d", &modi);
	
	if(modi == 1){
		printf("[Item Stock] : ");
		scanf("%d", &req_packet->item.stock);
		req_packet->item.price = -1;	// -1 --> 이 값은 변경하지 말아라
	}else if(modi == 2){
		printf("[Item Price] : ");
		scanf("%d", &req_packet->item.price);
		req_packet->item.stock = -1;
	}

}

void delete_item(ADMIN_REQ_PACKET * req_packet){

	// 예도 입력 오류는 나중에 생각할게요
	puts("\n\t =========== DELETE ITEM ==========");
	puts("Please enter the properties of deleting item");
	printf("[CATEGORY] ( 1. coffee 2. tea 3. juice 4. brunch): ");
	scanf("%d", &req_packet->item.category);
	printf("[KEY] : ");
	scanf("%d", &req_packet->item.key);
}