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

void make_menu(int item_category, int item_key, char *res_msg, int *result);
void backup();
void error_handling(char *msg);
void *handle_clnt(void *arg);
void *handle_admin(void *args);
void remove_clnt(int clnt_sock);


int admin_add_item(ITEM);

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

	// 서버 소켓 바인딩, 리스닝 작업
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

	// 서버를 부팅하며 item 폴더에 있는 아이템들을 cafe.c 에 있는 ITEM 구조체 items에 저장함.
	restore_menu();
	while (1)
	{
		// 클러아언트가 접속하면 클라이언트와 소켓 통신을 시작하고 각각의 쓰레드를 만들어서 작업
		clnt_adr_sz = sizeof(clnt_adr);
		clnt_sock = accept(serv_sock, (struct sockaddr *)&clnt_adr, &clnt_adr_sz);
		printf("Connected client IP: %s, clnt_sock:%d \n", inet_ntoa(clnt_adr.sin_addr), clnt_sock);
		clnt_socks[clnt_cnt++] = clnt_sock;
		int is_admin;
		if (read(clnt_sock, &is_admin, sizeof(is_admin)) == -1)
			error_handling("read()");
		// 쓰레드를 만들어 쓰레드 내에서 작업
		if (is_admin == ADMIN)
			pthread_create(&t_id, NULL, handle_admin, (void *)&clnt_sock); // create thread per client, handle handle_clnt func
		else if(is_admin == CLIENT)
			pthread_create(&t_id, NULL, handle_clnt, (void *)&clnt_sock); // create thread per client, handle handle_clnt func
		else
			error_handling("who are you?");
		pthread_detach(t_id);
	}
	// 종료시 쓰레드를 모두 닫고 서버 소켓도 닫음
	pthread_mutex_destroy(&mutex);
	close(serv_sock);
}

// 소비자 전용 핸들러
void *handle_clnt(void *arg)
{
	int clnt_sock = *(int *)arg;
	REQ_PACKET req_packet;
	RES_PACKET res_packet;
	while (1)
	{
		// 요청 패킷과 반응 패킷을 선언
		memset(&req_packet, 0, sizeof(REQ_PACKET));
		memset(&res_packet, 0, sizeof(RES_PACKET));
		// 클라이언트 소켓의 요청을 패킷에 저장받아 처리
		read(clnt_sock, &req_packet, sizeof(REQ_PACKET)); // blocked until write request is arrived by client
		// 요청 패킷의 cmd 값 ( 클라이언트가 입력한 명령어 ) 에 따라 아래 내용을 수행
		switch (req_packet.cmd)
		{
		case QUIT: // 클라이언트 종료
			remove_clnt(clnt_sock);
			return NULL;
		case ORDER: // 클라이언트가 주문 cmd 를 실행
			// 요청 패킷의 아이템 카테고리와 아이템 번호를 input으로 받아 반응 패킷의 msg를 저장 ( 메뉴 준비됐다는 메세지 )
			make_menu(req_packet.item_category, req_packet.item_key, res_packet.res_msg, &(res_packet.result));
			// 클라이언트 소켓으로 반응 패킷 전달 ( 메뉴가 준비 다 되었다는 메세지 )
			write(clnt_sock, &res_packet, sizeof(RES_PACKET));
		}
	}
}

// admin 전용 핸들러
void *handle_admin(void *arg)
{
	int admin_sock = *(int *)arg;
	ADMIN_REQ_PACKET req_packet;
	ADMIN_RES_PACKET res_packet;
	while (1)
	{
		// 요청 패킷과 반응 패킷을 선언
		memset(&req_packet, 0, sizeof(ADMIN_REQ_PACKET));
		memset(&res_packet, 0, sizeof(ADMIN_RES_PACKET));
		// 어드민 소켓의 요청을 패킷에 저장받아 처리
		read(admin_sock, &req_packet, sizeof(ADMIN_REQ_PACKET)); // blocked until write request is arrived by client
		
		res_packet.cmd = req_packet.cmd;

		// 어드민이 입력한 cmd 값에 따라 해당 기능들을 수행합니다.
		switch (req_packet.cmd)
		{
		case ADD_ITEM :
			res_packet.result = admin_add_item(req_packet.item);
			break;
		case SHOW_ITEM:
			break;
		case UPDATE_ITEM:
			break;
		case DELETE_ITEM:
			break;
		
		default:
			break;
		}

		// 어드민이 요청한 입력이 잘 처리 됐는지, 어떤 요청이 처리됐는지 등을 전해줍니다. 
		write(admin_sock, &res_packet, sizeof(ADMIN_RES_PACKET));
	}
}

// 메뉴가 만들어질 때까지 기다리고, 해당 메뉴의 이름 및 주문 상태(READY / OUT_OF_STOCK)를 응답 패킷에 전달하는 함수
void make_menu(int item_category, int item_key, char *res_msg, int *result)
{
	int i = find_item_idx_by_category_and_key(item_category, item_key);
	if (!items[i].stock)
	{
		sprintf(res_msg, "Sorry. Item %s is currently out of stock.", items[i].name);
		*result = OUT_OF_STOCK;
	}
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
	sprintf(res_msg, "Thank you for waiting! Your %s is now ready.", items[i].name);
	pthread_mutex_lock(&mutex);
	// 상품의 재고는 모든 스레드가 공유하는 자원이므로 상호 배제를 위한 동기화 필요
	items[i].stock -= 1;
	pthread_mutex_unlock(&mutex);
	*result = READY;
}



// 어드민 함수들 정의

// 메뉴 추가에 성공하면 1, 아니면 0를 반환합니다.
int admin_add_item(ITEM item){
	FILE * fp = NULL;
	
	// 아이템의 카테고리에 맞춰 카테고리 메뉴 수와 전체 수를 더하고, server의 items 배열에 추가한 메뉴를 add 합니다.
	// 아이템에 따라 여는 파일의 이름도 다릅니다.
	switch(item.category){
		case COFFEE:
			item.key = ++coffee_cnt;
			total_item_cnt ++;
			items[total_item_cnt-1] = item;
			fp = fopen("./item/coffee.txt", "wt");
			break;
		case TEA:
			item.key = ++tea_cnt;
			total_item_cnt ++;
			items[total_item_cnt-1] = item;
			fp = fopen("./item/tea.txt", "wt");
			break;
		case JUICE:
			item.key = ++juice_cnt;
			total_item_cnt ++;
			items[total_item_cnt-1] = item;
			fp = fopen("./item/juice.txt", "wt");
			break;
		case BRUNCH:
			item.key = ++brunch_cnt;
			total_item_cnt ++;
			items[total_item_cnt-1] = item;
			fp = fopen("./item/brunch.txt", "wt");
			break;
		default:
			puts("Guess you typed wrong category?");
			return 0;
	}

	// 추가한 메뉴가 있는 파일은 내용을 모두 지우고 서버의 정보로 동기화합니다.
		if(!fp){
			printf("fopen error\n");
			return 0;
		}
	for(int i = 0 ; i < total_item_cnt ; i++){
		// 우리가 추가한 아이템의 카테고리와 같은 ITEM 구조체만 해당 카테고리 파일에 씁니다.
		if(items[i].category == item.category){
			fprintf(fp,"%s %d %d %d\n", items[i].name, items[i].key, items[i].stock, items[i].price);
		}
	}

	// 이유는 모르겠는데 puts 대신 printf를 하면 출력이 안되네요. 아마 \n를 쓰면 되는걸 보면 버퍼 문제인거 같은데
	// 그냥 puts 쓰면 되니까 puts썼고 어차피 디버깅용이라 후에 지워도 무방합니다.
	puts("Iclose!");
	fclose(fp);

	/* 무조건 필요한 과정은 아니긴하다만, 파일의 메뉴 순서로 ITEM 배열을 
	다시 읽어옵니다. 이걸 하는 이유는, 지금 ITEM 배열의 끝에 추가한 아이템이 있잖아요? 그걸 같은 카테고리 애들이랑
	순서를 맞춰서 저장하고 싶은게 다입니다.*/
	restore_menu(); // cafe.c 에 정의돼있음

	// 성공적으로 함수 수행!
	return 1;

}




// 클라이언트 접속해제
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