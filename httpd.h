#ifndef __HTTPD_H__
#define __HTTPD_H__
#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<string.h>
#include<pthread.h>
#include<sys/socket.h>
#include<fcntl.h>
#include<strings.h>
#include<ctype.h>
#include<sys/sendfile.h>
#include<sys/stat.h>
#include<sys/wait.h>
#include<signal.h>

#define MAX 1024
#define HOME_PAGE "index.html"

struct sockaddr_in self_addr;
struct sockaddr_in cli_addr;

enum pipe_sign
{
	READ,
	WRITE
};

pthread_mutex_t mutex;

//读取一行
void read_line(int sock, char *buff, int size);


//结束函数
void end(int sock, int statu_code);

//执行cgi程序
int exe_cgi(int sock, char* method, char *path, char *query_string);

//响应非cgi资源
int get_response(int sock, char *path, int length, char *type);

//初始化互斥量


//记录日志
void write_log(char *message);

//初始化监听套接字,以及互斥量
void usage(char *arg);

//初始化
int sock_init(int port);

//线程入口
void* request_handler(void *arg);

#endif //__HTTPD_H__
