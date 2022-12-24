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

#include "ae.h"
#include "logging.h"
#include "hashtable.h"

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

struct server
{
    dict *hashtable;
};

struct server *srv;

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

char op[MAX_LINE], key[MAX_LINE], value[MAX_LINE];
void handle_req(struct client_data *clientData) {
    char res[MAX_LINE] = "INVALID\n";
    if(sscanf(clientData->recvbuf, "%s", op) < 1) {
        LOG_ERROR("NO OP");
        goto EXIT;
    }
    if(!strcmp(op, "PUT")) {
        if(sscanf(clientData->recvbuf, "%s %s %s", op, key, value) < 3) {
            LOG_ERROR("PUT FORMAT ERR");
            goto EXIT;
        }
        int ret = dictAdd(srv->hashtable, (void*)key, (void*)value);
        
        if(ret == DICT_OK) {
            sprintf(res, "SUCCESS\n");
        } else {
            sprintf(res, "FAIL\n");
        }
    } else if(!strcmp(op, "GET")) {
        if(sscanf(clientData->recvbuf, "%s %s", op, key) < 2) {
            LOG_ERROR("GET FORMAT ERR");
            goto EXIT;
        }
        char *ret = dictFetchValue(srv->hashtable, key);
        if(ret != NULL) {
            sprintf(res, "%s\n", ret);
        } else {
            sprintf(res, "FAIL\n");
        }

    } else if(!strcmp(op, "DEL")) {
        if(sscanf(clientData->recvbuf, "%s %s", op, key) < 2) {
            LOG_ERROR("DEL FORMAT ERR");
            goto EXIT;
        }
        int ret = dictDelete(srv->hashtable, key);
        if(ret == DICT_OK) {
            sprintf(res, "SUCCESS\n");
        } else {
            sprintf(res, "FAIL\n");
        }
    }
EXIT:
    memset(clientData->sendbuf, 0, MAX_LINE);
    sprintf(clientData->sendbuf, "%s", res);
    clientData->sendbuf_size = strlen(clientData->sendbuf);
}

void writeFunc(struct aeEventLoop *eventLoop, int fd, void *clientData, int mask)
{
    int connfd, ret;
    struct client_data *clientDatad = clientData;
    if ((connfd = fd) < 0)
        return;
    
    handle_req(clientDatad);
    LOG_INFO("write back to client %s:%d: %s\n", inet_ntoa(clientDatad->cliaddr.sin_addr), clientDatad->cliaddr.sin_port, clientDatad->sendbuf);
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
        LOG_INFO("recv message from %s:%d: %s\n", inet_ntoa(clientDatad->cliaddr.sin_addr), clientDatad->cliaddr.sin_port, clientDatad->recvbuf);
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

        LOG_INFO("accpet a new client: %s:%d\n", inet_ntoa(cliaddr.sin_addr), cliaddr.sin_port);
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

struct server *initServer()
{
    struct server *psrv;
    psrv = malloc(sizeof(*psrv));
    psrv->hashtable = dictCreate(&dictTypeHeapStringCopyKeyValue, NULL);
    LOG_ASSERT(psrv->hashtable != NULL, "init server failed");
    dictAdd(psrv->hashtable, "version", "1.0");
    char *val = dictFetchValue(psrv->hashtable,"version");
    if(val != NULL) LOG_INFO("server version:%s", val);
    else LOG_ERROR("server version not found");
    return psrv;
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

    srv = initServer();

    if (aeCreateFileEvent(ae, listenfd, AE_READABLE, acceptFunc, (void *)listenfd))
    {
        LOG_ERROR("create net event err");
    }

    
    LOG_INFO("server started");
    aeMain(ae);

    aeDeleteEventLoop(ae);
}