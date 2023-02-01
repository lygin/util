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

extern "C" {
#include "ae.h"
}

#include "logging.h"
#include <unordered_map>
#include <string>
#include "spinlock.h"
#include "threadpool.h"

#define PORT 7778
#define MAX_LINE 2048
enum {
    kRead,
    kWrite
};

struct client_data
{
    int connfd;
    struct sockaddr_in cliaddr;
    char sendbuf[MAX_LINE];
    int sendbuf_size;
    char recvbuf[MAX_LINE];
    // Client status, if no status control, read/write func may trigger many times and do lots of reads and writes
    // Cause the buffer is not immediately retrived by main thread.
    int status;
};

struct server
{
    std::unordered_map<std::string, std::string> store;
    ThreadPool pool;
    aeEventLoop *ae;
    SpinMutex store_lock;
    server(size_t back_threads, int max_events): pool(back_threads), ae(aeCreateEventLoop(max_events)) {}
};

server *srv;

void setNonblocking(int sockfd)
{
    int opts;
    opts = fcntl(sockfd, F_GETFL);
    if (opts < 0)
    {
        perror("fcntl(sock,GETFL)");
        return;
    }

    opts = opts | O_NONBLOCK;
    if (fcntl(sockfd, F_SETFL, opts) < 0)
    {
        perror("fcntl(sock,SETFL,opts)");
        return;
    }
}

void readFunc(struct aeEventLoop *eventLoop, int fd, void *clientData, int mask);
void writeFunc(struct aeEventLoop *eventLoop, int fd, void *clientData, int mask);

void handle_req(client_data *clientData) {
    char res[MAX_LINE] = "INVALID\n";
    char op[MAX_LINE], key[MAX_LINE], value[MAX_LINE];
    if(sscanf(clientData->recvbuf, "%s", op) < 1) {
        LOG_ERROR("NO OP");
        goto EXIT;
    }
    if(!strcmp(op, "PUT")) {
        if(sscanf(clientData->recvbuf, "%s %s %s", op, key, value) < 3) {
            LOG_ERROR("PUT FORMAT ERR");
            goto EXIT;
        }
        srv->store_lock.lock();
        srv->store[std::string(key)] = std::string(value);
        srv->store_lock.unlock();

        sprintf(res, "SUCCESS\n");
    } else if(!strcmp(op, "GET")) {
        if(sscanf(clientData->recvbuf, "%s %s", op, key) < 2) {
            LOG_ERROR("GET FORMAT ERR");
            goto EXIT;
        }
        srv->store_lock.lock();
        if(srv->store.find(key) != srv->store.end()) {
            sprintf(res, "%s\n", srv->store[key].c_str());
            srv->store_lock.unlock();
        } else {
            srv->store_lock.unlock();
            sprintf(res, "FAIL\n");
        }

    } else if(!strcmp(op, "DEL")) {
        if(sscanf(clientData->recvbuf, "%s %s", op, key) < 2) {
            LOG_ERROR("DEL FORMAT ERR");
            goto EXIT;
        }
        srv->store_lock.lock();
        srv->store.erase(key);
        srv->store_lock.unlock();
        sprintf(res, "SUCCESS\n");
    }
EXIT:
    memset(clientData->sendbuf, 0, MAX_LINE);
    sprintf(clientData->sendbuf, "%s", res);
    clientData->sendbuf_size = strlen(clientData->sendbuf);
}

void process_read(client_data *clientDatad) {
    int n = read(clientDatad->connfd, clientDatad->recvbuf, MAX_LINE);
    
    if (n <= 0)
    {
        //close(clientDatad->connfd);
        aeDeleteFileEvent(srv->ae, clientDatad->connfd, AE_READABLE);
    }
    else
    {
        clientDatad->recvbuf[n] = '\0';
        LOG_INFO("recv message from %s:%d(len %d): %s\n", inet_ntoa(clientDatad->cliaddr.sin_addr), 
            clientDatad->cliaddr.sin_port, n, clientDatad->recvbuf);
        handle_req(clientDatad);
        /*删除读事件，设置用于注册写操作文件描述符和事件*/
        aeDeleteFileEvent(srv->ae, clientDatad->connfd, AE_READABLE);
        aeCreateFileEvent(srv->ae, clientDatad->connfd, AE_WRITABLE, writeFunc, clientDatad);
    }
}

void process_write(client_data *clientDatad) {
    int ret;
    LOG_INFO("write back to client %s:%d: %s\n", inet_ntoa(clientDatad->cliaddr.sin_addr), 
        clientDatad->cliaddr.sin_port, clientDatad->sendbuf);
    if ((ret = write(clientDatad->connfd, clientDatad->sendbuf, clientDatad->sendbuf_size)) != clientDatad->sendbuf_size)
    {
        printf("error writing to the sockfd!\n");
        return;
    }
    /*删除写事件，设置用于读的文件描述符和事件*/
    aeDeleteFileEvent(srv->ae, clientDatad->connfd, AE_WRITABLE);
    /*准备下一次读*/
    aeCreateFileEvent(srv->ae, clientDatad->connfd, AE_READABLE, readFunc, clientDatad);
}

void writeFunc(struct aeEventLoop *eventLoop, int fd, void *clientData, int mask)
{
    int connfd;
    client_data *clientDatad = (client_data *)clientData;
    if(clientDatad->status != kWrite) return;
    if ((connfd = fd) < 0)
        return;
    clientDatad->status = kRead;
    srv->pool.enqueue(process_write, clientDatad);
}

void readFunc(struct aeEventLoop *eventLoop, int fd, void *clientData, int mask)
{
    int connfd;
    if ((connfd = fd) < 0)
        return;
    client_data *clientDatad = (client_data *)clientData;
    if(clientDatad->status != kRead) return;
    clientDatad->status = kWrite;
    srv->pool.enqueue(process_read, clientDatad);
}

// el为当前el，fd为就绪的fd，clidata为AddEvent的data，mask为就绪fd的AE_READ或AE_WRITE
void acceptFunc(struct aeEventLoop *eventLoop, int fd, void *clientData, int mask)
{
    int listenfd = (long long)clientData;
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
        }

        LOG_INFO("accpet a new client: %s:%d\n", inet_ntoa(cliaddr.sin_addr), cliaddr.sin_port);
        client_data *clientData = new client_data;
        memset(clientData, 0, sizeof(*clientData));
        clientData->connfd = connfd;
        clientData->cliaddr = cliaddr;

        setNonblocking(connfd);
        aeCreateFileEvent(eventLoop, connfd, AE_READABLE, readFunc, clientData);
    }
}



server *initServer()
{
    server *psrv;
    psrv = new server(4, 10);
    return psrv;
}

int main()
{
    // socket, bind, and listen
    struct sockaddr_in servaddr;
    int listenfd = socket(AF_INET, SOCK_STREAM, 0);
    setNonblocking(listenfd);
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(PORT);
    bind(listenfd, (struct sockaddr *)&servaddr, sizeof(servaddr));
    listen(listenfd, 20);

    srv = initServer();
    
    if (aeCreateFileEvent(srv->ae, listenfd, AE_READABLE, acceptFunc, (void *)listenfd))
    {
        LOG_ERROR("create net event err");
    }

    
    LOG_INFO("server started");
    aeMain(srv->ae);

    aeDeleteEventLoop(srv->ae);
}