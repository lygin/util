#include "ae.h"
#include "logging.h"

int timeProc(struct aeEventLoop *eventLoop, long long id, void *clientData) {
    LOG_INFO("every 1000 ms");
    return 1000;
}
// 没有文件事件，eventloop会空转获取事件，定时任务到达才处理时间时间
int main() {
    aeEventLoop *el = aeCreateEventLoop(100);
    aeCreateTimeEvent(el, 1, timeProc, NULL, NULL);
    aeMain(el);
    aeDeleteEventLoop(el);
}