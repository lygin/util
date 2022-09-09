#include "tc_threadpool.h"
#include "timer.h"
#include "logging.h"
#include <cassert>

int func(int i) {
    //LOG_INFO("hello %d", i);
    //std::this_thread::sleep_for(std::chrono::seconds(1));
    return i*i;
}
int main() {
    ThreadPool tp;
    tp.init(8);
    tp.start();
    vector<std::future<int>> res;
    Timer tm;
    for(int i = 0; i < 100000; ++i) {
        res.emplace_back(
            tp.exec(func, i)
        );
    }
    int i = 0;
    for(auto & result: res) {
        int x = result.get();
        assert(x == i*i), i++;
        //LOG_INFO("res %d",x);
    }
    LOG_INFO("total time %fs", tm.GetDurationSec());
}