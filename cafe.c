#include <stdio.h>
#include <stdlib.h>
#include "cafe.h"

ITEM items[MAX_ITEM];
int total_item_cnt;
int coffee_cnt;
int tea_cnt;
int juice_cnt;
int brunch_cnt;

void restore_menu(){
	FILE*coffee_fp = fopen("item/coffee.txt","r");
	FILE*tea_fp = fopen("item/tea.txt","r");
	FILE*juice_fp = fopen("item/juice.txt","r");
	FILE*brunch_fp = fopen("item/brunch.txt","r");
	if(!coffee_fp || !tea_fp || !juice_fp || !brunch_fp){
		fprintf(stderr,"menu file open error\n");
		exit(1);
	}
	while(!feof(coffee_fp)){
		fscanf(coffee_fp,"%s %d\n",items[total_item_cnt].name,&items[total_item_cnt].key);
		items[total_item_cnt].category = COFFEE;
		coffee_cnt++;
		total_item_cnt++;
	}
	while(!feof(tea_fp)){
		fscanf(tea_fp,"%s %d\n",items[total_item_cnt].name,&items[total_item_cnt].key);
		items[total_item_cnt].category = TEA;
		tea_cnt++;
		total_item_cnt++;
	}
	while(!feof(juice_fp)){
		fscanf(juice_fp,"%s %d\n",items[total_item_cnt].name,&items[total_item_cnt].key);
		items[total_item_cnt].category = JUICE;
		juice_cnt++;
		total_item_cnt++;
	}
	while(!feof(brunch_fp)){
		fscanf(brunch_fp,"%s %d\n",items[total_item_cnt].name,&items[total_item_cnt].key);
		items[total_item_cnt].category = BRUNCH;
		brunch_cnt++;
		total_item_cnt++;
	}
	fclose(coffee_fp);fclose(tea_fp);fclose(juice_fp);fclose(brunch_fp);
}