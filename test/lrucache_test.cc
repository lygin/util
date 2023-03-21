#include "lrucache.h"

#include <vector>

#include "gtest/gtest.h"
#include "coding.h"

// Conversions between numeric keys/values and the types expected by Cache.
static std::string EncodeKey(int k) {
  std::string result;
  PutFixed32(&result, k);
  return result;
}
static int DecodeKey(const Slice& k) {
  assert(k.size() == 4);
  return DecodeFixed32(k.data());
}
static void* EncodeValue(uintptr_t v) { return reinterpret_cast<void*>(v); }
static int DecodeValue(void* v) { return reinterpret_cast<uintptr_t>(v); }

class CacheTest : public testing::Test {
 public:
  static void Deleter(const Slice& key, void* v) {
    current_->deleted_keys_.push_back(DecodeKey(key));
    current_->deleted_values_.push_back(DecodeValue(v));
  }

  static constexpr int kCacheSize = 1000;
  std::vector<int> deleted_keys_;
  std::vector<int> deleted_values_;
  SLruCache* cache_;

  CacheTest() : cache_(new SLruCache(kCacheSize, Deleter)) { 
      current_ = this; 
  }

  ~CacheTest() { delete cache_; }

  int Lookup(int key) {
    LRUEntry* handle = cache_->Lookup(EncodeKey(key));
    const int r = (handle == nullptr) ? -1 : DecodeValue(HandleValue(handle));
    if (handle != nullptr) {
      cache_->Release(handle);
    }
    return r;
  }

  void Insert(int key, int value, int charge = 1) {
    cache_->Release(cache_->Insert(EncodeKey(key), EncodeValue(value)));
  }

  LRUEntry* InsertAndReturnHandle(int key, int value, int charge = 1) {
    return cache_->Insert(EncodeKey(key), EncodeValue(value));
  }

  void Erase(int key) { cache_->Erase(EncodeKey(key)); }

  static CacheTest* current_;
};
CacheTest* CacheTest::current_;

TEST_F(CacheTest, HitAndMiss) {
  ASSERT_EQ(-1, Lookup(100));

  Insert(100, 101);
  ASSERT_EQ(101, Lookup(100));
  ASSERT_EQ(-1, Lookup(200));
  ASSERT_EQ(-1, Lookup(300));

  Insert(200, 201);
  ASSERT_EQ(101, Lookup(100));
  ASSERT_EQ(201, Lookup(200));
  ASSERT_EQ(-1, Lookup(300));

  Insert(100, 102);
  ASSERT_EQ(102, Lookup(100));
  ASSERT_EQ(201, Lookup(200));
  ASSERT_EQ(-1, Lookup(300));
  //deleted_keys_ is the replaced keys
  ASSERT_EQ(1, deleted_keys_.size());
  ASSERT_EQ(100, deleted_keys_[0]);
  ASSERT_EQ(101, deleted_values_[0]);
}

TEST_F(CacheTest, Erase) {
  Erase(200);
  ASSERT_EQ(0, deleted_keys_.size());

  Insert(100, 101);
  Insert(200, 201);
  Erase(100);
  ASSERT_EQ(-1, Lookup(100));
  ASSERT_EQ(201, Lookup(200));
  ASSERT_EQ(1, deleted_keys_.size());
  ASSERT_EQ(100, deleted_keys_[0]);
  ASSERT_EQ(101, deleted_values_[0]);

  Erase(100);
  ASSERT_EQ(-1, Lookup(100));
  ASSERT_EQ(201, Lookup(200));
  ASSERT_EQ(1, deleted_keys_.size());
}

TEST_F(CacheTest, EntriesArePinned) {
  Insert(100, 101);
  LRUEntry* h1 = cache_->Lookup(EncodeKey(100));
  ASSERT_EQ(101, DecodeValue(HandleValue(h1)));

  Insert(100, 102);
  LRUEntry* h2 = cache_->Lookup(EncodeKey(100));
  ASSERT_EQ(102, DecodeValue(HandleValue(h2)));
  ASSERT_EQ(0, deleted_keys_.size());

  cache_->Release(h1);
  ASSERT_EQ(1, deleted_keys_.size());
  ASSERT_EQ(100, deleted_keys_[0]);
  ASSERT_EQ(101, deleted_values_[0]);

  Erase(100);
  ASSERT_EQ(-1, Lookup(100));
  ASSERT_EQ(1, deleted_keys_.size());

  cache_->Release(h2);
  ASSERT_EQ(2, deleted_keys_.size());
  ASSERT_EQ(100, deleted_keys_[1]);
  ASSERT_EQ(102, deleted_values_[1]);
}

TEST_F(CacheTest, EvictionPolicy) {
  Insert(100, 101);
  Insert(200, 201);
  Insert(300, 301);
  LRUEntry* h = cache_->Lookup(EncodeKey(300));

  // Frequently used entry must be kept around,
  // as must things that are still in use.
  for (int i = 0; i < kCacheSize + 200; i++) {
    Insert(1000 + i, 2000 + i);
    ASSERT_EQ(2000 + i, Lookup(1000 + i));
    ASSERT_EQ(101, Lookup(100));
  }
  ASSERT_EQ(101, Lookup(100));
  ASSERT_EQ(-1, Lookup(200));
  ASSERT_EQ(301, Lookup(300));
  cache_->Release(h);
}

TEST_F(CacheTest, UseExceedsCacheSize) {
  // Overfill the cache, keeping handles on all inserted entries.
  std::vector<LRUEntry*> h;
  for (int i = 0; i < kCacheSize + 100; i++) {
    h.push_back(InsertAndReturnHandle(1000 + i, 2000 + i));
  }

  // Check that all the entries can be found in the cache.
  for (int i = 0; i < h.size(); i++) {
    ASSERT_EQ(2000 + i, Lookup(1000 + i));
  }

  for (int i = 0; i < h.size(); i++) {
    cache_->Release(h[i]);
  }
}

TEST_F(CacheTest, HeavyEntries) {
  // Add a bunch of light and heavy entries and then count the combined
  // size of items still in the cache, which must be approximately the
  // same as the total capacity.
  const int kLight = 1;
  int added = 0;
  int index = 0;
  while (added < 2 * kCacheSize) {
    const int weight = kLight;
    Insert(index, 1000 + index);
    added += weight;
    index++;
  }

  int cached_weight = 0;
  for (int i = 0; i < index; i++) {
    const int weight = kLight;
    int r = Lookup(i);
    if (r >= 0) {
      cached_weight += weight;
      ASSERT_EQ(1000 + i, r);
    }
  }
  ASSERT_LE(cached_weight, kCacheSize + kCacheSize);
}

TEST_F(CacheTest, Prune) {
  Insert(1, 100);
  Insert(2, 200);

  LRUEntry* handle = cache_->Lookup(EncodeKey(1));
  ASSERT_TRUE(handle);
  cache_->Prune();
  cache_->Release(handle);

  ASSERT_EQ(100, Lookup(1));
  ASSERT_EQ(-1, Lookup(2));
}
