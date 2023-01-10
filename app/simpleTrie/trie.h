#pragma once
#include <cstdlib>
#include <cstdio>
#include <string>
#include <cassert>

#define LEAF_NUM 26

class TrieNode
{
public:
  TrieNode(char c = '\0', bool end = false)
  {
    c_ = c;
    end_ = end;
    next_ = (TrieNode **)calloc(LEAF_NUM, sizeof(TrieNode *));
  }
  // 返回引用，便于直接修改本身的值
  TrieNode *&GetChild(int index)
  {
    return next_[index];
  }
  void SetEnd()
  {
    end_ = true;
  }
  bool IsEnd()
  {
    return end_;
  }
  char GetChar()
  {
    return c_;
  }
  TrieNode **&GetNext()
  {
    return next_;
  }

private:
  char c_;
  TrieNode **next_;
  bool end_;
};

class Trie
{
public:
  Trie()
  {
    root = new TrieNode();
  }
  ~Trie()
  {
    dfs_free(root);
  }

  void insert(const std::string &str)
  {
    int i = 0, j = str.size();
    auto p = root;
    while (i < j)
    {
      char c = tolower(str[i]);
      TrieNode *&child = p->GetChild(c - 'a');
      if (child == nullptr)
      {
        child = new TrieNode(c);
      }
      if (i == j - 1)
        child->SetEnd();
      p = child;
      ++i;
    }
  }
  bool find(const std::string &str)
  {
    int i = 0, j = str.size();
    auto p = root;
    while (i < j && p != nullptr)
    {
      char c = tolower(str[i]);
      auto child = p->GetChild(c - 'a');
      if (child == nullptr)
      {
        return false;
      }
      if (i == j - 1 && child->IsEnd())
        return true;
      p = child;
      i++;
    }
    return false;
  }

private:
  TrieNode *root;

  void dfs_free(TrieNode *t)
  {
    if (t == nullptr)
      return;
    for (int i = 0; i < LEAF_NUM; i++)
    {
      dfs_free(t->GetChild(i));
    }
    free(t->GetNext());
  }
};