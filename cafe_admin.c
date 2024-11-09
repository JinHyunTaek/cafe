#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include "cafe.h"

void *handle_customer(void *arg);
void print_welcome_msg();

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
	//get item info from server (sequence : write() -> read())
	int me = ADMIN;
	write(sock,&me,sizeof(me));
	restore_menu();
    pthread_create(&t_id, NULL, handle_customer, (void *)&sock);
	pthread_join(t_id, NULL);
	close(sock);
}

void *handle_customer(void *arg){
	int cmd;
	int sock = *(int*)arg;
	REQ_PACKET req_packet;
	RES_PACKET res_packet;
	memset(&req_packet,0,sizeof(REQ_PACKET));
	memset(&res_packet,0,sizeof(RES_PACKET));
	while(1){
		print_welcome_msg();
		scanf("%d",&cmd);
	}
}

void print_welcome_msg(){
	puts("============Hello Admin!===========");
	for(int i=0;i<MAX_ITEM;i++){
		if(!strlen(items[i].name))
		break;
		puts(items[i].name);
	}
	printf("1: add item, 2:get items 3: update item, 4: delete item, 5: get all customers 6: quit:");
}