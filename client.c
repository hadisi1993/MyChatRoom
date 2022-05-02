#include"utils.h"
#include<string.h>
#include<unistd.h>
#include<netinet/in.h>
#include<stdlib.h>
#include<sys/socket.h>
#include<stdlib.h>
#include<sys/types.h>
#include<pthread.h>
int main(int argc,char*argv[]){


	if(argc!=3){
		printf("Usage <IP> <Port>\n");
		return -1;
	}	
	int listen_fd;
	char recv_buf[BUF_SIZE],send_buf[BUF_SIZE];
	char acc_buf[100];  // 传输结构体的字符串大小要与结构体相等
	struct sockaddr_in serv_addr;


	// 向服务器传递账号和密码
	struct account acc_info;
	
	int stop = 0,ret;
	while(!stop){
		listen_fd = socket(PF_INET,SOCK_STREAM,0);
		if(listen_fd==-1){
			error_handling("socket() error!");
		}
		memset(&serv_addr,0,sizeof(serv_addr));
		serv_addr.sin_family = AF_INET;
		serv_addr.sin_addr.s_addr = inet_addr(argv[1]);
		serv_addr.sin_port = htons(atoi(argv[2]));
		//等待10秒
		if(connect(listen_fd,(struct sockaddr*)&serv_addr,sizeof(serv_addr))==-1){
			error_handling("connect() error!");
		}
		printf("USERNAME:");
		scanf("%s",acc_info.username);
		getchar();
		printf("PASSWORD:");
		scanf("%s",acc_info.password);
		printf("Recevied account information:\n");
		printf("username:%s\n",acc_info.username);
		printf("password:%s\n",acc_info.password);
        	memset(acc_buf,'\0',sizeof(acc_buf));
		memcpy(acc_buf,&acc_info,sizeof(acc_info));
		printf("%d\n",sizeof(send_buf));
		send(listen_fd,acc_buf,sizeof(acc_buf),0);
		printf("infomation already send!\n");
		memset(recv_buf,'\0',sizeof(recv_buf));
		ret= recv(listen_fd,recv_buf,BUF_SIZE,0);
		if(ret<0){
			error_handling("recv() error");
		}else{
			if(strcmp(recv_buf,"login success")==0){
				printf("%s\n",recv_buf);
				stop = 1;
			}else{
				close(listen_fd);
				printf("login faild!Check your username and password\n");
			}
		}
		sleep(10);
	}
		
	// 使用epoll监听用户输入和服务器传回来的消息
	// 说实话，这里用到多路复用有点大才小用了，毕竟只有两个事件
	int epoll_fd = epoll_create(5);
	if(epoll_fd==-1){
		error_handling("epoll_create() error!");
	}
	struct epoll_event events[2];
	
	//将标准输入的文件描述符(0)注册
	addfd(epoll_fd,0);
	//将客户端连接的socket注册
	struct epoll_event event;
	event.data.fd = listen_fd;
	event.events = EPOLLIN|EPOLLRDHUP;
	epoll_ctl(epoll_fd,EPOLL_CTL_ADD,listen_fd,&event);

	
	int pipefds[2];
	//使用管道传递消息
	if(pipe(pipefds)==-1){
		error_handling("pipe() error!");
	}
	stop = 0;
	while(!stop){
		int epoll_cnt = epoll_wait(epoll_fd,events,2,-1);
		if(epoll_cnt==-1){
			error_handling("epoll_wait() error!");
		}
		for(int i=0;i<epoll_cnt;i++){
			//要读取数据
			int sock_fd = events[i].data.fd; 
			if(listen_fd==sock_fd){
				if(events[i].events&EPOLLRDHUP){
				    // 服务器断开连接
				    stop = 1;
				    epoll_ctl(epoll_fd,EPOLL_CTL_DEL,listen_fd,NULL);
				    printf("Connect missed!\n");
				}
				else if(events[i].events&EPOLLIN){		
				    // 读取服务器传递过来的信息
				    memset(recv_buf,'\0',BUF_SIZE);
				    recv(sock_fd,recv_buf,BUF_SIZE-1,0);
				    printf("%s\n",recv_buf);
				}
			}
			else{
				if(events[i].events&EPOLLIN){
					//使用零拷贝和管道技术将输传sock当中去
					ret = splice(0,NULL,pipefds[1],NULL,65535,SPLICE_F_MORE|SPLICE_F_MOVE);
					ret = splice(pipefds[0],NULL,listen_fd,NULL,65535,SPLICE_F_MORE|SPLICE_F_MOVE);		
				}
			}
			

		}


	}
	close(listen_fd);
	return 0;
}
