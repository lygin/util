#include "ae.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <time.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <fcntl.h>

#define DEBUG
#ifdef DEBUG
#define LOG(frm, argc...) {\
    printf("[%s : %d] ", __func__, __LINE__);\
    printf(frm, ##argc);\
    printf("\n");\
}
#else
#define LOG(frm, argc...)
#endif

#define PORT  7777
#define MAX_LINE 2048

void setNonblocking(int sockfd)
{
	int opts;
    opts=fcntl(sockfd,F_GETFL);
    if(opts<0)
    {
        perror("fcntl(sock,GETFL)");
        return;
    }//if

    opts = opts|O_NONBLOCK;
    if(fcntl(sockfd,F_SETFL,opts)<0)
    {
 		perror("fcntl(sock,SETFL,opts)");
        return;
    }//if
}

struct serverdata{

};
char buf[MAX_LINE];
void readFunc(struct aeEventLoop *eventLoop, int fd, void *clientData, int mask);

void writeFunc(struct aeEventLoop *eventLoop, int fd, void *clientData, int mask) {
    int connfd, ret;
    int n = (int)clientData;
    if((connfd = fd) < 0)
        return;
    
    if((ret = write(connfd , buf , n)) != n)	
    {
        printf("error writing to the sockfd!\n");
        return;
    }//if
    /*设置用于读的文件描述符和事件（原来读了没删不用设）*/
    //aeCreateFileEvent(eventLoop, connfd, AE_READABLE, readFunc, NULL);
}

void readFunc(struct aeEventLoop *eventLoop, int fd, void *clientData, int mask) {
    int connfd;
    if((connfd = fd) < 0)
        return;
    
    bzero(buf , MAX_LINE);
    printf("reading the socket~~~\n");
    int n;
    if((n = read(connfd , buf , MAX_LINE)) <= 0)
    {
        close(connfd);
        aeDeleteFileEvent(eventLoop, connfd, AE_READABLE);
    } else {
        buf[n] = '\0';
        printf("client send message: %s\n", buf);
        /*设置用于注册写操作文件描述符和事件*/
        aeCreateFileEvent(eventLoop, connfd, AE_WRITABLE, writeFunc, n);	
    }
}
//el为当前el，fd为就绪的fd，clidata为AddEvent的data，mask为就绪fd的AE_READ或AE_WRITE
void acceptFunc(struct aeEventLoop *eventLoop, int fd, void *clientData, int mask) {
    int listenfd = (int)clientData;
    if(fd == listenfd)
    {	
        /*接收客户端的请求*/
        struct sockaddr_in cliaddr;
        socklen_t clilen = sizeof(cliaddr);
        int connfd;
        //return cliaddr
        if((connfd = accept(listenfd , (struct sockaddr *)&cliaddr , &clilen)) < 0)
        {
            perror("accept error.\n");
            exit(1);
        }//if		

        printf("accpet a new client: %s:%d\n", inet_ntoa(cliaddr.sin_addr) , cliaddr.sin_port);
    
        /*设置为非阻塞*/
        setNonblocking(connfd);
        aeCreateFileEvent(eventLoop, connfd, AE_READABLE, readFunc, NULL);
    } else {
        readFunc(eventLoop, fd, clientData, mask);
    }
}



int main() {
    struct sockaddr_in servaddr;

    aeEventLoop* ae = aeCreateEventLoop(10);
    //socket, bind, and listen
    int listenfd = socket(AF_INET, SOCK_STREAM, 0);
    setNonblocking(listenfd);
    bzero(&servaddr , sizeof(servaddr));
    servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port = htons(PORT);
    bind(listenfd , (struct sockaddr *)&servaddr , sizeof(servaddr));
    listen(listenfd, 20);
    
    if(aeCreateFileEvent(ae, listenfd, AE_READABLE, acceptFunc, (void*)listenfd)) {
        LOG("create net event err");
    }
    aeMain(ae);
    
    aeDeleteEventLoop(ae);
}