#ifndef _THREAD_POOL_LOCKFREE_H_
#define _THREAD_POOL_LOCKFREE_H_

#include <vector>
#include <memory>
#include <thread>
#include <future>
#include <functional>
#include <stdexcept>
#include "lockfreeQueue.h"

namespace lockfree {

class ThreadPool
{
public:
    ThreadPool(size_t nthread, int queue_size=1024): stop(false), tasks(queue_size) {
        for (size_t i = 0; i < nthread; ++i)
            workers.emplace_back([this](){
                    while(true)
                    {
                        std::function<void()> task;
                        
                        if (stop && tasks.empty()) return;

                        if(tasks.try_pop(task)) {
                            task();
                        };
                    }
                });
    }
    template <class F, class... Args>
    auto enqueue(F &&f, Args &&...args) -> std::future<typename std::result_of<F(Args...)>::type> {
        using return_type = typename std::result_of<F(Args...)>::type;

        auto task = std::make_shared<std::packaged_task<return_type()>>(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...)
        );

        std::future<return_type> res = task->get_future();

        tasks.emplace([task](){ 
            (*task)(); 
        });

        return res;
    }
    ~ThreadPool() {
        stop = true;
        for (auto &worker : workers) {
            worker.join();
        }
    }
private:
    // need to keep track of threads so we can join them
    std::vector<std::thread> workers;
    // the task queue
    MPMCRing< std::function<void()> > tasks;
    std::atomic<bool> stop;
};
}

#endif
