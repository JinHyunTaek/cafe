#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>
#include <sys/time.h>
#include "cafe.h"
#define MAX_USER 10000

int clnt_socks[MAX_USER];
int clnt_cnt; // (connected)
int waiting_clnt;

pthread_mutex_t admin_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t client_mutex = PTHREAD_MUTEX_INITIALIZER;

RECENT_MENU make_recent_menu();
void make_menu(int item_category, int item_key, int quantity, char *res_msg, int *result);
void backup(int signum); // signal handler : 종료 시 백업
void error_handling(char *msg);
void *handle_clnt(void *arg);
void login(int sock);
void *handle_admin(void *args);
void remove_clnt(int clnt_sock);

void synchronize(int what_category);
void add_item_to_res_packet(ADMIN_RES_PACKET *);
int get_item_size_per_category(int category);

int admin_add_item(ITEM);
int admin_show_item(ADMIN_RES_PACKET *);
int admin_update_item(ITEM);
int admin_delete_item(ITEM);

// 타이머를 통한 주기적 백업 기능 구현을 위한 함수들입니다.
int set_ticker(int timer_secs)
{
	// 원래 받는 단위는 msec긴 한데, 백업을 msec 단위로 할 일은 없을거 같아서
	// 그냥 sec 로 퉁치겠습니다.
	int msecs = timer_secs * 1000;

	struct itimerval new_timeset;
	long secs, usecs;

	secs = (long)(msecs / 1000);
	usecs = (long)(msecs % 1000) * 1000L;

	new_timeset.it_value.tv_sec = secs;
	new_timeset.it_value.tv_usec = usecs;

	new_timeset.it_interval.tv_sec = secs;
	new_timeset.it_interval.tv_usec = usecs;

	return setitimer(ITIMER_REAL, &new_timeset, NULL);
}

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

	// 주기적 백업을 위한 타이머 설정입니다.
	set_ticker(BACKUP_FREQUENCY);
	// 백업을 위한 signal handler 등록
	signal(SIGINT, backup);
	signal(SIGALRM, backup);

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
		else if (is_admin == CLIENT)
			pthread_create(&t_id, NULL, handle_clnt, (void *)&clnt_sock); // create thread per client, handle handle_clnt func
		else
			error_handling("who are you?");
		pthread_detach(t_id);
	}
	// 종료시 delete mutex, 서버 소켓도 닫음
	pthread_mutex_destroy(&admin_mutex);
	pthread_mutex_destroy(&client_mutex);
	close(serv_sock);
}

// signal handler : 종료 시 백업
void backup(int signum)
{
	FILE *coffee_fp = fopen("item/coffee.txt", "wt");
	FILE *tea_fp = fopen("item/tea.txt", "wt");
	FILE *juice_fp = fopen("item/juice.txt", "wt");
	FILE *brunch_fp = fopen("item/brunch.txt", "wt");

	if (!coffee_fp || !tea_fp || !juice_fp || !brunch_fp)
	{
		fprintf(stderr, "menu file open error\n");
		exit(1);
	}

	for (int i = 0; i < total_item_cnt; i++)
	{
		FILE *fp;
		switch (items[i].category)
		{
		case COFFEE:
			fp = coffee_fp;
			break;
		case TEA:
			fp = tea_fp;
			break;
		case JUICE:
			fp = juice_fp;
			break;
		case BRUNCH:
			fp = brunch_fp;
			break;
		}
		fprintf(fp, "%s %d %d %d\n", items[i].name, items[i].key, items[i].stock, items[i].price);
	}

	fclose(coffee_fp);
	fclose(tea_fp);
	fclose(juice_fp);
	fclose(brunch_fp);

	// 어떤 신호에 의한 백업인지 출력
	puts("BACK Up COMPLETED");
	printf("by SIGNAL = %d \n", signum);

	// SIGINT의 경우에만 종료
	if (signum == SIGINT)
		exit(0);
}

// 소비자 전용 핸들러
void *handle_clnt(void *arg)
{
	char dummy = 0;
	int clnt_sock = *(int *)arg;
	REQ_PACKET req_packet;
	RES_PACKET res_packet;
	RECENT_MENU recent_menu = make_recent_menu();
	if (read(clnt_sock, &dummy, sizeof(dummy)) < sizeof(dummy))
		{
			error_handling("read()");
		}
	write(clnt_sock, &recent_menu, sizeof(RECENT_MENU));
	while (1)
	{
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
			make_menu(req_packet.item_category, req_packet.item_key, req_packet.quantity, res_packet.res_msg, &(res_packet.result));
			memcpy(res_packet.items, items, sizeof(ITEM) * MAX_ITEM);
			// 클라이언트 소켓으로 반응 패킷 전달 ( 메뉴가 준비 다 되었다는 메세지 )
			write(clnt_sock, &res_packet, sizeof(RES_PACKET));
		}
	}
}

void login(int sock)
{
	ADMIN_LOGIN_REQ_PACKET log_req_packet;
	ADMIN_LOGIN_RES_PACKET log_res_packet;
	while (1)
	{
		memset(&log_req_packet, 0, sizeof(ADMIN_LOGIN_REQ_PACKET));
		memset(&log_res_packet, 0, sizeof(ADMIN_LOGIN_RES_PACKET));

		read(sock, &log_req_packet, sizeof(ADMIN_LOGIN_REQ_PACKET));
		if (strcmp(log_req_packet.password, ADMIN_PWD) == 0)
		{
			log_res_packet.result = 1;
			write(sock, &log_res_packet, sizeof(ADMIN_LOGIN_RES_PACKET));
			printf("\nAdmin Login!\n");
			break;
		}
		else
		{
			log_res_packet.result = 0;
			write(sock, &log_res_packet, sizeof(ADMIN_LOGIN_RES_PACKET));
		}
	}
}

// admin 전용 핸들러
void *handle_admin(void *arg)
{
	char dummy = 0;
	int admin_sock = *(int *)arg;
	ADMIN_REQ_PACKET req_packet;
	ADMIN_RES_PACKET res_packet;
	RECENT_MENU recent_menu;
	login(admin_sock);

	while (1)
	{ // 요청 패킷과 반응 패킷을 선언
		memset(&req_packet, 0, sizeof(ADMIN_REQ_PACKET));
		memset(&res_packet, 0, sizeof(ADMIN_RES_PACKET));
		memset(&recent_menu, 0, sizeof(RECENT_MENU));
		if (read(admin_sock, &dummy, sizeof(dummy)) < sizeof(dummy))
		{
			error_handling("read()");
		}
		recent_menu = make_recent_menu();
		write(admin_sock, &recent_menu, sizeof(RECENT_MENU));

		// 어드민 소켓의 요청을 패킷에 저장받아 처리
		read(admin_sock, &req_packet, sizeof(ADMIN_REQ_PACKET)); // blocked until write request is arrived by client

		res_packet.cmd = req_packet.cmd;
		pthread_mutex_lock(&admin_mutex); // most(ADD, UPDATE, DELETE) operations admin does need to be locked for synchronization
		// 어드민이 입력한 cmd 값에 따라 해당 기능들을 수행합니다.
		switch (req_packet.cmd)
		{
		case ADD_ITEM:
			res_packet.result = admin_add_item(req_packet.item);
			break;
		case SHOW_ITEM:
			res_packet.result = admin_show_item(&res_packet);
			break;
		case UPDATE_ITEM:
			res_packet.result = admin_update_item(req_packet.item);
			break;
		case DELETE_ITEM:
			res_packet.result = admin_delete_item(req_packet.item);
			break;
		case ADMIN_QUIT:
			remove_clnt(admin_sock);
			pthread_mutex_unlock(&admin_mutex);
			return NULL;
		default:
			break;
		}

		// 어드민과 서버 카운트, 아이템 배열 동기화를 위한 전달
		// 모든 기능에서 items를 수정하든 뭘하든 이 과정을 통해 동기화 됩니다.
		add_item_to_res_packet(&res_packet);

		pthread_mutex_unlock(&admin_mutex); // unlock when operation end
		// 어드민이 요청한 입력이 잘 처리 됐는지, 어떤 요청이 처리됐는지 등을 전해줍니다.
		write(admin_sock, &res_packet, sizeof(ADMIN_RES_PACKET));
		printf("Accomplished. cmd = %d, result = %d\n", res_packet.cmd, res_packet.result);
	}
}

// 메뉴가 만들어질 때까지 기다리고, 해당 메뉴의 이름 및 주문 상태(READY / OUT_OF_STOCK)를 응답 패킷에 전달하는 함수
void make_menu(int item_category, int item_key, int quantity, char *res_msg, int *result)
{
	int i = find_item_idx_by_category_and_key(item_category, item_key);
	// mutex와 item stock을 빼는 시간을 바꿔서 stock이 없는데 주문을 받는 경우를 없앰
	pthread_mutex_lock(&client_mutex);
	if (items[i].stock <= 0 )
	{
		sprintf(res_msg, "\nSorry. Item %s is currently out of stock.", items[i].name);
		*result = OUT_OF_STOCK;
		pthread_mutex_unlock(&client_mutex);
		return;
	}
	else if(items[i].stock-quantity<0){
		sprintf(res_msg, "\nSorry. There are only %d %s left.", items[i].stock, items[i].name);
		*result = STOCK_LACK;
		pthread_mutex_unlock(&client_mutex);
		return;
	}
	items[i].stock -= quantity;
	pthread_mutex_unlock(&client_mutex);
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
	sprintf(res_msg, "\nThank you for waiting! Your %s is now ready.", items[i].name);
	
	*result = READY;
}

// --------------------------------- 어드민 함수들 정의 ---------------------------------------
// 서버의 어드민 함수는 공통적으로 server의 items 배열을 수정하고, 이 성공 여부를 반환합니다.

// 메뉴 추가에 성공하면 1, 아니면 0를 반환합니다.
int admin_add_item(ITEM item)
{
	for (int i = 0; i < total_item_cnt; i++)
	{
		if (!strcmp(item.name, items[i].name))
		{
			return -1; // duplicated item name
		}
	}
	// 아이템의 카테고리에 맞춰 카테고리 메뉴 수와 전체 수를 더하고, server의 items 배열에 추가한 메뉴를 add 합니다.
	// 아이템에 따라 여는 파일의 이름도 다릅니다.
	switch (item.category)
	{
	case COFFEE:
		item.key = ++coffee_cnt;
		total_item_cnt++;
		items[total_item_cnt - 1] = item;
		break;
	case TEA:
		item.key = ++tea_cnt;
		total_item_cnt++;
		items[total_item_cnt - 1] = item;
		break;
	case JUICE:
		item.key = ++juice_cnt;
		total_item_cnt++;
		items[total_item_cnt - 1] = item;
		break;
	case BRUNCH:
		item.key = ++brunch_cnt;
		total_item_cnt++;
		items[total_item_cnt - 1] = item;
		break;
	default:
		puts("Guess you typed wrong category?");
		return 0;
	}

	// 추가한 메뉴가 있는 파일은 내용을 모두 지우고 서버의 정보로 동기화합니다.
	// synchronize(item.category);	<-- 로직 변경으로 불필요

	/* 무조건 필요한 과정은 아니긴하다만, 파일의 메뉴 순서로 ITEM 배열을
	다시 읽어옵니다. 이걸 하는 이유는, 지금 ITEM 배열의 끝에 추가한 아이템이 있잖아요? 그걸 같은 카테고리 애들이랑
	순서를 맞춰서 저장하고 싶은게 다입니다.*/
	// restore_menu(); // cafe.c 에 정의돼있음 --> synchronize 하지 않아서 불필요. 오히려 하면 안됩니다.

	// 성공적으로 함수 수행!
	return 1;
}

// show_item을 하기 위한 함수
int admin_show_item(ADMIN_RES_PACKET *res_packet)
{

	// for(int i = 0 ; i < total_item_cnt ; i++){
	// 	res_packet->items[i] = items[i];
	// }

	// add_item_to_res_packet 함수로 기능 이전. 사실상 구색 갖추기용 함수입니다.

	return 1;
}

// 업데이트를 하기 위한 함수
int admin_update_item(ITEM item)
{
	int modi_idx;
	modi_idx = find_item_idx_by_category_and_key(item.category, item.key);

	if (item.price == -1)
	{
		items[modi_idx].stock = item.stock;
	}
	else if (item.stock == -1)
	{
		items[modi_idx].price = item.price;
	}
	return 1;
}

// delete 하기 위한 함수
int admin_delete_item(ITEM item)
{
	int dele_idx, cate_size, curr_idx;
	dele_idx = find_item_idx_by_category_and_key(item.category, item.key);
	cate_size = get_item_size_per_category(item.category);

	// 삭제할 인덱스 번호부터 끝까지 아이템을 하나씩 당기며, 만약 삭제한 아이템과 카테고리가 같다면 키를 1씩 줄입니다.
	// 뒤에 추가된 아이템은 그 전 아이템보다는 키가 높을 것이니 이렇게 작성했습니다.
	// 이렇게 하면 아이템을 추가할 때는 따로 shift를 안 해도 됩니다.
	for (curr_idx = dele_idx; curr_idx < total_item_cnt - 1; curr_idx++)
	{
		items[curr_idx] = items[curr_idx + 1];
		if (items[curr_idx].category == item.category)
			items[curr_idx].key--;
	}

	switch (item.category)
	{
	case COFFEE:
		coffee_cnt--;
		break;
	case TEA:
		tea_cnt--;
		break;
	case JUICE:
		juice_cnt--;
		break;
	case BRUNCH:
		brunch_cnt--;
	}
	total_item_cnt--;
	return 1;
}

// 현재 서버의 items를 res_packet에 넘겨줍니다.
void add_item_to_res_packet(ADMIN_RES_PACKET *res_packet)
{
	for (int i = 0; i < total_item_cnt; i++)
	{
		res_packet->items[i] = items[i];
	}
	res_packet->cnts[0] = total_item_cnt;
	res_packet->cnts[1] = coffee_cnt;
	res_packet->cnts[2] = tea_cnt;
	res_packet->cnts[3] = juice_cnt;
	res_packet->cnts[4] = brunch_cnt;
}

int get_item_size_per_category(int category)
{
	switch (category)
	{
	case COFFEE:
		return coffee_cnt;
	case TEA:
		return tea_cnt;
	case JUICE:
		return juice_cnt;
	case BRUNCH:
		return brunch_cnt;
	}
	error_handling("wrong category");
}

// 서버의 아이템 정보 --> 파일에 동기화 시키는 매우 중요한 합수입니다.
void synchronize(int what_category)
{
	FILE *fp = NULL;

	if ((what_category == COFFEE) || (what_category == ALL_CATEGORY))
	{
		// coffee 카테고리 동기화
		fp = fopen("./item/coffee.txt", "wt");
		for (int i = 0; i < total_item_cnt; i++)
		{
			// 우리가 추가한 아이템의 카테고리와 같은 ITEM 구조체만 해당 카테고리 파일에 씁니다.
			if (items[i].category == COFFEE)
			{
				fprintf(fp, "%s %d %d %d\n", items[i].name, items[i].key, items[i].stock, items[i].price);
			}
		}
		fclose(fp);
	}

	if ((what_category == TEA) || (what_category == ALL_CATEGORY))
	{
		// tea 카테고리 동기화
		fp = fopen("./item/tea.txt", "wt");
		for (int i = 0; i < total_item_cnt; i++)
		{
			// 우리가 추가한 아이템의 카테고리와 같은 ITEM 구조체만 해당 카테고리 파일에 씁니다.
			if (items[i].category == TEA)
			{
				fprintf(fp, "%s %d %d %d\n", items[i].name, items[i].key, items[i].stock, items[i].price);
			}
		}
		fclose(fp);
	}

	if ((what_category == JUICE) || (what_category == ALL_CATEGORY))
	{
		// juice 카테고리 동기화
		fp = fopen("./item/tea.txt", "wt");
		for (int i = 0; i < total_item_cnt; i++)
		{
			// 우리가 추가한 아이템의 카테고리와 같은 ITEM 구조체만 해당 카테고리 파일에 씁니다.
			if (items[i].category == JUICE)
			{
				fprintf(fp, "%s %d %d %d\n", items[i].name, items[i].key, items[i].stock, items[i].price);
			}
		}
		fclose(fp);
	}

	if ((what_category == BRUNCH) || (what_category == ALL_CATEGORY))
	{
		// brunch 카테고리 동기화
		fp = fopen("./item/brunch.txt", "wt");
		for (int i = 0; i < total_item_cnt; i++)
		{
			// 우리가 추가한 아이템의 카테고리와 같은 ITEM 구조체만 해당 카테고리 파일에 씁니다.
			if (items[i].category == BRUNCH)
			{
				fprintf(fp, "%s %d %d %d\n", items[i].name, items[i].key, items[i].stock, items[i].price);
			}
		}
		fclose(fp);
	}
}

void restore_menu()
{
	memset(&items, 0, sizeof(ITEM) * MAX_ITEM);
	FILE *coffee_fp = fopen("item/coffee.txt", "r");
	FILE *tea_fp = fopen("item/tea.txt", "r");
	FILE *juice_fp = fopen("item/juice.txt", "r");
	FILE *brunch_fp = fopen("item/brunch.txt", "r");

	// 호출 때 마다 reset하지 않으면 cnt가 계속 커져서 막 두번 씩 출력되더라고요. 그래서 호출할 떄마다 cnt들을 초기화 합니다.
	//  reset_cnt();

	if (!coffee_fp || !tea_fp || !juice_fp || !brunch_fp)
	{
		fprintf(stderr, "menu file open error\n");
		exit(1);
	}
	while (!feof(coffee_fp))
	{
		fscanf(coffee_fp, "%s %d %d %d\n", items[total_item_cnt].name, &items[total_item_cnt].key, &items[total_item_cnt].stock, &items[total_item_cnt].price);
		items[total_item_cnt].category = COFFEE;
		coffee_cnt++;
		total_item_cnt++;
	}
	while (!feof(tea_fp))
	{
		fscanf(tea_fp, "%s %d %d %d\n", items[total_item_cnt].name, &items[total_item_cnt].key, &items[total_item_cnt].stock, &items[total_item_cnt].price);
		items[total_item_cnt].category = TEA;
		tea_cnt++;
		total_item_cnt++;
	}
	while (!feof(juice_fp))
	{
		fscanf(juice_fp, "%s %d %d %d\n", items[total_item_cnt].name, &items[total_item_cnt].key, &items[total_item_cnt].stock, &items[total_item_cnt].price);
		items[total_item_cnt].category = JUICE;
		juice_cnt++;
		total_item_cnt++;
	}
	while (!feof(brunch_fp))
	{
		fscanf(brunch_fp, "%s %d %d %d\n", items[total_item_cnt].name, &items[total_item_cnt].key, &items[total_item_cnt].stock, &items[total_item_cnt].price);
		items[total_item_cnt].category = BRUNCH;
		brunch_cnt++;
		total_item_cnt++;
	}
	fclose(coffee_fp);
	fclose(tea_fp);
	fclose(juice_fp);
	fclose(brunch_fp);
}

RECENT_MENU make_recent_menu()
{
	RECENT_MENU recent_menu;
	memcpy(recent_menu.items, items, sizeof(ITEM) * MAX_ITEM);
	recent_menu.cnts[0] = total_item_cnt;
	recent_menu.cnts[COFFEE] = coffee_cnt;
	recent_menu.cnts[TEA] = tea_cnt;
	recent_menu.cnts[JUICE] = juice_cnt;
	recent_menu.cnts[BRUNCH] = brunch_cnt;
	return recent_menu;
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