#pragma once
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <atomic>
#include <unordered_map>
#include <thread>
#include "xxhash.h"
#include "commom.h"
#include "arena.h"

using namespace std;


struct Bucket
{
  uint64_t key;
  uint64_t value;
  Bucket* next_bucket;
  Bucket() : next_bucket(nullptr) {}
};

class HashTable
{
  Arena *get_arena()
  {
    thread_local Arena *arena = new Arena;
    return arena;
  }
public:
  HashTable(uint32_t capacity) : size_(0), list_(nullptr) {
    capacity_ = nextPowerOf2(capacity);
    list_ = new Bucket* [capacity_];
    for(uint32_t i = 0; i < capacity_; i++) list_[i] = nullptr;
  }
  ~HashTable() {
    delete[] list_;
  }

  void Put(uint64_t key, uint64_t value)
  {
    uint64_t hash = XXH3_64bits(&key, sizeof(key));
    uint32_t bucket_no = hash & (capacity_ - 1);
    Bucket* newbucket = (Bucket*)get_arena()->Allocate(sizeof(Bucket));
    newbucket->key = key;
    newbucket->value = value;
    while(true) {
      newbucket->next_bucket = list_[bucket_no];
      Bucket *old_head = list_[bucket_no];
      if(__sync_bool_compare_and_swap(&list_[bucket_no], old_head, newbucket)) {
        break;
      }
    }
  }

  bool Get(uint64_t key, uint64_t &value)
  {
    uint64_t hash = XXH3_64bits(&key, sizeof(key));
    uint32_t bucket_no = hash & (capacity_ - 1);
    Bucket* p = list_[bucket_no];
    while(p != nullptr && p->key != key) {
      p = p->next_bucket;
    }
    if(p != nullptr) {
      value = p->value;
      return true;
    }
    return false;
  }

  size_t Size() { return size_; }

private:
  uint32_t capacity_; // capacity
  uint32_t size_;  // size
  Bucket **list_; // hashtable
};