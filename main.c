#include "httpd.h"


int main(int argc, char *argv[])
{
	if(argc != 2)
	{
		write_log("usage error!");
		usage(argv[0]);
	}
	//初始化套接字,以及互斥量
	int lst_fd = sock_init(atoi(argv[1])); 
	if(lst_fd < 0)
	{
		return -lst_fd;
	}
	else
	{
		printf("server is running\n");
		write_log("init OK!");
	}
	//开始接客
	while(1)
	{

		socklen_t add_len = sizeof(cli_addr);
		int cli_fd = accept(lst_fd, (struct sockaddr*)&cli_addr, &add_len);
		if(cli_fd < 0)
		{
			write_log("accept error");
		}
		else
		{
			char mess_buff[1024];
			sprintf(mess_buff, "[sockfd:%4d]\tclient ip:%s\tport:%d\tconnected", cli_fd, inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port));
			write_log(mess_buff);
		}
		//创建一个线程去处理客户算发来的请求
		pthread_t tid;
		int *p = malloc(sizeof(int));
		*p = cli_fd;
		pthread_create(&tid, NULL, request_handler, (void*)p);
		pthread_detach(tid);
	}
	pthread_mutex_destroy(&mutex);

	return 0;
}
