#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

#include "lrucache.h"
#include <iostream>
#include <memory>
using namespace std;

class XX{
    public:
    ~XX() {
    }
};
struct VAL{
    string x;
    XX a;
};

void DeleteVAL(void *v) {
    auto val = reinterpret_cast<VAL*>(v);
    delete val;
}

TEST_CASE("cache test") {
    shared_ptr<ShardedLRUCache> cache = make_shared<ShardedLRUCache>(1<<20, DeleteVAL);
    
    for(int i=0; i<10; ++i) {
        string k = "abc" + to_string(i);
        auto v = new VAL;
        v->x = "iam" + to_string(i);
        auto e = cache->Insert(k,v);
        //插入之后也算e在使用，不用e的时候需要release
        cache->Release(e);
    }
    CHECK(cache->TotalElem() == 10);

    string k = "abc0";
    auto res = cache->Lookup(k);
    //不能在此release，因为如果release后，并发情况下res被淘汰，那么res->val就有问题
    //应该不读写res的时候，release
    REQUIRE(res != nullptr);
    auto t = reinterpret_cast<VAL*>(res->value);
    CHECK(t->x == "iam0");
    cache->Release(res);
    k = "abc11";
    res = cache->Lookup(k);;
    REQUIRE(res == nullptr);
}

