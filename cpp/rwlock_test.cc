#include "rwlock.h"
#include "logging.h"
#include <iostream>
#include <vector>
#include <thread>

using namespace std;
const int N = 1<<20;
int a[N];

int main() {
    RWLock rwlock;
    mutex mtx;
    LOG_INFO("start");
    thread t1([&rwlock](){
        
        for(int i=0; i<N; i++) {
            rwlock.WLock();
            a[i] = i;
            rwlock.WUnlock();
        }
    });
    volatile int idx = 0;
    thread t2([&rwlock, &idx](){
        while(1) {
            rwlock.RLock();
            a[idx];
            idx++;
            if(idx >= N-1) {
                rwlock.RUnlock();
                break;
            }
            rwlock.RUnlock();
        }
    });
    thread t3([&rwlock, &idx](){
        while(1) {
            rwlock.RLock();
            a[idx];
            idx++;
            if(idx >= N-1) {
                rwlock.RUnlock();
                break;
            }
            rwlock.RUnlock();
        }
    });
    t1.join();
    t2.join();
    t3.join();
    LOG_INFO("end");
    return 0;
}