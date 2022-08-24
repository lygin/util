#include "rwlock.h"
#include "timer.h"
#include <iostream>
#include <vector>
#include <thread>

using namespace std;
vector<int> v;

int main() {
    RWLock rwlock;
    mutex mtx;
    Timer tt;
    thread t1([&rwlock](){
        
        for(int i=0; i<(1<<15); i++) {
            rwlock.WLock();
            v.push_back(i);
            rwlock.WUnlock();
        }
    });
    int idx = 0;
    thread t2([&rwlock, &idx](){
        while(1) {
            rwlock.RLock();
            if(idx<v.size())
                cout<<v[idx++]<<"\n";
            if(idx >= (1<<15)-1) break;
            rwlock.RUnlock();
        }
    });
    thread t3([&rwlock, &idx](){
        while(1) {
            rwlock.RLock();
            if(idx<v.size())
                cout<<v[idx++]<<"\n";
            if(idx >= (1<<15)-1) break;
            rwlock.RUnlock();
        }
    });
    t1.join();
    t2.join();
    t3.join();
    cout<<tt.GetDurationSec()<<"sec\n";
    return 0;
}