#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "threadpool.h"
#include "threadpool_lockfree.h"
#include "timer.h"
#include <iostream>
#include <cassert>
#include <unistd.h>

int func(int i) {
    //std::this_thread::sleep_for(std::chrono::seconds(1));
    return i*i;
}

TEST_CASE("threadpool") {
    ThreadPool pool(8);
    std::vector<std::future<int>> results;
    Timer tm;
    int testnum = 100000;
    for(int i = 0; i < testnum; ++i) {
        results.emplace_back(
            pool.enqueue(func, i)
        );
    }
    int i = 0;
    for(auto & result: results) {
        int x = result.get();
        REQUIRE(x == i*i);
        i++;
    }
    printf("threadpool task per time %fus\n", tm.GetDurationUs()/testnum);
}

TEST_CASE("threadpool_lockfree") {
    lockfree::ThreadPool<1024> pool(8);
    std::vector<std::future<int>> results;
    Timer tm;
    int testnum = 100000;
    for(int i = 0; i < testnum; ++i) {
        results.emplace_back(
            pool.enqueue(func, i)
        );
    }
    int i = 0;
    for(auto & result: results) {
        int x = result.get();
        REQUIRE(x == i*i);
        i++;
    }
    printf("threadpool_lockfree task per time %fus\n", tm.GetDurationUs()/testnum);
}