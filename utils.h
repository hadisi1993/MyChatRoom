#define _GNU_SOURCE 1
#ifndef _UTILS_H
#define _UTILS_H
#include<arpa/inet.h>
#include<mysql/mysql.h>
#include<fcntl.h>
#include<sys/epoll.h>
#include<stdio.h>
#define BUF_SIZE 65536
struct client_data{

	struct sockaddr_in addr;
	char buf[BUF_SIZE];
	char*write_buf;    // 这里将读缓冲区和写缓冲区区分开来，便于服务器端的处理
};
struct account{
	char username[50];
	char password[50];

};


void error_handling(char*message);
void addfd(int epollfd,int fd);
int setnonblocking(int fd);
int checkLogin(struct account acc_info);
#endif
