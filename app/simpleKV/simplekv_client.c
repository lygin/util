#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <time.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <fcntl.h>

#define PORT 7778
#define MAX_LINE 2048
char sendbuf[MAX_LINE], recvbuf[MAX_LINE];

inline int max(int a, int b)
{
	return a > b ? a : b;
}

ssize_t readline(int fd, char *vptr, size_t maxlen)
{
	ssize_t n, rc;
	char c, *ptr;

	ptr = vptr;
	for (n = 1; n < maxlen; n++)
	{
		if ((rc = read(fd, &c, 1)) == 1)
		{
			*ptr++ = c;
			if (c == '\n')
				break; /* newline is stored, like fgets() */
		}
		else if (rc == 0)
		{
			*ptr = 0;
			return n - 1; /* EOF, n - 1 bytes were read */
		}
		else
			return -1; /* error, errno set by read() */
	}

	*ptr = 0; /* null terminate like fgets() */
	return n;
}

/*普通客户端消息处理函数*/
void str_cli(int sockfd)
{
	/*发送和接收缓冲区*/
	
	while (1)
	{
		printf("> ");
		if (fgets(sendbuf, MAX_LINE, stdin) == NULL) 
			break;
		int n = write(sockfd, sendbuf, strlen(sendbuf));
		if(n <= 0) {
			printf("write fail\n");
			exit(1);
		}
		memset(recvbuf, 0, MAX_LINE);
		if (readline(sockfd, recvbuf, MAX_LINE) == 0)
		{
			perror("server terminated prematurely");
			exit(1);
		} // if

		if (fputs(recvbuf, stdout) == EOF)
		{
			perror("fputs error");
			exit(1);
		} // if

		memset(sendbuf, 0, MAX_LINE);
	} // while
}

int main(int argc, char **argv)
{
	/*声明套接字和链接服务器地址*/
	int sockfd;
	struct sockaddr_in servaddr;

	/*判断是否为合法输入*/
	if (argc != 2)
	{
		perror("usage:tcpcli <IPaddress>");
		exit(1);
	} // if

	/*(1) 创建套接字*/
	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
	{
		perror("socket error");
		exit(1);
	} // if

	/*(2) 设置链接服务器地址结构*/
	memset(&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(PORT); // hostTonet
	if (inet_pton(AF_INET, argv[1], &servaddr.sin_addr) < 0)
	{
		printf("inet_pton error for %s\n", argv[1]);
		exit(1);
	} // if

	/*(3) 发送链接服务器请求*/
	if (connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
	{
		perror("connect error");
		exit(1);
	} // if

	/*调用消息处理函数*/
	str_cli(sockfd);
	exit(0);
}