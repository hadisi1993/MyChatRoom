//我决定用poll实现聊天室这个程序
#include"utils.h"
#include<string.h>
#include<stdlib.h>
#include<sys/socket.h>
#include<poll.h>
#include<errno.h>
#define FD_LIMIT 50
#define USER_LIMIT 15
const char*info1 = "The chatroom is full";
const char*info2 = "server internal error";
const char*info3 = "wrong username and password";
const char*info4 = "login success";
int user_counter;     // 用户数量
int main(int argc,char*argv[]){

	if(argc!=2){
		printf("Usage <Port>\n");
		return -1;
	}
	int serv_fd,clnt_fd,listen_fd;
	struct sockaddr_in serv_addr,clnt_addr;
	socklen_t adr_sz;
	int str_len;
	char buf[BUF_SIZE];

	struct account acc_info;
	char acc[100]; //接受账户信息
	// 参考书上的代码，要注意数组的大小
	struct pollfd fds[USER_LIMIT+1];
	struct client_data*users= (struct client_data*)malloc(sizeof(struct client_data)*FD_LIMIT);
	serv_fd = socket(PF_INET,SOCK_STREAM,0);
	if(serv_fd==-1){
		error_handling("socket error()!");
	}
	memset(&serv_addr,0,sizeof(struct sockaddr_in));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_addr.sin_port = htons(atoi(argv[1]));


	if(bind(serv_fd,(struct sockaddr*)&serv_addr,sizeof(serv_addr))==-1){
		error_handling("bind() error");
	}

	if(listen(serv_fd,5)==-1){
		error_handling("listen() error!");
	}

	// 初始化fds数组
	for(int i=0;i<=USER_LIMIT;i++){
		fds[i].fd = -1;
		fds[i].events = 0;
	}
	fds[0].fd = serv_fd;
	fds[0].events = POLLIN|POLLERR;
	fds[0].revents = 0;
	int stop = 0;
	while(!stop){
		printf("waiting for  new events\n");
		printf("users_counter:%d\n",user_counter);
		int ret = poll(fds,user_counter+1,-1);
		printf("new events coming!\n");
		if(ret<0){
			error_handling("poll() error!");
		}
		for(int i=0;i<user_counter+1;i++){
			printf("i:%d\n",i);
			if((fds[i].fd==serv_fd)&&(fds[i].revents==POLLIN)){
				adr_sz = sizeof(clnt_addr);
				clnt_fd = accept(serv_fd,(struct sockaddr*)&clnt_addr,&adr_sz);
				printf("clnt_fd:%d\n",clnt_fd);
				if(clnt_fd<0){
					printf("error is: %d\n",errno);
					continue;
				}
				//如果连接过多则关闭连接
				if(user_counter>=USER_LIMIT){
					printf("%s",info1);
					send(clnt_fd,info1,strlen(info1),0);
					close(clnt_fd);
					continue;
				}
				//验证登陆
				memset(acc,'\0',BUF_SIZE);
				str_len = recv(clnt_fd,acc,sizeof(acc),0);
				printf("str_len:%d\n",str_len);
				if(str_len<0){
					//如果读取错误就断开连
					printf("connection:recv() error!\n");
					close(clnt_fd);
					continue;
				}else if(str_len>0){
				        // 判断登陆 
					memcpy(&acc_info,acc,sizeof(acc));
					int flag = checkLogin(acc_info);
					if(flag==-1){    // 登陆失败
						send(clnt_fd,info2,strlen(info2),0);
						close(clnt_fd);
					}else if(flag==1){
						send(clnt_fd,info3,strlen(info3),0);
						close(clnt_fd);
					}
					else{		// 登陆成功	
						send(clnt_fd,info4,strlen(info4),0);	
						user_counter++;
						users[clnt_fd].addr = clnt_addr;
						setnonblocking(clnt_fd);
						fds[user_counter].fd = clnt_fd;
						fds[user_counter].events = POLLIN|POLLRDHUP|POLLERR;
						fds[user_counter].revents = 0;
						printf("comes a new user,now have %d users\n",user_counter);
					}
				}
			}
			else if(fds[i].revents&POLLRDHUP){
				// 断开连接
				clnt_fd = fds[i].fd;
				users[clnt_fd] = users[fds[user_counter].fd];
				fds[i] = fds[user_counter];
				user_counter--;
				i--;
				close(clnt_fd);
				printf("A user: %d  leave!\n",clnt_fd);
				printf("user_count:%d\n",user_counter);

			}
			else if(fds[i].revents&POLLIN){
				// 准备转发其他客户端发送过来的数据
				clnt_fd = fds[i].fd;
				memset(users[clnt_fd].buf,'\0',BUF_SIZE);
				str_len = recv(clnt_fd,users[clnt_fd].buf,BUF_SIZE-1,0);
				printf("This is the recv message: %s\n",users[clnt_fd].buf);
				printf("get %d bytes of client data from %d\n",str_len,clnt_fd);
				if(str_len<0){
					//如果读取错误就断开连接 
					if(errno!=EAGAIN){
						close(clnt_fd);
						users[fds[i].fd] = users[fds[user_counter].fd];
						fds[i] = fds[user_counter];
						user_counter--;
						i--;
					}
					printf("recv() error!\n");
				}
				else if(str_len>0){
					for(int j=1;j<=user_counter;j++){	
						if(j!=i){
							fds[j].events|=~POLLIN;
							fds[j].events|=POLLOUT;
							users[fds[j].fd].write_buf = users[clnt_fd].buf;

						}
					}
				}		
			}
			else if(fds[i].revents&POLLERR){
				printf("get an error from %d\n",fds[i].fd);
				char errors[100];
				memset(errors,'\0',100);
				socklen_t length = sizeof(errors);
				if(getsockopt(fds[i].fd,SOL_SOCKET,SO_ERROR,&errors,&length)<0){
					printf("get a socket option failed\n");
				}
				continue;
			}
			else if(fds[i].revents&POLLOUT){
				clnt_fd = fds[i].fd;
				if(!users[clnt_fd].write_buf){
					continue;
				}
				send(clnt_fd,users[clnt_fd].write_buf,strlen(users[clnt_fd].write_buf)-1,0);
				users[clnt_fd].write_buf = NULL;
				fds[i].revents|=~POLLOUT;
				fds[i].revents|=POLLIN;
			}

		}
	}
	close(serv_fd);
	free(users);
	return 0;
}
