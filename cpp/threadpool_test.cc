#include "threadpool.h"
#include "logging.h"
#include "timer.h"
#include <iostream>
#include <cassert>
int func(int i) {
    //LOG_INFO("hello %d", i);
    //std::this_thread::sleep_for(std::chrono::seconds(1));
    return i*i;
}

int main()
{
    
    ThreadPool pool(8);
    std::vector<std::future<int>> results;
    // auto res = std::async(func, 1);
    Timer tm;
    for(int i = 0; i < 100000; ++i) {
        results.emplace_back(
            pool.enqueue(func, i)
        );
    }
    int i = 0;
    for(auto & result: results) {
        int x = result.get();
        assert(x == i*i), i++;
        //LOG_INFO("res %d",x);
    }
    LOG_INFO("total time %fs", tm.GetDurationSec());
    return 0;
}