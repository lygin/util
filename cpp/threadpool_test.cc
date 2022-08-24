#include "threadpool.h"
#include "easylog.h"
#include <iostream>

INITIALIZE_EASYLOGGINGPP
el::Logger* Logger = el::Loggers::getLogger("default");
int func(int i) {
    Logger->info("hello %v", i);
    std::this_thread::sleep_for(std::chrono::seconds(1));
    return i*i;
}

int main()
{
    
    ThreadPool pool(4);
    std::vector< std::future<int> > results;
    // auto res = std::async(func, 1);
    for(int i = 0; i < 8; ++i) {
        results.emplace_back(
            pool.enqueue(func, i)
        );
    }

    for(auto && result: results)
        Logger->info("res %v", result.get());
    
    return 0;
}