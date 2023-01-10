#pragma once
#include <map>
#include <vector>

template <typename K, typename V = int>
class Trie
{
public:
  Trie() : end_(false) {}
  V &insert(const K &key)
  {
    Trie *node = this;
    for (auto &c : key)
    {
      auto iter = node->child_.find(c);
      if (iter == node->child_.end())
      {
        auto pref = node->prefix_;
        std::back_inserter(pref) = c;
        /* data store in std::map(which in heap) */
        node->child_[c] = Trie(pref);
      }
      node = &(node->child_[c]);
    }
    node->end_ = true;
    return node->value_;
  }
  V &operator[](const K &key)
  {
    return insert(key);
  }
  bool find(const K &key)
  {
    Trie *node = this;
    for (const auto &c : key)
    {
      auto iter = node->child_.find(c);
      if (iter == node->child_.end())
      {
        return false;
      }
      node = &(iter->second);
    }
    return node->end_;
  }
  std::vector<const K *> list_keys() const
  {
    std::vector<const K *> keys;
    if (end_ == true)
    {
      keys.push_back(&prefix_);
    }
    for (auto &c : child_)
    {
      const auto ckeys = c.second.list_keys();
      for (const auto &k : ckeys)
      {
        keys.push_back(k);
      }
    }
    return keys;
  }

  unsigned int size() const
  {
    return list_keys().size();
  }

  std::vector<const K *> complete(const K &prefix) const
  {
    const Trie *node = this;
    for (const auto &c : prefix)
    {
      const auto iter = node->child_.find(c);
      // no one match
      if (iter == node->child_.end())
      {
        return std::vector<const K *>();
      }
      node = &(iter->second);
    }
    return node->list_keys();
  }

protected:
  Trie(const K &pref) : prefix_(pref), end_(false) {}
  K prefix_; // (optional)for list keys and complete a key
  V value_;  // (optional)for store value
  bool end_;
  std::map<typename K::value_type, Trie> child_;
};