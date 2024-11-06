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

ITEM items[MAX_ITEM];
USER users[MAX_USER];
int clnt_socks[MAX_USER];
int clnt_cnt; // (connected)
int item_cnt;
pthread_mutex_t mutex;

void resfore_files();
void backup();
void error_handling(char *msg);
void *handle_clnt(void *arg);

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
	resfore_files();
	while (1)
	{
		clnt_adr_sz = sizeof(clnt_adr);
		clnt_sock = accept(serv_sock, (struct sockaddr *)&clnt_adr, &clnt_adr_sz);
		printf("Connected client IP: %s, clnt_sock:%d \n", inet_ntoa(clnt_adr.sin_addr), clnt_sock);
		clnt_socks[clnt_cnt++] = clnt_sock;
		//send item info to client
		write(clnt_sock,&items,sizeof(ITEM)*item_cnt);

		pthread_create(&t_id, NULL, handle_clnt, (void *)&clnt_sock); // create thread per client, handle handle_clnt func
		pthread_detach(t_id);
	}
	pthread_mutex_destroy(&mutex);
	close(serv_sock);
}

void *handle_clnt(void *arg){
	int clnt_sock = *(int*)arg;
}

void resfore_files(){
	int idx=0;
	FILE*fp = fopen("users.txt","r");
	if(!fp){
		fprintf(stderr,"user file open error\n");
		exit(1);
	}
	while(!feof(fp)){
		USER user;
		fscanf(fp,"%s %s\n",user.name,user.password);
		users[idx++]=user;
	}
	fclose(fp);
	idx=0;
	fp = fopen("items.txt","r");
	if(!fp){
		fprintf(stderr,"item file open error\n");
		exit(1);
	}
	while(!feof(fp)){
		ITEM item;
		fscanf(fp,"%s %d\n",item.name,&(item.key));
		items[item_cnt++]=item;
	}
	fclose(fp);
}

void error_handling(char *msg)
{
    puts(msg);
    exit(1);
}