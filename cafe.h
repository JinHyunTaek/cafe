#ifndef CAFE_H
#define	CAFE_H
#define BUF_SIZE 2048
#define MAX_ITEM 10000
#define NAME_SIZE 20
#define PWD_SIZE 10

typedef struct ITEM{
	int key;
	char name[NAME_SIZE];
	int stock; //not yet
	int price; //not yet
}ITEM;

typedef struct USER{
	char name[NAME_SIZE];
	char password[PWD_SIZE];
}USER;

typedef struct REQ_PACKET{
	int cmd;
	int item_no;
}REQ_PACKET;

typedef struct RES_PACKET{
	int cmd;
	int result;
	char res_msg[BUF_SIZE];
}RES_PACKET;
#endif