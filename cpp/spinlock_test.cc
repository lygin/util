#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "easylog.h"
#include "tc_threadpool.h"
#include "timer.h"
#include "spinlock.h"
#include "common.h"
#include <cstring>

INITIALIZE_EASYLOGGINGPP

constexpr int N = 1'000'000;
el::Logger* Logger = el::Loggers::getLogger("default");
std::queue<int> q;
std::mutex mtx;
TC_SpinLock lk;

TEST_CASE("spinlock") {
    TC_ThreadPool tp;
    tp.init(16);
    tp.start();
    Timer t;
    std::vector< std::future<int> > res;
    for(int i=0; i<8; ++i) {
        res.emplace_back(
            tp.exec([]{
                for(int i=0; i<N; ++i) {
                    lk.lock();
                    q.emplace(i);
                    lk.unlock();
                }
                return 1;
            })
        );
    }
    for(auto&& re : res) {
        re.get();
    }
    CHECK(q.size() == 8*N);
    for(int i=0; i<8; ++i) {
        tp.exec([]{
            for(int i=0; i<N; ++i) {
                lk.lock();
                IGNORE_UNUSE int x = std::move(q.front());
                q.pop();
                lk.unlock();
            }
        });
    }
    
    tp.waitForAllTaskDone();
    CHECK(q.size() == 0);
    double tsec = t.GetDurationSec();
    Logger->info("totol time %v", tsec);
}