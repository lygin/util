#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

#include "cache.h"
#include <iostream>
using namespace std;

constexpr uint32_t _1MB = 1<<20;
constexpr uint32_t _64MB = 1<<26;


TEST_CASE("cache test") {
    ShardedLRUCache* cache = NewLRUCache(_64MB);

    for(int i=0; i<10; ++i) {
        Slice k{"abc"}, v{"def"};
        cache->Insert(k,&v,1,nullptr);
    }
    cout<<cache->TotalCharge();
    Slice k{"abc"};
    auto res = cache->Lookup(k);
    if(res) {
        auto t = reinterpret_cast<Slice*>(res->value);
        CHECK(t->ToString() == "def");
    }
}

