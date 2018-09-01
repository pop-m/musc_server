#include"httpd.h"
#include<ctype.h>

/*将十六进制数转为十进制*/
int hex_to_decade(char * s)
{
    char *digits="0123456789ABCDEF";
 
    /*判断大小写，小写的话转为大写，达到统一*/
    if (islower (s[0]))
        s[0]=toupper(s[0]);
    if (islower (s[1]))
        s[1]=toupper(s[1]);
 
    return 16*(strchr(digits,s[0])-strchr(digits,'0'))+(strchr(digits,s[1])-strchr(digits,'0'));
}
void url_to_gb_or_utf(char *get_url, char *return_gb_or_utf)
{
    int url_position;/*用来保存get_url的位置*/
    int return_position;/*用来保存解码后的字符串的位置*/
    int url_len;/*用来保存get_url的长度*/
    char tmp[2];/*保存%后面的十六进制字符*/
    url_len = strlen(get_url);
    return_position = 0;
 

    for ( url_position = 0; url_position < url_len; )
    {
        /*如果是%将它后面的十六进制字符考到数组里*/
        if ( get_url[url_position] == '%' ){
            tmp[0] = get_url[url_position+1];/*第一个十六进制字符*/
            tmp[1] = get_url[url_position+2];/*第二个*/
        //  tmp[2] = '\0';  **串口通信中会出现乱码，结尾必须不能有其他字符**
 
            url_position+= 3; /*使url_position跳到的下一个%*/
            /*将十六进制数转为十进制后考入要返回的数组里*/
 
            return_gb_or_utf[return_position] = hex_to_decade(tmp);
        }
        /*如果不是特殊字符，如英文，数字那么直接返回*/
        else{
            return_gb_or_utf[return_position] = get_url[url_position];
            url_position++;
        }
        return_position++;
    }
 
    return_gb_or_utf[return_position] = 0;
}

//读取一行
int read_line(int sock, char *buff, int size)
{
	char tmp = 'm';
	int i = 0;
	while(tmp != '\n' && i < size)
	{
		ssize_t s = recv(sock, &tmp, 1, 0);
		if(s > 0)//读取正常
		{
			if(tmp == '\r')//如果读取到的是一个\r说明需要窥探一下下一个字符是不是\n
			{
				int ret = recv(sock, &tmp, 1, MSG_PEEK);
				if(ret < 0)//读取失败
				{
					break;
				}
				else//读成功
				{
					if(tmp == '\n')//如果下一个字符是\n,那就需要将此字符读取,并放入buff
					{
						recv(sock, &tmp, 1, 0);
					}
					else//下一个字母不是\n
					{
						tmp = '\n';
					}
				}
			}
			//此时已经将\r  \r\n  \n都准换为\n了
			buff[i++] = tmp;
		}
		else//读取出错
		{
			return -1;
			break;
		}
	}
	//将\0添加上去
	buff[i] = 0;
	return 0;
}
//清除头部
void clear_head(int sock)
{
	char line[MAX] = {0};
	while(strcmp(line, "\n"))
	{
		if(read_line(sock, line, MAX-1) == -1)
		{
			end(sock, 400);
		}
	}
}

//响应错误
void echo_error(int sock, int statu_code, char descri[], char page_path[])
{
	char buff[MAX] = {0};
	sprintf(buff, "HTTP/1.0 %d %s\r\n", statu_code, descri);
	send(sock, buff, strlen(buff), 0);
	struct stat st;
	stat(page_path, &st);
	sprintf(buff, "Content-Length: %d\r\n", st.st_size);
	send(sock, buff, strlen(buff), 0);
	send(sock,"Content-Type: text/html\r\n", strlen("Content-Type: test/html\r\n"), 0);
	send(sock, "\r\n", 2, 0);
	int fd =open(page_path, O_RDONLY);
	sendfile(sock, fd, NULL, st.st_size);
	close(fd);
}


//结束函数
void end(int sock, int statu_code)
{
	switch(statu_code)
	{
	case 400:
		echo_error(sock, 400, "Bad Request", "wwwroot/error_page/400.html");
		break;
	case 404:
		echo_error(sock, 404, "Not Found", "wwwroot/error_page/404.html");
		break;
	case 500:
		echo_error(sock, 500, "Internal Server Error", "wwwroot/error_page/500.html");
		break;
	default:
		break;
	}
	close(sock);
	printf("线程已经退出\n");
	pthread_exit(NULL);
}



//响应非cgi资源
int get_response(int sock, char *path, int length, char *type)
{
	//清理请求报头
	clear_head(sock);

	int fd = open(path, O_RDONLY);
	if(fd < 0)
	{
		return 404;
	}
	//构造响应报头
	char line[MAX];
	sprintf(line, "HTTP/1.0 200 OK\r\n");
	send(sock, line, strlen(line), 0);
	//构造响应正文长度和类型
	sprintf(line, "Content-Type: %s\r\nContent-Length: %d\r\n", type, length);
	send(sock, line, strlen(line), 0);
	send(sock, "\r\n", 2, 0);//空行
	sendfile(sock, fd, NULL, length);
	char log_buff[1024] = {0};
	sprintf(log_buff, "[sockfd:%4d]\tresponse:%s\tGET\tok", sock, path);
	write_log(log_buff);
	close(fd);
	return 200;
}

//根据文件路径得出响应报文的content-type字段
void get_filetype(char *path, char *type)
{
	char *file_end;
	int i = strlen(path)-1;
	while(i >= 0)
	{
		if(path[i] == '.')
		{
			file_end = path + i;
			//后缀已经拿到
			if(strcmp(file_end, ".html") == 0)
			{
				strcpy(type, "text/html");
				return;
			}
			else if(strcmp(file_end, ".css") == 0)
			{
				strcpy(type, "text/css");
				return;
			}
			else if(strcmp(file_end, ".js") == 0)
			{
				strcpy(type, "application/x-javascript");
				return;
			}
			else if(strcmp(file_end, ".jpg") == 0)
			{
				strcpy(type, "image/jpeg");
				return;
			}
			else if(strcmp(file_end, ".png") == 0)
			{
				strcpy(type, "image/png");
				return;
			}
		}
		i--;
	}
	strcpy(type, "");
}

//执行cgi程序
int exe_cgi(int sock, char* method, char *path, char *query_string)
{
	char line[MAX] = {0};
	int content_length = -1;
	if(strcasecmp(method, "GET") == 0)
	{
		clear_head(sock);
	}
	else
	{
		//method为POST
		//读取Content-Length
		do
		{
			if(read_line(sock, line, MAX-1) == -1)
			{
				int statu_code = 400;
				end(sock, statu_code);
			}
			if(strncmp(line, "Content-Length: ", 16) == 0)
			{
				content_length = atoi(line + 16);
			}//取到Content_Length之后不能直接进行break,因为一直读取,要把报头读完,这样也就不用调用clear_head函数了
		}while(strcmp(line, "\n"));
		if(content_length == -1)
		{
			return 400;
		}
	}
	//开始执行cgi程序
	int parent_to_child[2];//父进程发送给子进程数据所用的管道
	int child_to_parent[2];//子进程发送数据给父进程所用的管道
	int pipe_ret = pipe(parent_to_child);
	if(pipe_ret < 0)
	{
		return 500;
	}
	pipe_ret = pipe(child_to_parent);
	if(pipe_ret < 0)
	{
		return 500;
	}
	//创建子进程
	int pid = fork();
	if(pid < 0)
	{
		//服务器错误
		return 500;
	}
	if(pid == 0)
	{
		close(sock);
		//子进程
		close(parent_to_child[WRITE]);
		close(child_to_parent[READ]);
		//重定向cgi程序的标准输入和标准输出到管道中
		dup2(parent_to_child[READ],0);
		dup2(child_to_parent[WRITE],1);
		
		char method_env[MAX/16] = {0};
		sprintf(method_env,"METHOD=%s", method);
		//将方法当作环境变量存储,因为环境变量在程序替换之后不会改变,并且最容易取出来
		putenv(method_env);
		if(strcasecmp(method, "GET") == 0)
		{
			char query_string_env[MAX] = {0};
			sprintf(query_string_env, "QUERY_STRING=%s", query_string);
			putenv(query_string_env);//如果方法是GET,那就把query_string也设置为环境变量
			//再此就直接写入get ok,因为无法在下面确定是get请求的cgi程序还是POST请求的cgi程序
			char log_buff[1024] = {0};
			sprintf(log_buff, "[sockfd:%4d]\tresponse:%s\tGET\tok", sock, path);
			write_log(log_buff);
		}
		else if(strcasecmp(method, "POST") == 0)
		{
			char content_length_env[MAX] = {0};
			sprintf(content_length_env,"CONTENT_LENGTH=%d", content_length);
			putenv(content_length_env);//如果方法为POST,那就把刚才读到的content_length设置为环境变量
		}
	
		execl(path, path, NULL);
		close(parent_to_child[READ]);
		close(child_to_parent[WRITE]);
		return 500;//走到这里的时候说明execl出错
	}
	else
	{
		close(parent_to_child[READ]);
		close(child_to_parent[WRITE]);
		if(strcasecmp(method, "POST") == 0)//如果方法是POST,那就应该将正文通过管道发送给子进程
		{
			char buff[MAX] = {0};
			int i = 0;
			while(i < content_length)
			{
				char c = 0;
				if(recv(sock, &c, 1, 0) <= 0)
				{
					return 400;//如果正文的的大小和头部的content_length不相同,那就直接报客户端错误
				}
				else
				{
					buff[i] = c;
				}
				i++;
			}
			char log_buff[1024] = {0};
			sprintf(log_buff, "[sockfd:%4d]\trequest:%s\tpost_string:%s\tPOST", sock, path, buff);
			write_log(log_buff);
			write(parent_to_child[WRITE], buff, content_length);//将正文发送管道
			//再此就直接写入get ok,因为无法在下面确定是get请求的cgi程序还是POST请求的cgi程序
			sprintf(log_buff, "[sockfd:%4d]\tresponse:%s\tpost_string:%s\tPOST\tOK", sock, path,buff);
			write_log(log_buff);
		}

		
		//发送完就接收
		char respon_mess[MAX*2] = {0};//用来存储响应正文
		int i = 0;
		read(child_to_parent[READ], respon_mess+i, 1);
		while(respon_mess[i] != 0)
		{
			i++;
			read(child_to_parent[READ], respon_mess+i, 1);
		}
		//构造响应
		send(sock, "HTTP/1.0 200 OK\r\n", strlen("HTTP/1.0 200 OK\r\n"), 0);//响应行
		char respon_content_length[MAX] = {0};
		sprintf(respon_content_length, "Content-Length: %d\r\n", strlen(respon_mess));

		send(sock, respon_content_length, strlen(respon_content_length), 0);//响应content-length字段
		send(sock, "Content-Type: text/html\r\n", strlen("Content-Type: text/html\r\n"), 0);//响应content-type字段
		send(sock, "\r\n", 2, 0);//响应的空行
		send(sock, respon_mess, strlen(respon_mess), 0);//发送响应正文
		//父进程
		waitpid(pid, NULL, 0);
		
		close(parent_to_child[WRITE]);
		close(child_to_parent[READ]);
		return 200;
	}

}

//线程入口
void* request_handler(void *arg)
{
	int sock = *(int*)arg;
	free(arg);
	//接收数据
	//首先接收请求行
	char line[MAX] = {0};
	char method[MAX/10] = {0};
	int statu_code = 200;
	printf("开始读取数据\n");
	if(read_line(sock, line, MAX-1) == -1)
	{
		statu_code = 400;
		end(sock, statu_code);
	}
	printf("数据已经读取\n");
	int i=0;
	int j=0;
	char path[MAX] = {0};
	char query_string[MAX] = {0};
	int line_len = strlen(line);
	int cgi = 0;
	char url[MAX] = {0};
	//获得请求方法
	while(i < sizeof(method) && j < line_len && !isspace(line[j]))
	{
		method[i++] = line[j++];
	}
	method[i] = '\0';
	i = 0;
	j++;
	//读取URL(包含URI和参数)
	printf("开始拿url和query_string\n");
	while(i < sizeof(url) && j < line_len && !isspace(line[j]))
	{
		url[i++] = line[j++];
	}
	//
	url[i] = 0;
	i = 0;
	int url_len = strlen(url);
	while(i <= url_len)
	{
		if(url[i] == '?' || url[i] == 0)
		{
			if(url[i] == '?')//说明有参数
			{
				sprintf(query_string, "%s", url+i+1);//提取query_string
			}
			url[i] = 0;
			sprintf(path, "wwwroot%s", url);//提取并拼接path
			break;
		}
		i++;
	}
	printf("已经拿到url和query_string\n");
	char string_utf[MAX] = {0};
	url_to_gb_or_utf(query_string, string_utf);

	if(strcasecmp(method, "GET") == 0)
	{
		//请求方法为GET
		char log_buff[1024] = {0};
		sprintf(log_buff, "[sockfd:%4d]\trequest:%s\tquerty_string:%s\tGET", sock, path, string_utf);
		write_log(log_buff);
		//根据请求的资源构造响应
		struct stat st;
		if(stat(path, &st) < 0)
		{
			//请求资源不存在
			clear_head(sock);
			statu_code = 404;
			char log_buff[MAX] = {0};
			sprintf(log_buff, "[sockfd:%4d]\tresponse: not foundt\t%s\tERROR\tGET", sock, path);
			write_log(log_buff);
			printf("资源不存在\n");
			end(sock, statu_code);
		}
		else
		{
			//请求的资源没问题
			if(S_ISDIR(st.st_mode))
			{
				//请求的是一个目录,就定向到默认的主页
				strcat(path, HOME_PAGE);
				if(stat(path, &st) < 0)
				{
					//请求资源有问题
					clear_head(sock);
					statu_code = 404;
					char log_buff[MAX] = {0};
					sprintf(log_buff, "[sockfd:%4d]\tresponse: not found\t%s\tERROR\tGET", sock, path);
					write_log(log_buff);
					printf("资源不存在\n");
					end(sock, statu_code);
				}
			}
			//请求的资源没有任何问题,并且将请求的目录转变为了文件
			//已将path和query_string提取以及拼接

			
			if((st.st_mode & S_IXUSR) || (st.st_mode & S_IXGRP) || (st.st_mode & S_IXOTH))
			{
				printf("具有可执行权限\n");
				//所访问的资源有可执行权限
				statu_code = exe_cgi(sock, method, path, query_string);
				if(statu_code != 200)
				{
					end(sock, statu_code);
				}
				else
				{
					close(sock);
					printf("线程已经退出\n");
					pthread_exit(NULL);
				}
				
			}
			else
			{
				//不是可执行文件(只需要将文件数据发送给客户端即可)
				printf("文本文件\n");
				char log_buff[1024] = {0};
				sprintf(log_buff, "[sockfd:%4d]\trequest:%s\tquerty_string:%s\tGET", sock, path, string_utf);
				write_log(log_buff);

				char type[MAX] = {0};
				get_filetype(path, type);
				statu_code = get_response(sock, path, st.st_size, type);
				if(statu_code != 200)
				{
					end(sock, statu_code);
				}
				else
				{
					char log_buff[1024] = {0};
					sprintf(log_buff, "[sockfd:%4d]\tresponse:%s\tquerty_string:%s\tGET\tOK", sock, path, string_utf);
					write_log(log_buff);
					close(sock);
					printf("线程已经退出\n");
					pthread_exit(NULL);
				}
			}
		}
	}
	else if(strcasecmp(method, "POST") == 0)
	{
		//请求方法为POST
		statu_code = exe_cgi(sock, method, path, query_string);
		if(statu_code != 200)
		{
			end(sock, statu_code);
		}
		else
		{
			close(sock);
			printf("线程已经退出\n");
			pthread_exit(NULL);
		}

	}
	else
	{
		//请求方法不是GET也不是POST
		clear_head(sock);
		statu_code = 400;
		end(sock, statu_code);
	}
}

//记录日志
void write_log(char *message)
{
	int fd = open("log.log", O_WRONLY|O_APPEND);
	if(fd < 0)
	{
		perror("write log");
		printf("日志线程已经退出\n");
		pthread_exit(NULL);
	}
	char mess_buff[1024];
	sprintf(mess_buff,"[%s--%s]--%s\n", __DATE__, __TIME__, message);
	pthread_mutex_lock(&mutex);
	write(fd, mess_buff, strlen(mess_buff));
	pthread_mutex_unlock(&mutex);
	close(fd);
}

//用法
void usage(char *arg)
{
	printf("usage: %s[port]\n", arg);
	exit(1);//如果退出码为1的话说明是运行时的参数出错
}


//初始化监听套接字,以及互斥量,信号忽略
int sock_init(int port)
{
	signal(SIGPIPE, SIG_IGN);
	//初始化互斥量
	pthread_mutex_init(&mutex, NULL);
	//创建套接字
	int sock_fd = socket(AF_INET, SOCK_STREAM, 0);
	if(sock_fd < 0)
	{
		write_log("create sock error");
		return -2;//如果退出码为2的话代表套接字创建错误
	}
	
	//因为我们置考虑短连接, 即每次向客户端响应完成之后,就可以立马关闭socket了,
	//这样的话就是服务器主动断开连接,就会进入TIME_WAIT状态,
	//因为服务器要在算时间内处理大量的连接,所以服务器上就会出现大量的TIME_WAIT连接
	//因此需要设置setsocketopt REUSEADD来重用TIME_WAIT状态的连接
	int opt = 1;
	setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

	//绑定
	self_addr.sin_family = AF_INET;
	self_addr.sin_port = htons(port);
	self_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	int ret = bind(sock_fd, (struct sockaddr*)&self_addr, sizeof(self_addr));
	if(ret < 0)
	{
		write_log("bind error");
		return -3;
	}
	//监听
	ret = listen(sock_fd, 10);
	if(ret < 0)
	{
		write_log("listen error");
		return -4;
	}
	return sock_fd;
}

