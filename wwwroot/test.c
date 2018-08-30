//                      _ooOoo_
//                     o8888888o
//                     88" . "88
//                     (| -_- |)
//                     0\  =  /O
//                  ____/`---'\____
//                .'  \\|     |//  `.
//               /  \\|||  :  |||//  \
//              /  _||||| -:- |||||-  \
//              |   | \\\  -  /// |   |
//             | \_|  ''\---/''  |   |
//             \  .-\__  `-`  ___/-. /
//             ___`. .'  /--.--\  `. . __
//          ."" '<  `.____<|>_/___.'  >'"".
//         | | :  `- \`.;`\ _ /`;.`/ - ` : | |
//         \  \ `-.   \_ __\ /__ _/   .-` /  /
//    ======`-.____`-.___\_____/___.-`____.-'======
//                      `=---='
//                 佛祖保佑，永无bug！！！
//                  版权所有：pop_m

#include<stdio.h>
#include<fcntl.h>
#include<unistd.h>
#include<stdlib.h>
#include<strings.h>


int main()
{
	char *method = getenv("METHOD");
	if(strcasecmp(method, "GET") == 0)
	{
		char *query_string = getenv("QUERY_STRING");
		printf("<html><body><h1>method:%s  query_string:%s</h1></body></html>", method, query_string);
	}
	else
	{
		int content_length = atoi(getenv("CONTENT_LENGTH"));
		char buff[1024] = {0};
		read(0, buff, 1023);
		printf("<html><body><h1>method:%s  post_string:%s</h1></body></html>", method, buff);
	}
	return 0;
}
