#pragma once
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <mutex>

#include "slice.h"
#include "murmur2.h"

//一个Entry是一个堆分配的struct
struct LRUEntry {
  void* value; //节点中保存的值
  void (*deleter)(const Slice&, void* value); // for erase()
  LRUEntry* next_hash; //链表法解决哈希冲突，指向下一个hash（key）相同的节点
  LRUEntry* next; 
  LRUEntry* prev; 
  size_t key_length; //节点键的长度
  bool in_cache;     // 有没有使用这个块。Whether entry is in the cache.
  uint32_t refs;     // References, including cache reference, if present.
  uint32_t hash;     // Hash of key(); used for fast sharding and comparisons
  char key_data[1];  // Beginning of key K

  Slice key() const {
    // next is only equal to this if the LRU handle is the list head of an
    // empty list. List heads never have meaningful keys.
    assert(next != this);

    return Slice(key_data, key_length);
  }
};

class HashTable {
 public:
  HashTable() : length_(0), elems_(0), list_(nullptr) { Resize(); }
  ~HashTable() { delete[] list_; }

  LRUEntry* Lookup(const Slice& key, uint32_t hash) {
    return *FindPointer(key, hash);
  }

  LRUEntry* Insert(LRUEntry* h) {
    LRUEntry** ptr = FindPointer(h->key(), h->hash);
    LRUEntry* old = *ptr;
    h->next_hash = (old == nullptr ? nullptr : old->next_hash);
    *ptr = h;
    if (old == nullptr) {
      ++elems_;
      if (elems_ > length_) {
        // Since each cache entry is fairly large, we aim for a small
        // average linked list length (<= 1).
        Resize();
      }
    }
    return old;
  }

  LRUEntry* Remove(const Slice& key, uint32_t hash) {
    LRUEntry** ptr = FindPointer(key, hash);
    LRUEntry* result = *ptr;
    if (result != nullptr) {
      *ptr = result->next_hash;
      --elems_;
    }
    return result;
  }

  size_t Size() { return elems_; }

 private:
  // The table consists of an array of buckets where each bucket is
  // a linked list of cache entries that hash into the bucket.
  uint32_t length_; //capacity
  uint32_t elems_; //size
  LRUEntry** list_; //Handle* 数组首地址

  // Return a pointer to slot that points to a cache entry that
  // matches key/hash.  If there is no such cache entry, return a
  // pointer to the trailing slot in the corresponding linked list.
  LRUEntry** FindPointer(const Slice& key, uint32_t hash) {
    LRUEntry** ptr = &list_[hash & (length_ - 1)];
    while (*ptr != nullptr && ((*ptr)->hash != hash || key != (*ptr)->key())) {
      ptr = &(*ptr)->next_hash;
    }
    return ptr;
  }

  void Resize() {
    uint32_t new_length = 4;
    while (new_length < elems_) {
      new_length *= 2;
    }
    LRUEntry** new_list = new LRUEntry*[new_length];
    memset(new_list, 0, sizeof(new_list[0]) * new_length);
    uint32_t count = 0;
    for (uint32_t i = 0; i < length_; i++) {
      LRUEntry* h = list_[i];
      while (h != nullptr) {
        LRUEntry* next = h->next_hash;
        uint32_t hash = h->hash;
        LRUEntry** ptr = &new_list[hash & (new_length - 1)];
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

class LRUCache {
 public:
  LRUCache();
  ~LRUCache();

  // Separate from constructor so caller can easily make an array of LRUCache
  void SetCapacity(size_t capacity) { capacity_ = capacity; }

  // Like Cache methods, but with an extra "hash" parameter.
  LRUEntry* Insert(const Slice& key, uint32_t hash, void* value,
                        void (*deleter)(const Slice& key, void* value));
  LRUEntry* Lookup(const Slice& key, uint32_t hash);
  void Release(LRUEntry* handle); //在哈希表中释放节点引用
  void Erase(const Slice& key, uint32_t hash); //在哈希表中删除节点
  void Prune(); //将lru_的节点全部删除
  size_t TotalCharge() const {
    std::lock_guard<std::mutex> l(mutex_);
    return usage_;
  }

 private:
  void LRU_Remove(LRUEntry* e);
  void LRU_Append(LRUEntry* list, LRUEntry* e);
  void Ref(LRUEntry* e);
  void Unref(LRUEntry* e);
  bool FinishErase(LRUEntry* e);

  // Initialized before use.
  size_t capacity_;

  // mutex_ protects the following state.
  mutable std::mutex mutex_; //保护这个LRU
  size_t usage_;
  // GUARDED_BY可以在编译时期检查对变量操作是否加锁，clang++ -Wthread-safety

  // Dummy head of LRU list.
  // lru.prev is newest entry, lru.next is oldest entry.
  // Entries have refs==1 and in_cache==true.
  // 可以进行淘汰的节点
  LRUEntry lru_;

  // Dummy head of in-use list.
  // Entries are in use by clients, and have refs >= 2 and in_cache==true.
  // 不可淘汰的节点（正在使用？）
  LRUEntry in_use_;

  HashTable table_;
};

LRUCache::LRUCache() : capacity_(0), usage_(0) {
  // Make empty circular linked lists.
  lru_.next = &lru_;
  lru_.prev = &lru_;
  in_use_.next = &in_use_;
  in_use_.prev = &in_use_;
}

LRUCache::~LRUCache() {
  assert(in_use_.next == &in_use_);  // Error if caller has an unreleased handle
  for (LRUEntry* e = lru_.next; e != &lru_;) {
    LRUEntry* next = e->next;
    assert(e->in_cache);
    e->in_cache = false;
    assert(e->refs == 1);  // Invariant of lru_ list.
    Unref(e);
    e = next;
  }
}

void LRUCache::Ref(LRUEntry* e) {
  if (e->refs == 1 && e->in_cache) {  // If on lru_ list, move to in_use_ list.
    LRU_Remove(e);
    LRU_Append(&in_use_, e);
  }
  e->refs++;
}

void LRUCache::Unref(LRUEntry* e) {
  assert(e->refs > 0);
  e->refs--;
  if (e->refs == 0) {  // Deallocate.
    assert(!e->in_cache);
    //blockcache则需要将该block(e->value)写到RMDA远端
    (*e->deleter)(e->key(), e->value);
    free(e);
  } else if (e->in_cache && e->refs == 1) {
    // No longer in use; move to lru_ list.
    LRU_Remove(e);
    LRU_Append(&lru_, e);
  }
}

void LRUCache::LRU_Remove(LRUEntry* e) {
  e->next->prev = e->prev;
  e->prev->next = e->next;
}

void LRUCache::LRU_Append(LRUEntry* list, LRUEntry* e) {
  // Make "e" newest entry by inserting just before *list
  e->next = list;
  e->prev = list->prev;
  e->prev->next = e;
  e->next->prev = e;
}

LRUEntry* LRUCache::Lookup(const Slice& key, uint32_t hash) {
  std::lock_guard<std::mutex> l(mutex_);
  LRUEntry* e = table_.Lookup(key, hash);
  if (e != nullptr) {
    Ref(e);
  }
  return e;
}

void LRUCache::Release(LRUEntry* handle) {
  std::lock_guard<std::mutex> l(mutex_);
  Unref(reinterpret_cast<LRUEntry*>(handle));
}

LRUEntry* LRUCache::Insert(const Slice& key, uint32_t hash, void* value,
                                void (*deleter)(const Slice& key,
                                                void* value)) {
  std::lock_guard<std::mutex> l(mutex_);

  LRUEntry* e =
      reinterpret_cast<LRUEntry*>(malloc(sizeof(LRUEntry) - 1 + key.size()));
  e->value = value;
  e->deleter = deleter;
  e->key_length = key.size();
  e->hash = hash;
  e->in_cache = false;
  e->refs = 1;  // for the returned handle.
  std::memcpy(e->key_data, key.data(), key.size());

  if (capacity_ > 0) {
    e->refs++;  // for the cache's reference.
    e->in_cache = true;
    LRU_Append(&in_use_, e);
    usage_ ++;
    FinishErase(table_.Insert(e));
  } else {  // don't cache. (capacity_==0 is supported and turns off caching.)
    // next is read by key() in an assert, so it must be initialized
    e->next = nullptr;
  }
  while (usage_ > capacity_ && lru_.next != &lru_) {//lru不空
    LRUEntry* old = lru_.next; //oldest
    assert(old->refs == 1);
    bool erased = FinishErase(table_.Remove(old->key(), old->hash));
    if (!erased) {  // to avoid unused variable when compiled NDEBUG
      assert(erased);
    }
  }

  return e;
}

// If e != nullptr, finish removing *e from the cache; it has already been
// removed from the hash table.  Return whether e != nullptr.
bool LRUCache::FinishErase(LRUEntry* e) {
  if (e != nullptr) {
    assert(e->in_cache);
    LRU_Remove(e);
    e->in_cache = false;
    usage_--;
    Unref(e);
  }
  return e != nullptr;
}

void LRUCache::Erase(const Slice& key, uint32_t hash) {
  std::lock_guard<std::mutex> l(mutex_);
  FinishErase(table_.Remove(key, hash));
}

void LRUCache::Prune() {
  std::lock_guard<std::mutex> l(mutex_);
  while (lru_.next != &lru_) {
    LRUEntry* e = lru_.next;
    assert(e->refs == 1);
    bool erased = FinishErase(table_.Remove(e->key(), e->hash));
    if (!erased) {  // to avoid unused variable when compiled NDEBUG
      assert(erased);
    }
  }
}

static const int kNumShardBits = 4;
static const int kNumShards = 1 << kNumShardBits;

class ShardedLRUCache {
 private:
  LRUCache shard_[kNumShards];
  std::mutex id_mutex_;
  uint64_t last_id_;

  static inline uint32_t HashSlice(const Slice& s) {
    return Hash(s.data(), s.size(), dict_hash_function_seed);
  }

  static uint32_t Shard(uint32_t hash) { return hash >> (32 - kNumShardBits); }

 public:
  explicit ShardedLRUCache(size_t capacity) : last_id_(0) {
    const size_t per_shard = (capacity + (kNumShards - 1)) / kNumShards;
    for (int s = 0; s < kNumShards; s++) {
      shard_[s].SetCapacity(per_shard);
    }
  }

  ~ShardedLRUCache() {}
  LRUEntry* Insert(const Slice& key, void* value, void (*deleter)(const Slice& key, void* value)) {
    const uint32_t hash = HashSlice(key);
    return shard_[Shard(hash)].Insert(key, hash, value, deleter);
  }
  LRUEntry* Lookup(const Slice& key) {
    const uint32_t hash = HashSlice(key);
    return shard_[Shard(hash)].Lookup(key, hash);
  }
  void Release(LRUEntry* handle) {
    LRUEntry* h = reinterpret_cast<LRUEntry*>(handle);
    shard_[Shard(h->hash)].Release(handle);
  }
  void Erase(const Slice& key) {
    const uint32_t hash = HashSlice(key);
    shard_[Shard(hash)].Erase(key, hash);
  }
  void* Value(LRUEntry* handle) {
    return reinterpret_cast<LRUEntry*>(handle)->value;
  }
  uint64_t NewId() {
    std::lock_guard<std::mutex> l(id_mutex_);
    return ++(last_id_);
  }
  void Prune() {
    for (int s = 0; s < kNumShards; s++) {
      shard_[s].Prune();
    }
  }
  size_t TotalCharge() const {
    size_t total = 0;
    for (int s = 0; s < kNumShards; s++) {
      total += shard_[s].TotalCharge();
    }
    return total;
  }
};

ShardedLRUCache* NewLRUCache(size_t capacity) { return new ShardedLRUCache(capacity); }