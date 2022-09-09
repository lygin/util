/**
 * @file easylog_test.cc
 * @brief 可以直接打印STL容器，Logger线程安全
 * @version 0.1
 * @date 2022-09-06
 * 
 * @copyright Copyright (c) 2022
 * 
 */
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "easylog.h"
INITIALIZE_EASYLOGGINGPP

TEST_CASE("easy log") {
    el::Configurations conf("/home/lj/base_compo/cpp/easylog.conf");
    // Actually reconfigure all loggers instead
    el::Loggers::reconfigureAllLoggers(conf);

    //Logger class. This feature is thread and type safe
    // (as we do not use any macros like LOG(INFO) etc)
    el::Logger* Logger = el::Loggers::getLogger("default");
    uint64_t a = 1ull<<63;
    Logger->info("log %v", a);
    std::vector<int> v;
    v.emplace_back(1);
    Logger->info("log %v", v);
    
    //LOG(INFO) << "a=" << a;
}