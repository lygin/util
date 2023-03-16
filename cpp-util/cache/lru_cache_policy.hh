#ifndef LRU_CACHE_POLICY_HH
#define LRU_CACHE_POLICY_HH

#include "cache_policy.hh"
#include <list>
#include <unordered_map>

// lru policy just store key's order in the list
template <typename Key>
class LRUCachePolicy : public ICachePolicy<Key>
{
public:
  using lru_iterator = typename std::list<Key>::iterator;

  LRUCachePolicy() = default;
  ~LRUCachePolicy() = default;

  void Insert(const Key &key) override
  {
    lru_queue.emplace_front(key);
    key_finder[key] = lru_queue.begin();
  }

  void Touch(const Key &key) override
  {
    // move the touched element at the beginning of the lru_queue
    lru_queue.splice(lru_queue.begin(), lru_queue, key_finder[key]);
  }


  void Erase(const Key &key) override
  {
    lru_queue.erase(key_finder[key]);
    key_finder.erase(key);
  }

  // return a key of a displacement candidate
  const Key &ReplCandidate() const override
  {
    return lru_queue.back();
  }

private:
  std::list<Key> lru_queue;
  std::unordered_map<Key, lru_iterator> key_finder;
};
#endif