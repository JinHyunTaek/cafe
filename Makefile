# Compiler 및 플래그 설정
CC = gcc
LIBS = -lpthread

# 실행 파일
TARGETS = cadmin cclient cserver

# 소스 파일들
SOURCES_COMMON = cafe.c cafe.h
SOURCES_ADMIN = cafe_admin.c $(SOURCES_COMMON)
SOURCES_CLIENT = cafe_client.c $(SOURCES_COMMON)
SOURCES_SERVER = cafe_server.c $(SOURCES_COMMON)

# 기본 타겟
all: $(TARGETS)

# cadmin 빌드
cadmin: $(SOURCES_ADMIN)
	$(CC) $(CFLAGS) -o $@ cafe.c cafe_admin.c

# cclient 빌드
cclient: $(SOURCES_CLIENT)
	$(CC) $(CFLAGS) -o $@ cafe.c cafe_client.c

# cserver 빌드 (pthread 링크 추가)
cserver: $(SOURCES_SERVER)
	$(CC) $(CFLAGS) -o $@ cafe.c cafe_server.c $(LIBS)

# clean 명령어로 모든 실행 파일 및 임시 파일 제거
clean:
	rm -f $(TARGETS) *.o