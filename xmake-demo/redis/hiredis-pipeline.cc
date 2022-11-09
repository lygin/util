#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

#ifdef __cplusplus
extern "C" {
#endif

#include <hiredis/hiredis.h>>

#ifdef __cplusplus
}
#endif


int main (int argc, char **argv) {

    redisContext * c = redisConnect("127.0.0.1", 6379);

    if (c->err) {
        /* Let *c leak for now... */
        printf("Error: %s\n", c->errstr);
        return 1;
    }

    redisReply *reply;
    // pipeline of commands
    redisAppendCommand((redisContext *)c,"SET foo bar");
    redisAppendCommand((redisContext *)c,"SET foo2 bar2");
    redisAppendCommand((redisContext *)c,"SET foo3 bar3");
    redisAppendCommand((redisContext *)c,"GET foo");
    while(redisGetReply((redisContext *)c,(void**)&reply) == REDIS_OK) {
      // 得到pipeline命令的结果
      printf("%s\n", reply->str);
      freeReplyObject(reply);
    }

    return 0;
}