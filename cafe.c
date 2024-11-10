#include <stdio.h>
#include <stdlib.h>
#include "cafe.h"

ITEM items[MAX_ITEM];

//혹시 모르니까 최초실행 시 0으로 초기화
int total_item_cnt = 0;
int coffee_cnt = 0;
int tea_cnt = 0;
int juice_cnt = 0;
int brunch_cnt = 0;

void reset_cnt();

void restore_menu()
{
	FILE *coffee_fp = fopen("item/coffee.txt", "r");
	FILE *tea_fp = fopen("item/tea.txt", "r");
	FILE *juice_fp = fopen("item/juice.txt", "r");
	FILE *brunch_fp = fopen("item/brunch.txt", "r");

	//호출 때 마다 reset하지 않으면 cnt가 계속 커져서 막 두번 씩 출력되더라고요. 그래서 호출할 떄마다 cnt들을 초기화 합니다.
	reset_cnt();

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

int find_item_idx_by_category_and_key(int item_category, int item_key)
{
	for (int i = 0; i < total_item_cnt; i++)
		if (items[i].category == item_category && items[i].key == item_key)
			return i;
	error_handling("could not find menu");
}

void error_handling(char *msg)
{
	puts(msg);
	exit(1);
}

void reset_cnt(){
	total_item_cnt = 0;
	coffee_cnt = 0;
	tea_cnt = 0;
	juice_cnt = 0;
	brunch_cnt = 0;
}