#include "libgo/coroutine.h"
#include <stdio.h>
#include "timer.h"
using namespace std;
const int MAX_CORO = 64;
const int N = 100'000;
atomic<bool> send_req[N];
atomic<bool> req_done[N];
atomic<bool> dma_ok;
atomic<int> req_id;
bool poll_cq(int id)
{
    return req_done[id];
}
bool dma_poll_cq()
{
    return dma_ok;
}

void dma_write()
{
    dma_ok = false;
}

void do_merge()
{
    int x = 0;
    while (true)
    {
        x++;
        if (x % 3 == 0)
        {
            dma_write(); // dma read-read-write
            while (dma_poll_cq() == false)
                co_yield;
            // printf("merge done\n");
        }
        co_yield;
    }
}

void do_flush()
{
    dma_write(); // dma write
    while (dma_poll_cq() == false)
        co_yield;
    req_done[req_id.load()] = true;
    req_id++;
}

void server()
{
    go do_merge;//async merge
    while (true)
    {
        if (send_req[req_id.load()].load())
        {
            go do_flush;//async flush
        }
        else
        {
            co_yield;
        }
    }
}
atomic<int> cur_id;
co::Scheduler *sched;
atomic<int> pending;
Timer *t;
void write()
{
    int id = cur_id.load();
    // printf("req %d send\n", id);
    req_done[id] = false;
    send_req[id] = true;
    if (poll_cq(id) == false)
        co_yield;
    // printf("req %d done\n", id);
    pending--;
    if(pending == 0 && cur_id==N) printf("pending: 0, dur: %fms\n", t->GetDurationMs());
}

void client_write()
{
    go co_scheduler(sched)[]
    {
        cur_id++;
        write();
    };
    pending++;
}

void client()
{
    // init
    sched = co::Scheduler::Create();
    std::thread t1([]{ sched->Start(1); });
    t1.detach();
    // test client write
    t = new Timer;
    for (int i = 0; i < N; ++i)
    {
        client_write();
    }
    printf("pending: %d, dur: %fms\n", pending.load(),t->GetDurationMs());
}
void dma_host()
{
    while (true)
    {
        usleep(1000); // 1ms 完成dma_write
        dma_ok = true;
    }
}
int main(int argc, char **argv)
{
    // client
    thread t1(client);
    t1.detach();
    thread t2(dma_host);
    t2.detach();

    // server
    //  在协程中使用co_yield关键字, 可以主动让出调度器执行权限,并将当前协程移动到可执行协程列表的尾部。(FIFO)
    go server;
    co_sched.Start();
    return 0;
}
