/**
 * thread safe lru cache implementation
 * learn from leveldb
*/
#pragma once
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <mutex>
#include <functional>

#include "slice.h"
#include "murmur2.h"
using namespace std;

// "in_cache" boolean indicating whether the cache has a reference on the entry. 
// The ways that this can become false:
// 1. being passed to its "deleter" are via Erase()
// 2. via Insert() when an element with a duplicate key is inserted
// 3. on destruction of the cache.
// the meaning of it:
// think one client is reading, but other client delete it, we cannot delete it immediately
// because the former might cannot read value, so we make it in_cache == false and ref==1
// when the former release it, we can safely delete it
struct LRUEntry
{
  void *value;
  LRUEntry *next_hash; //链表法解决哈希冲突
  LRUEntry *next;
  LRUEntry *prev;
  size_t key_length;
  uint32_t refs;
  uint32_t hash;     // Hash of key(); used for fast sharding and comparisons
  bool in_cache;     // Whether entry is in the cache.
  char key_data[1];  // Beginning of key K

  Slice key() const
  {
    assert(next != this);
    return Slice(key_data, key_length);
  }
};

class HashTable
{
public:
  HashTable() : length_(0), elems_(0), list_(nullptr) { Resize(); }
  ~HashTable() { delete[] list_; }

  LRUEntry *Lookup(const Slice &key, uint32_t hash)
  {
    return *FindPointer(key, hash);
  }
  //返回旧值，如果为null，则旧值不存在
  LRUEntry *Insert(LRUEntry *h)
  {
    LRUEntry **ptr = FindPointer(h->key(), h->hash);
    LRUEntry *old = *ptr;
    //如果old == null，说明是新添加的key
    //如果old不为null，则把old的next_hash付给h的next_hash
    h->next_hash = (old == nullptr ? nullptr : old->next_hash);
    *ptr = h;
    if (old == nullptr)
    {
      ++elems_;
      if (elems_ > length_)
      {
        Resize();
      }
    }
    return old;
  }

  LRUEntry *Remove(const Slice &key, uint32_t hash)
  {
    LRUEntry **ptr = FindPointer(key, hash);
    LRUEntry *result = *ptr;
    if (result != nullptr)
    {
      *ptr = result->next_hash;
      --elems_;
    }
    return result;
  }

  size_t Size() { return elems_; }

private:
  uint32_t length_; // capacity
  uint32_t elems_;  // size
  LRUEntry **list_; // hashtable

  LRUEntry **FindPointer(const Slice &key, uint32_t hash)
  {
    LRUEntry **ptr = &list_[hash & (length_ - 1)];
    while (*ptr != nullptr && ((*ptr)->hash != hash || key != (*ptr)->key()))
    {
      ptr = &(*ptr)->next_hash;
    }
    return ptr;
  }

  void Resize()
  {
    uint32_t new_length = 4;
    // new_length为首个大于elem的2的整数次幂
    while (new_length < elems_)
    {
      new_length <<= 1;
    }
    LRUEntry **new_list = new LRUEntry *[new_length];
    memset(new_list, 0, sizeof(LRUEntry *) * new_length);
    uint32_t count = 0;
    //一次迁移完
    for (uint32_t i = 0; i < length_; i++)
    {
      LRUEntry *h = list_[i];
      //迁移这个bucket
      while (h != nullptr)
      {
        LRUEntry *next = h->next_hash;
        uint32_t hash = h->hash;
        LRUEntry **ptr = &new_list[hash & (new_length - 1)];
        h->next_hash = *ptr;
        *ptr = h;
        h = next;
        count++;
      }
    }
    assert(elems_ == count);
    delete[] list_;
    list_ = new_list;
    length_ = new_length;
  }
};

class LRUCache
{
public:
  LRUCache();
  ~LRUCache();

  inline void SetCapacity(size_t capacity) { capacity_ = capacity; }
  inline void SetValDeleter(function<void(const Slice&, void* value)> deleter) { deleter_ = deleter; }

  LRUEntry *Insert(const Slice &key, uint32_t hash, void *value);
  LRUEntry *Lookup(const Slice &key, uint32_t hash);
  void Release(LRUEntry *handle); 
  void Erase(const Slice &key, uint32_t hash);
  //将lru_的节点全部删除
  void Prune();
  size_t TotalElem() const
  {
    std::lock_guard<std::mutex> l(mutex_);
    return usage_;
  }

private:
  void LRU_Remove(LRUEntry *e);
  void LRU_Append(LRUEntry *list, LRUEntry *e);
  void Ref(LRUEntry *e);
  void Unref(LRUEntry *e);
  bool FinishErase(LRUEntry *e);

  size_t capacity_;

  // mutex_ protects the following state.
  mutable std::mutex mutex_;
  size_t usage_;

  // Entries have refs==1 and in_cache==true.
  LRUEntry lru_;
  // Entries are in use by clients, and have refs >= 2 and in_cache==true.
  LRUEntry in_use_;
  function<void(const Slice&, void* value)> deleter_;
  HashTable table_;
};

inline LRUCache::LRUCache() : capacity_(0), usage_(0)
{
  // Make empty circular linked lists.
  lru_.next = &lru_;
  lru_.prev = &lru_;
  in_use_.next = &in_use_;
  in_use_.prev = &in_use_;
}

inline LRUCache::~LRUCache()
{
  // no client is using 
  assert(in_use_.next == &in_use_);
  for (LRUEntry *e = lru_.next; e != &lru_;)
  {
    LRUEntry *next = e->next;
    assert(e->in_cache);
    e->in_cache = false;
    assert(e->refs == 1);
    Unref(e);
    e = next;
  }
}

inline void LRUCache::Ref(LRUEntry *e)
{
  // If on lru_ list, move to in_use_ list.
  if (e->refs == 1 && e->in_cache) { 
    LRU_Remove(e);
    LRU_Append(&in_use_, e);
  }
  e->refs++;
}

inline void LRUCache::Unref(LRUEntry *e)
{
  assert(e->refs > 0);
  e->refs--;
  if (e->refs == 0) { 
    // Deallocate.
    assert(!e->in_cache);
    deleter_(e->key(), e->value);
    free(e);
  } else if (e->in_cache && e->refs == 1) {
    // No longer in use; move to lru_ list.
    LRU_Remove(e);
    LRU_Append(&lru_, e);
  }
  // if incache==false but refs==1, means one client has removed entry, but currently has client reading
  // cannot delete yet.
}

inline void LRUCache::LRU_Remove(LRUEntry *e)
{
  e->next->prev = e->prev;
  e->prev->next = e->next;
}

inline void LRUCache::LRU_Append(LRUEntry *list, LRUEntry *e)
{
  // put it to list backend
  e->next = list;
  e->prev = list->prev;
  e->prev->next = e;
  e->next->prev = e;
}

inline LRUEntry *LRUCache::Lookup(const Slice &key, uint32_t hash)
{
  std::lock_guard<std::mutex> l(mutex_);
  LRUEntry *e = table_.Lookup(key, hash);
  if (e != nullptr) {
    Ref(e);
  }
  return e;
}

inline void LRUCache::Release(LRUEntry *handle)
{
  std::lock_guard<std::mutex> l(mutex_);
  Unref(handle);
}

// return the newly created entry
inline LRUEntry *LRUCache::Insert(const Slice &key, uint32_t hash, void *value)
{
  std::lock_guard<std::mutex> l(mutex_);

  LRUEntry *e = (LRUEntry *)malloc(sizeof(LRUEntry) - 1 + key.size());
  e->value = value;
  e->key_length = key.size();
  e->hash = hash;
  e->refs = 1;
  e->in_cache = false;
  std::memcpy(e->key_data, key.data(), key.size());

  if (capacity_ > 0) {
    e->refs++; // client引用
    e->in_cache = true;
    LRU_Append(&in_use_, e);
    usage_++;
    //如果是替换，删除旧值
    FinishErase(table_.Insert(e));
  } else { 
    // don't cache. (capacity_==0 is not supported and turns off caching.)
    assert(false);
    e->next = nullptr;
  }
  while (usage_ > capacity_ && lru_.next != &lru_) {
    // list head is the oldest
    LRUEntry *oldest = lru_.next;
    assert(oldest->refs == 1);
    bool erased = FinishErase(table_.Remove(oldest->key(), oldest->hash));
    if (!erased){
      assert(erased);
    }
  }
  return e;
}

// If e != nullptr, finish removing *e from the cache; it has already been
// removed from the hash table.
// meanwhile, might have client reading cannot delete it
inline bool LRUCache::FinishErase(LRUEntry *e)
{
  if (e != nullptr)
  {
    assert(e->in_cache);
    LRU_Remove(e);
    e->in_cache = false;
    usage_--;
    Unref(e);
  }
  return e != nullptr;
}

inline void LRUCache::Erase(const Slice &key, uint32_t hash)
{
  std::lock_guard<std::mutex> l(mutex_);
  FinishErase(table_.Remove(key, hash));
}

inline void LRUCache::Prune()
{
  std::lock_guard<std::mutex> l(mutex_);
  while (lru_.next != &lru_)
  {
    LRUEntry *e = lru_.next;
    assert(e->refs == 1);
    bool erased = FinishErase(table_.Remove(e->key(), e->hash));
    if (!erased){
      assert(erased);
    }
  }
}

static const int kNumShardBits = 4;
static const int kNumShards = 1 << kNumShardBits;

class ShardedLRUCache
{
private:
  LRUCache shard_[kNumShards];

  static inline uint32_t HashSlice(const Slice &s)
  {
    return Hash(s.data(), s.size(), dict_hash_function_seed);
  }

  static uint32_t Shard(uint32_t hash) { return hash >> (32 - kNumShardBits); }

public:
  explicit ShardedLRUCache(size_t capacity, function<void(const Slice&, void*)> deleter=[](const Slice&, void* ){})
  {
    const size_t per_shard = (capacity + (kNumShards - 1)) / kNumShards;
    for (int s = 0; s < kNumShards; s++)
    {
      shard_[s].SetCapacity(per_shard);
      shard_[s].SetValDeleter(deleter);
    }
  }

  ~ShardedLRUCache() {}
  LRUEntry *Insert(const Slice &key, void *value)
  {
    const uint32_t hash = HashSlice(key);
    return shard_[Shard(hash)].Insert(key, hash, value);
  }
  LRUEntry *Lookup(const Slice &key)
  {
    const uint32_t hash = HashSlice(key);
    return shard_[Shard(hash)].Lookup(key, hash);
  }
  void Release(LRUEntry *handle)
  {
    LRUEntry *h = reinterpret_cast<LRUEntry *>(handle);
    shard_[Shard(h->hash)].Release(handle);
  }
  void Erase(const Slice &key)
  {
    const uint32_t hash = HashSlice(key);
    shard_[Shard(hash)].Erase(key, hash);
  }
  void Prune()
  {
    for (int s = 0; s < kNumShards; s++)
    {
      shard_[s].Prune();
    }
  }
  size_t TotalElem() const
  {
    size_t total = 0;
    for (int s = 0; s < kNumShards; s++)
    {
      total += shard_[s].TotalElem();
    }
    return total;
  }
};

using SLruCache = ShardedLRUCache;

#define HandleValue(handle) ((handle)->value)