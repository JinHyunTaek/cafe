#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cafe.h"

ITEM items[MAX_ITEM];
int total_item_cnt = 0;
int coffee_cnt = 0;
int tea_cnt = 0;
int juice_cnt = 0;
int brunch_cnt = 0;

void reset_cnt();

void error_handling(char *msg)
{
	puts(msg);
	exit(1);
}

void initialize_item_info(RECENT_MENU initial_packet){
	memcpy(items,initial_packet.items,sizeof(ITEM)*MAX_ITEM);
	total_item_cnt = initial_packet.cnts[0];
	coffee_cnt = initial_packet.cnts[COFFEE];
	tea_cnt = initial_packet.cnts[TEA];
	juice_cnt = initial_packet.cnts[JUICE];
	brunch_cnt = initial_packet.cnts[BRUNCH];
}

int find_item_idx_by_category_and_key(int item_category, int item_key)
{
	for (int i = 0; i < total_item_cnt; i++)
		if (items[i].category == item_category && items[i].key == item_key)
			return i;
	error_handling("could not find menu");
}

// signal handler: server 외에서 SIGINT 발생 시 경고 문구
void backup_warning(int signum) {
	printf("\n[WARNING] SIGINT signal: Backup signal at server shutdown.\n");
	printf("\t  No backup will be performed in this file.\n");
}
