#pragma once
#include <atomic>
#include <functional>
#include <list>
#include <cstddef>
#include <cstdlib>
#include <vector>
#include <memory>
#include <emmintrin.h>
#include <thread>
#include <cstdio>

#include "concurrentqueue.h"

#define likely(x) __builtin_expect(!!(x), 1) 
#define unlikely(x) __builtin_expect(!!(x), 0)
#define coro_run(_sche, _func) _sche->addTask(_func)

typedef void* fctx_t;
struct filectx {
    fctx_t fctx;
    void* coro;
};
extern "C" {
    filectx jump_fcontext(fctx_t to_ctx, void* to_coro_ptr);
    fctx_t make_fcontext(void* sp, size_t size, void (*enter)(filectx));
}

void co_wait();
void co_yield();

namespace Corot
{

using Task = std::function<void()>;
using TaskQueue = moodycamel::ConcurrentQueue<Task>;


enum class State {
    IDLE,
    Ready,
    Wait,
};

class Sche;
class Coro {
    friend class Sche;
    static constexpr size_t kCoroStackSize = 1<<20;
    public:
    class StackCtx {
        public:
        StackCtx() {
            stack_base_ = malloc(kCoroStackSize);
            stack_size_ = kCoroStackSize;
            fctx_ = make_fcontext(stack_base_ + stack_size_, stack_size_, &Coro::enter);
        }
        ~StackCtx() { free(stack_base_); }
        void* sp() { return stack_base_ + stack_size_; }
        size_t size() { return stack_size_; }

        fctx_t fctx() { return fctx_; }
        void set_fctx(fctx_t fctx) { fctx_ = fctx; }


        private:
        void* stack_base_;
        size_t stack_size_;
        fctx_t fctx_;
    };
    
    explicit Coro(Sche *sche): sche_(sche) {}

    void yield() {
        state_ = State::Ready;
        switch_coro((Coro*)sche_);
    }

    void co_wait() {
        state_ = State::Wait;
        switch_coro((Coro*)sche_);
    }

    void resume() { switch_coro(this); }

    private:
    static void enter(filectx from) {
        Coro *coro = reinterpret_cast<Coro *>(from.coro);
        Coro *sche = (Coro*)coro->sche_;
        sche->ctx_.set_fctx(from.fctx);
        coro->run();
        jump_fcontext(sche->ctx_.fctx(), nullptr);
    }

    void switch_coro(Coro *to) {
        filectx t = jump_fcontext(to->ctx_.fctx(), to);
        to->ctx_.set_fctx(t.fctx);
    }

    void run() {
        state_ = State::Ready;
        task_();
        state_ = State::IDLE;
        switch_coro((Coro*)sche_);
    }

    StackCtx ctx_;
    Sche *sche_;
    Task task_;
    std::list<Coro *>::iterator iter;
    State state_{State::IDLE};
    std::thread *thr_;
    
};

class Sche : public Coro{
    using CoroQueue = moodycamel::ConcurrentQueue<Coro*>;
    static constexpr uint32_t kMaxCoro = 128;
    public:
    explicit Sche(int coro_num) : Coro(this), coro_num_(coro_num) {
        for(int i=0; i<coro_num; ++i) {
            coros_.emplace_back(new Coro(this));
            idle_list_.emplace_back(coros_.back().get());
        }
        thr_ = new std::thread(&Sche::start, this);
    }
    ~Sche() {

    }
    void start();
    void exit() { stop_.store(true); }
    void addTask(Task &&task) { task_queue_.enqueue(std::move(task)); }


    Coro* current_;
    private:
    void dispatch() {
        if (unlikely(task_begin_ == task_size_)) {
            task_size_ = task_queue_.try_dequeue_bulk(task_buf_, kMaxCoro);
            task_begin_ = 0;
        }
        auto iter = idle_list_.begin();
        while (iter != idle_list_.end() && task_begin_ < task_size_) {
            auto coro = *iter;
            coro->task_ = std::move(task_buf_[task_begin_++]);
            coro->state_ = State::Ready;
            idle_list_.erase(iter++);
            ready_list_.emplace_back(coro);
        }
    }
    int coro_num_;

    TaskQueue task_queue_;
    Task task_buf_[kMaxCoro];
    uint32_t task_begin_{0};
    uint32_t task_size_{0};
    std::atomic<bool> stop_;
    CoroQueue wait_queue_;

    std::list<Coro*> idle_list_;
    std::list<Coro*> ready_list_;
    std::list<Coro*> wait_list_;
    
    std::vector<std::shared_ptr<Coro>> coros_;
};
}