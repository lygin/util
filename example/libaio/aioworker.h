#pragma once

#include <libaio.h>
#include <cassert>
#include <thread>
#include "waitgroup.h"
#include "logging.h"
#include <atomic>
#include "lfqueue.h"


enum OP {
    READ,
    WRITE
};

struct Task {
    OP op;
    int fd;
    void* buf;
    size_t off;
    size_t len;
    WaitGroup* wg;
};
class AIOworker {
private:
    io_context_t ctx_;
    int maxevents_;
    std::thread *th_;
    std::atomic<bool> stop_;
    void poller() {
        struct io_event events[maxevents_];
        timespec ts;
        ts.tv_nsec = 0;
        ts.tv_sec = 1;
        while(true) {
            if (stop_.load() == true) break;
            int ret = io_getevents(ctx_, 1, maxevents_, events, &ts);
            if (ret < 0)
            {
                LOG_ERROR("ret %d %s", ret, strerror(-ret));
                assert(0);
            } else if(ret == 0) {
                asm volatile("pause");
            }
            for (int i = 0; i < ret; i++)
            {
                struct iocb *completed_iocb = events[i].obj;
                // Process the completed I/O operation here
                WaitGroup* wg = (WaitGroup*)completed_iocb->data;
                wg->Done();
            }
        }
    }
public:
    AIOworker(int maxevents=8): maxevents_(maxevents), stop_(false) {
        ctx_ = NULL;
        int ret = io_setup(maxevents_, &ctx_);
        if (ret < 0)
        {
            LOG_ERROR("ret %d %s", ret, strerror(-ret));
            assert(0);
        }
        th_ = new std::thread(&AIOworker::poller, this);
    }
    ~AIOworker(){
        stop_.store(true);
        th_->join();
        delete th_;
        io_destroy(ctx_);
    }
    int submit_task(Task *task) {
        assert(((size_t)task->buf & 0x1ff) == 0);
        assert((task->off & 0x1ff) == 0 );
        assert((task->len & 0x1ff) == 0);
        struct iocb *cb = (struct iocb *)calloc(1, sizeof(struct iocb));
        if(task->op == READ) {
            io_prep_pread(cb, task->fd, task->buf, task->len, task->off);
        } else {
            io_prep_pwrite(cb, task->fd, task->buf, task->len, task->off);
        }
        cb->data = task->wg;
        int ret = io_submit(ctx_, 1, &cb);
        assert(ret == 1);
        return ret;
    }
};

class AIOworker2 {
private:
    io_context_t ctx_;
    int maxevents_;
    std::thread *th_;
    std::atomic<bool> stop_;
    moodycamel::ConcurrentQueue<Task*> task_queue_;
    void submit()
    {
        Task *task[maxevents_];
        int count = task_queue_.try_dequeue_bulk(task, maxevents_);
        if (count == 0) return;
        struct iocb *cb = (struct iocb *)calloc(count, sizeof(struct iocb));
        struct iocb **cbs = (struct iocb **)calloc(count, sizeof(struct iocb*));

        for(int i=0; i<count; ++i) {
            if(task[i]->op == READ) {
                io_prep_pread(&cb[i], task[i]->fd, task[i]->buf, task[i]->len, task[i]->off);
            } else {
                io_prep_pwrite(&cb[i], task[i]->fd, task[i]->buf, task[i]->len, task[i]->off);
            }
            cb[i].data = task[i]->wg;
            cbs[i] = &cb[i];
        }
        int ret = 0;

        ret += io_submit(ctx_, count, cbs);
 
        assert(ret == count);
    }
    void poller() {
        struct io_event events[maxevents_];
        timespec ts;
        ts.tv_nsec = 0;
        ts.tv_sec = 0;
        while(true) {
            if (stop_.load() == true) break;
            int ret = io_getevents(ctx_, 1, maxevents_, events, &ts);
            if (ret < 0)
            {
                LOG_ERROR("ret %d %s", ret, strerror(-ret));
                assert(0);
            } else if(ret == 0) {
                submit();
            }
            for (int i = 0; i < ret; i++)
            {
                struct iocb *completed_iocb = events[i].obj;
                // Process the completed I/O operation here
                WaitGroup* wg = (WaitGroup*)completed_iocb->data;
                wg->Done();
            }
        }
    }
public:
    AIOworker2(int maxevents=8): maxevents_(maxevents), stop_(false) {
        ctx_ = NULL;
        int ret = io_setup(maxevents_, &ctx_);
        if (ret < 0)
        {
            LOG_ERROR("ret %d %s", ret, strerror(-ret));
            assert(0);
        }
        th_ = new std::thread(&AIOworker2::poller, this);
    }
    ~AIOworker2(){
        stop_.store(true);
        th_->join();
        delete th_;
        io_destroy(ctx_);
    }
    int submit_task(Task *task) {
        assert(((size_t)task->buf & 0x1ff) == 0);
        assert((task->off & 0x1ff) == 0 );
        assert((task->len & 0x1ff) == 0);
        task_queue_.enqueue(task);
        return 0;
    }
};