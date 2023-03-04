#include "cache.hh"
#include "lru_cache_policy.hh"
#include "cassert"
using CacheT = fixed_sized_cache<int,int,LRUCachePolicy<int>>;
int main() {
  CacheT *cache = new CacheT(2);
  cache->Put(1, 1);
  cache->Put(2, 2);
  int val;
  assert(cache->Get(1,val));
  assert(1 == val);
  assert(cache->Get(2,val));
  assert(2 == val);
  cache->Put(3, 3);
  assert(cache->Get(1,val) == false);
  assert(cache->Get(3,val));
  assert(3 == val);
  return 0;
}