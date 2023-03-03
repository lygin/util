extern "C" {
#include "clht.h"
}

#include <cassert>
static const int DataNum = 1'000'000;

int main() {
  clht_t* ht = clht_create(1024);
  // initializes the GC for hash table resizing. 
  // Every thread should make this call before using the hash table.
  clht_gc_thread_init(ht, 1);
  for(int i=0; i<DataNum; ++i) {
    clht_put(ht, i, i);
  }
  for(int i=0; i<DataNum; ++i) {
    int val = clht_get(ht->ht, i);
    assert(val == i);
  }
  return 0;
} 