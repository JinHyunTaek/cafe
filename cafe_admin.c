#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>
#include <termio.h>
#include <sys/ioctl.h>
#include "cafe.h"

void handle_admin(int sock);
void error_handling(char *msg);

int login(int sock);
void tty_mode(int);
void set_noecho_mode();

void print_welcome_msg();
void print_nav();
void print_category();
void print_menu_list(ADMIN_REQ_PACKET);
void display_single_item(ITEM item);

void synchronize_server(ADMIN_RES_PACKET);

void add_item(ADMIN_REQ_PACKET *);
void show_item(ADMIN_REQ_PACKET *);
void update_item(ADMIN_REQ_PACKET *);
void delete_item(ADMIN_REQ_PACKET *);

// 되돌아가기를 구현하려니 코드가 너무 길어져서 좀 모듈화했습니다.
// -1을 입력받으면 cmd 값을 -1로 입력받고 이게 메뉴로 돌아가기 flag 입니다.
void get_category_or_key_input(ADMIN_REQ_PACKET *req_packet, int mode);

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
	signal(SIGINT, backup_warning); // signal handler 등록
	sock = socket(PF_INET, SOCK_STREAM, 0);

	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = inet_addr(argv[1]);
	serv_addr.sin_port = htons(atoi(argv[2]));

	// connect request to server (blocked until accept() is called)
	if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == -1)
		error_handling("connect() error");

	// 내가 어드민이라는 신호를 소켓에 적용
	int me = ADMIN;
	write(sock, &me, sizeof(me));
	handle_admin(sock);
	close(sock);
}

void handle_admin(int sock)
{
	int cmd;
	char dummy = 0;
	int islogin = 0;
	ADMIN_REQ_PACKET req_packet;
	ADMIN_RES_PACKET res_packet;
	RECENT_MENU recent_menu;
	memset(&req_packet, 0, sizeof(ADMIN_REQ_PACKET));
	memset(&res_packet, 0, sizeof(ADMIN_RES_PACKET));

	while (1)
	{
		clear_terminal();
		while (islogin == 0)
		{
			islogin = login(sock);
		}
		
		// 메뉴로 돌아올 때 마다 메뉴를 최신화
		write(sock, &dummy, sizeof(dummy));
		if (read(sock, &recent_menu, sizeof(RECENT_MENU)) < sizeof(RECENT_MENU))
			error_handling("read()");
		initialize_item_info(recent_menu);

		// 언제든 -1 입력 시 메뉴로 돌아감을 표기
		print_centered("\033[1;37;44mInsert -1 anytime to go Menu\033[0m\n\n");
		print_welcome_msg();
		scanf("%d", &req_packet.cmd);

		switch (req_packet.cmd)
		{
		case RETURN_MAIN:
			return_main();
			break;
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
			// 현재 미구현이라 메뉴로 돌아가는걸로 해둘게요
			req_packet.cmd = -1;
			break;

		case ADMIN_QUIT:
			write(sock, &req_packet, sizeof(ADMIN_REQ_PACKET));
			system("clear");
			return;
		default:
			puts("Something didnt go well in cmd...");
		}

		// 요청에 따라 만들어진 패킷을 전송
		write(sock, &req_packet, sizeof(ADMIN_REQ_PACKET));
		// 서버 응답까지 대기
		int read_size;
		(read_size = read(sock, &res_packet, sizeof(ADMIN_RES_PACKET)));

		// 먼저 받아온 items 와 cnts를 동기화 시킵니다.
		synchronize_server(res_packet);

		// 결과가 성공일 때만 수행합니다.
		if (res_packet.result == 1)
		{
			switch (res_packet.cmd)
			{

			case ADD_ITEM:
				print_nav();
				print_menu_list(req_packet);
				return_main();
				break;
			case SHOW_ITEM:
				print_nav();
				print_menu_list(req_packet);
				puts("");
				return_main();
				break;
			case UPDATE_ITEM:
				// 업데이트 된 메뉴만 보여줍시다. res_packet에 우리가 넣은 카테고리와 키가 없는 이유로 그냥 req_packet을 사용합니다.
				int idx = find_item_idx_by_category_and_key(req_packet.item.category, req_packet.item.key);
				print_nav();
				display_single_item(items[idx]);
				puts("");
				return_main();
				break;
			case DELETE_ITEM:
				print_nav();
				print_menu_list(req_packet);
				return_main();
				break;
			default:

				break;
			}
		}
		else
		{
			puts("Request has been denied");
			if (res_packet.cmd == ADD_ITEM && res_packet.result == -1)
			{
				puts("Reason : Item name already exists");
				return_main();
			}
		}
	}
}

int login(int sock)
{
	ADMIN_LOGIN_REQ_PACKET log_req_packet;
	ADMIN_LOGIN_RES_PACKET log_res_packet;

	tty_mode(0);	   // 현재 모드 저장
	set_noecho_mode(); // nonecho mode 설정
	while (1)
	{
		memset(&log_req_packet, 0, sizeof(ADMIN_LOGIN_REQ_PACKET));
		memset(&log_res_packet, 0, sizeof(ADMIN_LOGIN_RES_PACKET));

		printf("Admin password: ");
		scanf("%s", log_req_packet.password);

		write(sock, &log_req_packet, sizeof(ADMIN_LOGIN_REQ_PACKET));

		read(sock, &log_res_packet, sizeof(ADMIN_LOGIN_RES_PACKET));

		if (log_res_packet.result == 1)
		{
			clear_terminal();
			tty_mode(1); // 저장한 설정 불러오기(nonecho 해제)
			return 1;
		}
		else
		{
			clear_terminal();
			printf("Wrong Password!(%s)\n\n", log_req_packet.password);
		}
	}
}

void tty_mode(int how)
{
	static struct termios orig_mode;
	if (how == 0)
		tcgetattr(0, &orig_mode); // 설정값 저장
	else if (how == 1)
		tcsetattr(0, TCSANOW, &orig_mode); // 설정값 불러오기
}

void set_noecho_mode()
{
	struct termios ttyinfo;
	if (tcgetattr(0, &ttyinfo) == -1)
	{
		perror("tcgetattr");
		exit(EXIT_FAILURE);
	}
	ttyinfo.c_lflag &= ~ECHO; // disable ECHO bit
	ttyinfo.c_cc[VMIN] = 1;
	if (tcsetattr(0, TCSANOW, &ttyinfo) == -1)
	{
		perror("tcsetattr");
		exit(EXIT_FAILURE);
	}
}

void print_welcome_msg()
{
	print_centered("============ Main ============\n");
	print_centered("1: Add item\n");
	print_centered("2: Show items\n");
	print_centered("3: Update item\n");
	print_centered("4: Delete item\n");
	print_centered("5: Show all customers\n");
	print_centered("6: Quit\n");
	print_centered("\nSelect Option: ");
}

void print_nav()
{
	print_centered("============ Menu ============\n");
	char buffer[256];
	snprintf(buffer, sizeof(buffer), "%-16s%-16s%-16s%-16s%-16s\n", "category", "key", "name", "stock", "price");

	print_centered(buffer);
}

void print_category()
{
	print_centered("========== Category ==========\n");
	print_centered("1. Coffee\n");
	print_centered("2. Tea\n");
	print_centered("3. Juice\n");
	print_centered("4. Brunch\n");
}

void print_menu_list(ADMIN_REQ_PACKET req_packet)
{
	for (int i = 0; i < total_item_cnt; i++)
	{
		// 우리가 요청한 패킷의 카테고리와 같은 메뉴만 보여주면 되니까요.
		if (items[i].category == req_packet.item.category)
		{
			display_single_item(items[i]);
		}
	}
}

// 한 ITEM 구조체를 [	카테고리	키	이름	수량	가격	] 형식으로 출력합니다. 각 속성 사이 공백은 탭입니다.
void display_single_item(ITEM item)
{
	char buffer[256];
	snprintf(buffer, sizeof(buffer), "%-16d%-16d%-16s%-16d%-16d\n", item.category, item.key, item.name, item.stock, item.price);

	print_centered(buffer);
}

// 각 요청이 끝난 뒤, 받아온 server의 items와 cnts를 동기화 시키는 작업입니다.
void synchronize_server(ADMIN_RES_PACKET res_packet)
{
	total_item_cnt = res_packet.cnts[0]; // total_cnts
	coffee_cnt = res_packet.cnts[COFFEE];
	tea_cnt = res_packet.cnts[TEA];
	juice_cnt = res_packet.cnts[JUICE];
	brunch_cnt = res_packet.cnts[BRUNCH];
	for (int i = 0; i < total_item_cnt; i++)
	{
		items[i] = res_packet.items[i];
	}
}

// 아래는 요청에 따른 처리 함수들인데, 공통적으로 요청 패킷에 요청에 필요한 값들을 넘겨주기만 합니다.
// 사용자 목록 불러오는 건 좀 다를 수도 있겠네요. 잘 구현해주세요!

void add_item(ADMIN_REQ_PACKET *req_packet)
{
	// ITEM new_menu 에 아이템 속성을 입력받아 추가함.
	print_centered("========== ADD Menu ==========\n");
	print_category();
	printf("\nSelect Category: ");
	get_category_or_key_input(req_packet, 0);
	if (req_packet->cmd == -1)
		return;
	printf("\nInsert Name, Stock, Price of the menu\n");

	// 하나씩 입력하는게 직관적일 거 같아서 바꿨습니다.
	while (1)
	{

		// 카테고리, 키 입력은 모듈화 했는데 각 기능마다 세부하게 다른건 이렇게 if 문 써서 하는 수밖에 없을거 같아요
		printf("[Menu Name]: ");
		scanf("%s", req_packet->item.name);
		if (strcmp(req_packet->item.name, "-1") == 0)
		{
			req_packet->cmd = -1;
			return;
		}

		printf("[Item Stock]: ");
		scanf("%d", &req_packet->item.stock);
		if (req_packet->item.stock == -1)
		{
			req_packet->cmd = -1;
			return;
		}

		printf("[Item Price]: ");
		scanf("%d", &req_packet->item.price);
		if (req_packet->item.price == -1)
		{
			req_packet->cmd = -1;
			return;
		}

		// error handling?
		break;
	}
}

void show_item(ADMIN_REQ_PACKET *req_packet)
{
	print_category();
	print_centered("\nSelect Category: ");
	get_category_or_key_input(req_packet, 0);
}

void update_item(ADMIN_REQ_PACKET *req_packet)
{

	print_centered("========= Update Menu ========\n");
	// 수정할 카테고리와 키를 입력하는 과정
	print_category();
	printf("\nSelect Category: ");
	get_category_or_key_input(req_packet, 0);
	if (req_packet->cmd == -1)
		return;

	print_nav();
	print_menu_list(*req_packet);
	printf("Select Key:");
	get_category_or_key_input(req_packet, 1);
	if (req_packet->cmd == -1)
		return;

	int modi = 0;
	printf("Select Modify Option [1. Stock / 2. Price] : ");
	scanf("%d", &modi);

	// 아래도 모두 -1 입력시 돌아가기 기능 구현
	if (modi == -1)
	{
		req_packet->cmd = -1;
		return;
	}

	if (modi == 1)
	{
		printf("[Item Stock] : ");
		scanf("%d", &req_packet->item.stock);
		if (req_packet->item.stock == -1)
		{
			req_packet->cmd = -1;
			return;
		}
		req_packet->item.price = -1; // -1 --> 이 값은 변경하지 말아라
	}
	else if (modi == 2)
	{
		printf("[Item Price] : ");
		scanf("%d", &req_packet->item.price);
		if (req_packet->item.price == -1)
		{
			req_packet->cmd = -1;
			return;
		}
		req_packet->item.stock = -1;
	}
	else
	{
		puts("Wrong input!");
		req_packet->cmd = -1;
	}
}

void delete_item(ADMIN_REQ_PACKET *req_packet)
{

	// 예도 입력 오류는 나중에 생각할게요
	print_centered("========= Delete Menu ========\n");
	print_category();
	printf("Select Category: ");
	get_category_or_key_input(req_packet, 0);
	if (req_packet->cmd == -1)
		return;

	print_nav();
	print_menu_list(*req_packet);

	printf("Select Key: ");
	get_category_or_key_input(req_packet, 1);
	if (req_packet->cmd == -1)
		return;
}

// 카테고리 및 키 입력을 req_packet에 넘겨주는 기능을 합니다. 이 떄 mode == 0 이면 카테고리, 1 이면 키 입력을 뜻합니다.
void get_category_or_key_input(ADMIN_REQ_PACKET *req_packet, int mode)
{
	// 카테고리를 입력 받는 경우입니다.
	if (mode == 0)
	{
		while (1)
		{
			scanf("%d", &req_packet->item.category);
			if ((req_packet->item.category < 1) || (req_packet->item.category > CATEGORY_SIZE))
			{
				// -1 이면 되돌아가기
				if (req_packet->item.category == -1)
				{
					req_packet->cmd = -1;
					return;
				}
				printf("	< Please Enter valid value ( 1 ~ %d ) > : ", CATEGORY_SIZE);
				continue;
			}

			break;
		}
	}
	// 키를 입력 받는 경우입니다.
	if (mode == 1)
	{
		int key_size;
		switch (req_packet->item.category)
		{
		case COFFEE:
			key_size = coffee_cnt;
			break;
		case TEA:
			key_size = tea_cnt;
			break;
		case JUICE:
			key_size = juice_cnt;
			break;
		case BRUNCH:
			key_size = brunch_cnt;
			break;
		default:
			puts("wrong category");
			break;
		}
		while (1)
		{
			scanf("%d", &req_packet->item.key);
			if ((req_packet->item.key < 1) || (req_packet->item.key > key_size))
			{
				// -1 이면 되돌아가기
				if (req_packet->item.key == -1)
				{
					req_packet->cmd = -1;
					return;
				}
				printf("	< Please Enter valid value ( 1 ~ %d ) > : ", key_size);
				continue;
			}
			break;
		}
	}
}