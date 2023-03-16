#include "cache.hh"
#include "lru_cache_policy.hh"
#include "timer.h"
#include "cassert"
#include "lrucache_stl.hh"
#include "lrucache.h"
using CacheT = fixed_sized_cache<int64_t,int64_t,LRUCachePolicy<int64_t>>;
const int N = 1'000'000;
int64_t dataa[N];
int main() {
  CacheT *cache = new CacheT(2);
  cache->Put(1, 1);
  cache->Put(2, 2);
  int64_t val;
  assert(cache->Get(1,val));
  assert(1 == val);
  assert(cache->Get(2,val));
  assert(2 == val);
  cache->Put(3, 3);
  assert(cache->Get(1,val) == false);
  assert(cache->Get(3,val));
  assert(3 == val);
  
  // ----------test performance-------------
  CacheT *cache2 = new CacheT(1024);
  Timer t;
  for(int i=0; i<N; ++i) {
    cache2->Put(i, i);
    int64_t val;
    assert(cache2->Get(i, val) == true);
    assert(val == i);
  }
  printf("policy cache IOPS: %fMops\n", N/t.GetDurationUs());
  lru_cache<int64_t,int64_t> *lru = new lru_cache<int64_t,int64_t>(1024);
  t.Reset();
  for(int i=0; i<N; ++i) {
    lru->put(i, i);
    int64_t val = lru->get(i);
    assert(val == i);
  }
  printf("stl cache IOPS: %fMops\n", N/t.GetDurationUs());
  SLruCache * cache3 = new SLruCache(1024);
  for(int i=0; i<N; ++i) {dataa[i] = i;}
  t.Reset();
  for(int i=0; i<N; ++i) {
    Slice key{(char*)&dataa[i], sizeof(int64_t)};
    auto handle = cache3->Insert(key, (void*)i);
    cache3->Release(handle);
    handle = cache3->Lookup(key);
    assert(i == (int64_t)HandleValue(handle));
    cache3->Release(handle);
  }
  printf("mycache IOPS: %fMops\n", N/t.GetDurationUs());
  return 0;
}

/*
policy cache IOPS: 0.774806Mops
stl cache IOPS: 1.522429Mops
mycache IOPS: 2.916201Mops
*/