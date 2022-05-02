#include"utils.h"
int setnonblocking(int fd){
	int old_option = fcntl(fd,F_GETFL);
	int new_option = old_option|O_NONBLOCK;
	fcntl(fd,F_SETFL,new_option);
	return old_option;

}
void error_handling(char*message){
	fputs(message,stderr);
	fputc('\n',stderr);
	exit(1);
}

void addfd(int epollfd,int fd){
	struct epoll_event event;
	event.data.fd = fd;
	event.events = EPOLLIN;
	epoll_ctl(epollfd,EPOLL_CTL_ADD,fd,&event);
	return;
}

int checkLogin(struct account acc_info){
	
	// connect Mysql
	MYSQL conn;

	if(NULL == mysql_init(&conn)){
		printf("mysql_init() error:\n");
		return -1;	
	}

	if(NULL==mysql_real_connect(&conn,"localhost","root","","users",3306,NULL,0)){
		printf("mysql_real_connect() error:\n");
		return -1;
	}

	MYSQL_RES *res = NULL;
	MYSQL_ROW row;

	char query[100] = "select * from user where username = \'";
	strcat(query,acc_info.username);
	int len = strlen(query);
	query[len] = '\'';
	query[len+1] = ';';
	query[len+2] = '\0';

	int  rc = mysql_real_query(&conn,query,strlen(query));

	
	if(rc!=0){
		printf("mysql_real_query() error:\n");
		return -1;
	}
	res = mysql_store_result(&conn);
	if(res==NULL){
		printf("mysql_store_query() error:\n");
		return -1;
	}

	int rows = mysql_num_rows(res);

	if(rows==0){
		return 1;
	}
	while((row=mysql_fetch_row(res))!=NULL){
		printf("password:%s\n",row[2]);
		if(strcmp(row[2],acc_info.password)!=0){
			return 1;
		}
	}
	printf("login success!\n");
	return 0;

}
