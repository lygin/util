#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

#ifdef __cplusplus
extern "C" {
#endif

#include <hiredis/hiredis.h>>
#include <hiredis/async.h>
#include <hiredis/adapters/libevent.h>

#ifdef __cplusplus
}
#endif

void getCallback(redisAsyncContext *c, void *r, void *privdata) {
    redisReply *reply = (redisReply *)r;
    if (reply == NULL) {
        if (c->errstr) {
            printf("errstr: %s\n", c->errstr);
        }
        return;
    }
    printf("argv[%s]: %s\n", (char*)privdata, reply->str);

    /* Disconnect after receiving the reply to GET */
    redisAsyncDisconnect(c);
}

void connectCallback(const redisAsyncContext *c, int status) {
    if (status != REDIS_OK) {
        printf("Error: %s\n", c->errstr);
        return;
    }
    printf("Connected...\n");
}

void disconnectCallback(const redisAsyncContext *c, int status) {
    if (status != REDIS_OK) {
        printf("Error: %s\n", c->errstr);
        return;
    }
    printf("Disconnected...\n");
}

int main (int argc, char **argv) {

    struct event_base *base = event_base_new();
    redisAsyncContext * c = redisAsyncConnect("127.0.0.1", 6379);

    if (c->err) {
        /* Let *c leak for now... */
        printf("Error: %s\n", c->errstr);
        return 1;
    }

    redisLibeventAttach(c,base);
    redisAsyncSetConnectCallback(c,connectCallback);
    redisAsyncSetDisconnectCallback(c,disconnectCallback);
    //二进制数据%b
    //异步调用，不需要后续处理（回调）
    redisAsyncCommand(c, NULL, NULL, "SET key %b", argv[argc-1], strlen(argv[argc-1]));
    //异步调用get key，完成后自动调用getCallback
    redisAsyncCommand(c, getCallback, (char*)"end-1", "GET key");

    event_base_dispatch(base);
    return 0;
}