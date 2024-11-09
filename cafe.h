#ifndef CAFE_H
#define	CAFE_H
#define BUF_SIZE 2048
#define MAX_ITEM 10000
#define MENU_NAME_SIZE 20
#define PWD_SIZE 10

#define ADMIN 1
#define CLIENT 2

#define CATEGORY_SIZE 4
#define COFFEE 1
#define TEA 2
#define JUICE 3
#define BRUNCH 4

// cmd
#define ORDER 1
#define QUIT 2

// waiting(sleep) time
#define W_COFFEE 2
#define W_TEA 2
#define W_JUICE 1
#define W_BRUNCH 3

// order status
// #define WAITING 1
#define READY 2
#define OUT_OF_STOCK 3

typedef struct ITEM{
	int category;
	int key; // primary key per item category
	char name[MENU_NAME_SIZE];
	int stock; 
	int price; 
}ITEM;

// typedef struct USER{
// 	char name[NAME_SIZE];
// 	char password[PWD_SIZE];
// }USER;

typedef struct REQ_PACKET{
	int cmd;
	int item_category;
	int item_key;
}REQ_PACKET;

typedef struct RES_PACKET{
	int result;
	char res_msg[BUF_SIZE];
}RES_PACKET;
#endif

extern ITEM items[MAX_ITEM];
extern int total_item_cnt;
extern int coffee_cnt;
extern int tea_cnt;
extern int juice_cnt;
extern int brunch_cnt;

void restore_menu();
int find_item_idx_by_category_and_key(int item_category, int item_key);
void error_handling(char *msg);
// void restore_user();