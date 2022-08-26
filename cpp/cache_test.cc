#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

#include "cache.h"
#include <iostream>
using namespace std;

#define DEBUG
#ifdef DEBUG
#define LOG(frm, argc...) {\
    printf("[%s : %d] ", __func__, __LINE__);\
    printf(frm, ##argc);\
    printf("\n");\
}
#else
#define LOG(frm, argc...)
#endif

TEST_CASE("cache test") {
    ShardedLRUCache* cache = NewLRUCache(1<<20);
    Slice k{"abc"}, v{"def"};
    for(int i=0; i<10; ++i) {
        
        cache->Insert(k,&v,nullptr);
    }
    CHECK(cache->TotalCharge() == 1);

    auto res = cache->Lookup(k);
    REQUIRE(res != nullptr);
    auto t = reinterpret_cast<Slice*>(res->value);
    CHECK(t->ToString() == "def");
}

