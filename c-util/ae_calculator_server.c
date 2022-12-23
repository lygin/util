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
#define LOG(frm, argc...)                         \
    {                                             \
        printf("[%s : %d] ", __func__, __LINE__); \
        printf(frm, ##argc);                      \
        printf("\n");                             \
    }
#else
#define LOG(frm, argc...)
#endif

#define PORT 7778
#define MAX_LINE 2048

struct client_data
{
    int connfd;
    struct sockaddr_in cliaddr;
    char sendbuf[MAX_LINE];
    int sendbuf_size;
    char recvbuf[MAX_LINE];
};

void setNonblocking(int sockfd)
{
    int opts;
    opts = fcntl(sockfd, F_GETFL);
    if (opts < 0)
    {
        perror("fcntl(sock,GETFL)");
        return;
    } // if

    opts = opts | O_NONBLOCK;
    if (fcntl(sockfd, F_SETFL, opts) < 0)
    {
        perror("fcntl(sock,SETFL,opts)");
        return;
    } // if
}

void readFunc(struct aeEventLoop *eventLoop, int fd, void *clientData, int mask);

void handle_req(struct client_data *clientData) {
    int a, b;
    char op;
    sscanf(clientData->recvbuf, "%d%c%d", &a, &op, &b);
    int res = 0;
    switch (op) {
        case '+':
            res = a+b;
            break;
        case '-':
            res = a-b;
            break;
        case '*':
            res = a*b;
            break;
        case '/':
            res = a/b;
            break;
    }
    memset(clientData->sendbuf, 0, MAX_LINE);
    sprintf(clientData->sendbuf, "%d\n", res);
    clientData->sendbuf_size = strlen(clientData->sendbuf);
}

void writeFunc(struct aeEventLoop *eventLoop, int fd, void *clientData, int mask)
{
    int connfd, ret;
    struct client_data *clientDatad = clientData;
    if ((connfd = fd) < 0)
        return;
    
    handle_req(clientDatad);
    LOG("write back to client %s:%d: %s\n", inet_ntoa(clientDatad->cliaddr.sin_addr), clientDatad->cliaddr.sin_port, clientDatad->sendbuf);
    if ((ret = write(connfd, clientDatad->sendbuf, clientDatad->sendbuf_size)) != clientDatad->sendbuf_size)
    {
        printf("error writing to the sockfd!\n");
        return;
    } // if
    /*删除写事件，设置用于读的文件描述符和事件*/
    aeDeleteFileEvent(eventLoop, connfd, AE_WRITABLE);
    /*准备下一次读*/
    aeCreateFileEvent(eventLoop, connfd, AE_READABLE, readFunc, clientDatad);
}

void readFunc(struct aeEventLoop *eventLoop, int fd, void *clientData, int mask)
{
    int connfd;
    if ((connfd = fd) < 0)
        return;
    struct client_data *clientDatad = clientData;
    memset(clientDatad->recvbuf, 0, MAX_LINE);
    int n;
    if ((n = read(connfd, clientDatad->recvbuf, MAX_LINE)) <= 0)
    {
        close(connfd);
        aeDeleteFileEvent(eventLoop, connfd, AE_READABLE);
    }
    else
    {
        clientDatad->recvbuf[n] = '\0';
        LOG("recv message from %s:%d: %s\n", inet_ntoa(clientDatad->cliaddr.sin_addr), clientDatad->cliaddr.sin_port, clientDatad->recvbuf);
        /*删除读事件，设置用于注册写操作文件描述符和事件*/
        aeDeleteFileEvent(eventLoop, connfd, AE_READABLE);
        aeCreateFileEvent(eventLoop, connfd, AE_WRITABLE, writeFunc, clientDatad);
    }
}
// el为当前el，fd为就绪的fd，clidata为AddEvent的data，mask为就绪fd的AE_READ或AE_WRITE
void acceptFunc(struct aeEventLoop *eventLoop, int fd, void *clientData, int mask)
{
    int listenfd = (int)clientData;
    if (fd == listenfd)
    {
        /*接收客户端的请求*/
        struct sockaddr_in cliaddr;
        socklen_t clilen = sizeof(cliaddr);
        int connfd;
        // return cliaddr
        if ((connfd = accept(listenfd, (struct sockaddr *)&cliaddr, &clilen)) < 0)
        {
            perror("accept error.\n");
            exit(1);
        } // if

        LOG("accpet a new client: %s:%d\n", inet_ntoa(cliaddr.sin_addr), cliaddr.sin_port);
        struct client_data *clientData = (struct client_data *)malloc(sizeof(struct client_data));
        memset(clientData, 0, sizeof(*clientData));
        clientData->connfd = connfd;
        clientData->cliaddr = cliaddr;
        /*设置为非阻塞*/
        setNonblocking(connfd);
        aeCreateFileEvent(eventLoop, connfd, AE_READABLE, readFunc, clientData);
    }
    else
    {
        readFunc(eventLoop, fd, clientData, mask);
    }
}

int main()
{
    struct sockaddr_in servaddr;

    aeEventLoop *ae = aeCreateEventLoop(10);
    // socket, bind, and listen
    int listenfd = socket(AF_INET, SOCK_STREAM, 0);
    setNonblocking(listenfd);
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(PORT);
    bind(listenfd, (struct sockaddr *)&servaddr, sizeof(servaddr));
    listen(listenfd, 20);

    if (aeCreateFileEvent(ae, listenfd, AE_READABLE, acceptFunc, (void *)listenfd))
    {
        LOG("create net event err");
    }
    aeMain(ae);

    aeDeleteEventLoop(ae);
}