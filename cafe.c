#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
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

void initialize_item_info(RECENT_MENU initial_packet)
{
	memcpy(items, initial_packet.items, sizeof(ITEM) * MAX_ITEM);
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
void backup_warning(int signum)
{
	printf("\n[WARNING] SIGINT signal: Backup signal at \033[31mserver\033[0m shutdown.\n");
	printf("\t  If you want to terminate this file, use the QUIT option.\n");
}

void clear_terminal()
{
	printf("\033[3J"); // 스크롤 백 버퍼 지우기
	printf("\033[2J"); // 화면 지우기
	printf("\033[H");  // 커서를 좌상단으로 이동
}

void return_main()
{
	printf("\nPRESS Enter to Main");
	getchar();
	getchar();
}

int get_visible_length(const char *str)
{
	int len = 0;
	int inside_escape = 0;

	for (const char *p = str; *p != '\0'; ++p)
	{
		if (*p == '\033') // ANSI escape 코드 시작
		{
			inside_escape = 1;
		}
		else if (inside_escape && *p == 'm') // ANSI escape 코드 종료
		{
			inside_escape = 0;
		}
		else if (!inside_escape)
		{
			len++; // 화면에 표시되는 문자만 길이에 포함
		}
	}

	return len;
}

void print_centered(const char *str)
{
	struct winsize w;
	int terminal_width, str_len, padding;

	// 터미널 크기 가져오기
	if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == -1)
	{
		perror("ioctl error");
		return;
	}
	terminal_width = w.ws_col;		   // 터미널 너비
	str_len = get_visible_length(str); // 표시될 문자열의 실제 길이

	// 좌우 여백 계산
	if (str_len >= terminal_width)
	{
		padding = 0; // 문자열이 터미널 너비를 초과할 경우, 여백 없음
	}
	else
	{
		padding = (terminal_width - str_len) / 2; // 중앙 정렬을 위한 여백
	}

	// 좌측 여백을 공백으로 출력 후 문자열 출력
	for (int i = 0; i < padding; i++)
	{
		putchar(' ');
	}
	printf("%s", str);
}