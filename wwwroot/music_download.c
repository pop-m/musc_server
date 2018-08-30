//调用酷狗音乐的API获得歌曲的地址,将此地址以一个button的方式返回给浏览器
//1.根据数据拼接获得filehash字段和album_id字段
//2.通过两个字段拼接请求music_data的url
//3.从music_data的url响应中获取music_url
//4.将music_url制成html返回给浏览器
////////////////////////////////////////////////////////////////////////
#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include<netinet/in.h>
#include<string.h>
#include<netdb.h>
#include<ctype.h>
#include<fcntl.h>

#define HOST "http://www.baidu.com"

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
void return_html(char *mp3_name, char *mp3_url, char *ret);
void return_client_error()
{
	return_html("抱歉,没有找到", HOST, "回到首页");

}
void return_ok(char *mp3_url ,char *mp3_name)
{
	return_html(mp3_name, mp3_url, "开始下载");
}

void return_server_error()
{
	printf("<html><body><h1>不好意思！我们的服务器出了错误,请您稍后再是一下哟！</h1></body></html>");
	exit(1);
}

typedef struct seq_list
{
	int top;
	char *data;
	int capacity;
} seq_list_t;
static void seq_init(seq_list_t *seq_list)
{
	seq_list->top = 0;
	seq_list->data = malloc(1024);
	seq_list->capacity = 1024;
}
static void seq_push(seq_list_t *seq_list, char c)
{
	if(seq_list->top < seq_list->capacity - 1)
	{
		seq_list->data[seq_list->top++] = c;
	}
	else
	{
		seq_list->capacity *= 2;
		char *p = realloc(seq_list->data, seq_list->capacity);
		if(p == NULL)
		{
			return_server_error();
		}
		else
		{
			seq_list->data = p;
		}
		seq_list->data[seq_list->top++] = c;
	}
}


void return_html(char *mp3_name, char *mp3_url, char *ret)
{

	seq_list_t seq;
	seq_init(&seq);
	//首先从文件中将所有的数据读入
	//首先是传送文件1
	
	int fd = open("wwwroot/result1.html", O_RDONLY);
	char c = 0;
	int count = lseek(fd, -1, SEEK_END);
	lseek(fd, 0, SEEK_SET);
	while(read(fd, &c, 1) > 0 && count > 0)
	{
		seq_push(&seq, c);
		fflush(stdout);
		count--;
	}
	close(fd);
	char name[20] = {0};
	url_to_gb_or_utf(mp3_name, name);

	char *p = name;
	while(*p != 0)
	{
		seq_push(&seq, *p++);
	}

	fd = open("wwwroot/result2.html", O_RDONLY);
	count = lseek(fd, -1, SEEK_END);
	lseek(fd, 0, SEEK_SET);
	while(read(fd, &c, 1) > 0 && count > 0)
	{
		seq_push(&seq, c);
		count--;
	}
	close(fd);

	p = mp3_url;
	while(*p != 0)
	{
		seq_push(&seq, *p++);
	}
	fd = open("wwwroot/result3.html", O_RDONLY);
	count = lseek(fd, -1, SEEK_END);
	lseek(fd, 0, SEEK_SET);
	while(read(fd, &c, 1) > 0 && count > 0)
	{
		seq_push(&seq, c);
		count--;
	}
	close(fd);

	p = ret;
	while(*p != 0)
	{
		seq_push(&seq, *p++);
	}
	
	fd = open("wwwroot/result4.html", O_RDONLY);
	count = lseek(fd, -1, SEEK_END);
	lseek(fd, 0, SEEK_SET);
	while(read(fd, &c, 1) > 0 && count > 0)
	{
		seq_push(&seq, c);
		count--;
	}
	close(fd);

	seq_push(&seq, 0);
	int length = strlen(seq.data);
	write(1, seq.data, length+1);

	fd = open("wwwroot/ret.txt", O_WRONLY);
	write(fd, seq.data, length+1);
}


int connect_to(int sock, char *hostname)
{
	//根据域名解析主机地址
	struct hostent *hs = gethostbyname(hostname);
	if(hs == NULL)
	{
		return -1;
	}
	struct sockaddr_in server_addr;
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(80);
	int i = 0;
	server_addr.sin_addr = *((struct in_addr*)hs->h_addr_list[0]);
	socklen_t len = sizeof(server_addr);
	while(connect(sock, (struct sockaddr*)&server_addr, len) < 0)
	{
		if(hs->h_addr_list[++i] != NULL)
		{
			server_addr.sin_addr = *((struct in_addr*)hs->h_addr_list[i]);
		}
		else
		{
			return -1;
		}
	}
	return 0;
}


void read_line(int sock, char *line, int size)	
{
	char c = 'x';
	int i = 0;
	while(c != '\n' && i < size)
	{
		int s = recv(sock, &c, 1, 0);
		if(s < 0)
		{
			//return_error();
		}
		if(c == '\r')
		{
			recv(sock, &c, 1, MSG_PEEK);
			if(c == '\n')
			{
				recv(sock, &c, 1, 0);
			}
			c = '\n';
		}
		line[i++] = c;
	}
	line[i] = 0;
}

void clear_head(int sock)
{
	char line[1024] = {0};
	do
	{
		read_line(sock, line, 1023);	
	}while(strcmp(line, "\n"));
}

char* search_data(char *src, char *need)
{
	char all_need[20] = {0};
	sprintf(all_need, "\"%s\":\"", need);
	char *start_pos = strstr(src, all_need);
	if(start_pos != NULL)
	{
		char *pos = start_pos + strlen(all_need);
		while(*pos != '\"')
		{
			pos++;
		}
		*pos = 0;
		return start_pos+strlen(all_need);
	}
	else
	{
		return NULL;
	}
}

void request(int sock, char *url)
{
	//构造key的请求
	char first_line[1024] = {0};
	sprintf(first_line, "GET %s HTTP/1.0\r\n", url);
	send(sock, first_line, strlen(first_line), 0);
	send(sock, "\r\n", 2, 0);
}

void seq_clear(seq_list_t *seq)
{
	memset(seq->data, 0x00, seq->capacity);
	seq->top = 0;
}
void seq_destroy(seq_list_t *seq)
{
	free(seq->data);
	seq->data = NULL;
	seq->top = 0;
	seq->capacity = 0;
}


int main()
{
	char *method = getenv("METHOD");
	if(strcasecmp(method, "POST") == 0)
	{
		return_client_error();
	}
	//获得歌曲名字
	char *query_string = getenv("QUERY_STRING");
	char music_name[50] = {0};
	sscanf(query_string, "music_name=%s", music_name);

	int sock = socket(AF_INET, SOCK_STREAM, 0);
	if(sock < 0)
	{
		return_server_error();
	}
	char *hostname = "songsearch.kugou.com";
	if(connect_to(sock, hostname)<0)
	{
		return_server_error();
	}
	
	//按照所给的sock和url请求服务器的响应
	char key_url[1024] = {0};
	//拼接请求filehash和album_id字段的url
	sprintf(key_url, "http://songsearch.kugou.com/song_search_v2?keyword=%s&page=1&pagesize=1&userid=-1&clientver=&platform=WebFilter&tag=em&filter=2&iscorrection=1&privilege_filter=0", music_name);
	request(sock, key_url);

	seq_list_t seq;
	seq_init(&seq);

	clear_head(sock);
	char c = 0;
	while(recv(sock, &c, 1, 0) != 0)
	{
		seq_push(&seq, c);
	}
	//抽取album 和 hash字段
	char *AlbumID = search_data(seq.data, "AlbumID");
	char album[20] = {0};
	if(album == NULL)
	{
		return_client_error();
	}
	strcpy(album, AlbumID);
	char *FileHash = search_data(seq.data, "FileHash");
	char hash[50] = {0};
	if(hash == NULL)
	{
		return_client_error();
	}
	strcpy(hash, FileHash);
	close(sock);
	seq_clear(&seq);

	sock = socket(AF_INET, SOCK_STREAM, 0);
	if(connect_to(sock, "www.kugou.com")<0)
	{
		return_server_error();
	}
	char mp3_data_url[1024] = {0};
	sprintf(mp3_data_url, "http://www.kugou.com/yy/index.php?r=play/getdata&hash=%s&album_id=%s&_=1497972864535", hash, album);
	request(sock, mp3_data_url);
	c = 0;

	while(recv(sock, &c, 1, 0) !=  0)
	{
		seq_push(&seq, c);
	}
	char *mp3_url = search_data(seq.data, "play_url");
	if(mp3_url == NULL)
	{
		return_client_error();
	}
	char url[200] = {0};
	int i = 0;
	int j = 0;
	while(mp3_url[i] != 0)
	{
		if(mp3_url[i] != '\\')
		{
			url[j++] = mp3_url[i];
		}
		i++;
	}
	url[j] = 0;

	return_ok(url, music_name);
	
	seq_destroy(&seq);
	close(sock);
	return 0;
}
