#ifndef CAFE_H
#define CAFE_H
#define BUF_SIZE 2048
#define MAX_ITEM 100
#define MENU_NAME_SIZE 20
#define PWD_SIZE 20
#define ADMIN_PWD "password"
#define BACKUP_FREQUENCY 10

#define ADMIN 1
#define CLIENT 2

#define CATEGORY_SIZE 4
#define COFFEE 1
#define TEA 2
#define JUICE 3
#define BRUNCH 4
#define ALL_CATEGORY 5

// cmd for client
#define ORDER 1
#define QUIT 2

// cmd for admin
#define RETURN_MAIN -1
#define ADD_ITEM 1
#define SHOW_ITEM 2
#define UPDATE_ITEM 3
#define DELETE_ITEM 4
#define SHOW_CUSTOMER 5
#define ADMIN_QUIT 6

// waiting(sleep) time
#define W_COFFEE 2
#define W_TEA 2
#define W_JUICE 1
#define W_BRUNCH 3

// order status
// #define WAITING 1
#define READY 2
#define OUT_OF_STOCK 3
#define STOCK_LACK 4

// 파일에는 name key stock price 순으로 저장됨
typedef struct ITEM
{
	int category;
	int key; // primary key per item category
	char name[MENU_NAME_SIZE];
	int stock;
	int price;
} ITEM;

// for client
typedef struct RECENT_MENU
{
	ITEM items[MAX_ITEM];
	int cnts[CATEGORY_SIZE + 1];
} RECENT_MENU;

typedef struct REQ_PACKET
{
	int cmd;
	int item_category;
	int item_key;
	int quantity;
} REQ_PACKET;

typedef struct RES_PACKET
{
	int cmd;
	int result;
	char res_msg[BUF_SIZE];
	ITEM items[MAX_ITEM];
} RES_PACKET;

// 어드민의 패킷은 따로 설정했습니다
typedef struct ADMIN_REQ_PACKET
{
	int cmd;
	ITEM item;
} ADMIN_REQ_PACKET;
typedef struct ADMIN_RES_PACKET
{
	int cmd;
	int result;
	char res_msg[BUF_SIZE];
	// 생각해보니 서버랑 어드민이 동기화 되려면 cnt 값들도 다시 다 줘야해요
	// 그래서 cnts = {total_cnt, coffee_cnt, tea_cnt, juice_cnt, brunch_cnt} 로 선언해서 넘겨줄겁니다
	int cnts[CATEGORY_SIZE + 1];
	// 얘도 어드민과 아이템 배열 동기화 해주기위해 해주는겁니다.
	ITEM items[MAX_ITEM];
} ADMIN_RES_PACKET;

// admin login을 위한 packet
typedef struct ADMIN_LOGIN_REQ_PACKET
{
	char password[PWD_SIZE];
} ADMIN_LOGIN_REQ_PACKET;
typedef struct ADMIN_LOGIN_RES_PACKET
{
	int result;
} ADMIN_LOGIN_RES_PACKET;

#endif

extern ITEM items[MAX_ITEM];
extern int total_item_cnt;
extern int coffee_cnt;
extern int tea_cnt;
extern int juice_cnt;
extern int brunch_cnt;

void backup_warning(int signum); // signal handler: SIGINT 발생 시 경고 문구
void return_main();
void clear_terminal();
void restore_menu();
int find_item_idx_by_category_and_key(int item_category, int item_key);
void error_handling(char *msg);
int get_item_size_per_category(int category);
int get_visible_length(const char *);
void print_centered(const char *);
void initialize_item_info(RECENT_MENU recent_menu);