#include <stdio.h>
#include <stdlib.h>
#include "cafe.h"

ITEM items[MAX_ITEM];
int total_item_cnt;
int coffee_cnt;
int tea_cnt;
int juice_cnt;
int brunch_cnt;

void restore_menu()
{
	FILE *coffee_fp = fopen("item/coffee.txt", "r");
	FILE *tea_fp = fopen("item/tea.txt", "r");
	FILE *juice_fp = fopen("item/juice.txt", "r");
	FILE *brunch_fp = fopen("item/brunch.txt", "r");
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