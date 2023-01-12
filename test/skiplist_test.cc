/* TODO: ADD arena */
#include <gtest/gtest.h>
#include <set>
#include <random>
#include <future>
#include <cstdlib>
#include "threadpool_lockfree.h"
#include "Random.h"
#include "skiplist.h"

using Key = uint64_t;

struct TestComparator {
  int operator()(const Key& a, const Key& b) const {
    if (a < b) {
      return -1;
    } else if (a > b) {
      return +1;
    } else {
      return 0;
    }
  }
};

TEST(SkipTest, Empty) {
  TestComparator cmp;
  SkipList<Key, TestComparator> list(cmp);
  ASSERT_TRUE(!list.Contains(10));

  SkipList<Key, TestComparator>::Iterator iter(&list);
  ASSERT_TRUE(!iter.Valid());
  iter.SeekToFirst();
  ASSERT_TRUE(!iter.Valid());
  iter.Seek(100);
  ASSERT_TRUE(!iter.Valid());
  iter.SeekForPrev(100);
  ASSERT_TRUE(!iter.Valid());
  iter.SeekToLast();
  ASSERT_TRUE(!iter.Valid());
}

TEST(SkipTest, InsertAndLookup) {
  const int N = 2000;
  const int R = 5000;
  std::set<Key> keys;
  TestComparator cmp;
  RandomRng rd(0, R-1);
  SkipList<Key, TestComparator> list(cmp);
  for (int i = 0; i < N; i++) {
    Key key = rd.rand();
    if (keys.insert(key).second) {
      list.Insert(key);
    }
  }

  for (int i = 0; i < R; i++) {
    if (list.Contains(i)) {
      ASSERT_EQ(keys.count(i), 1U);
    } else {
      ASSERT_EQ(keys.count(i), 0U);
    }
  }

  // Simple iterator tests
  {
    SkipList<Key, TestComparator>::Iterator iter(&list);
    ASSERT_TRUE(!iter.Valid());

    iter.Seek(0);
    ASSERT_TRUE(iter.Valid());
    ASSERT_EQ(*(keys.begin()), iter.key());

    iter.SeekForPrev(R - 1);
    ASSERT_TRUE(iter.Valid());
    ASSERT_EQ(*(keys.rbegin()), iter.key());

    iter.SeekToFirst();
    ASSERT_TRUE(iter.Valid());
    ASSERT_EQ(*(keys.begin()), iter.key());

    iter.SeekToLast();
    ASSERT_TRUE(iter.Valid());
    ASSERT_EQ(*(keys.rbegin()), iter.key());
  }

  // Forward iteration test
  for (int i = 0; i < R; i++) {
    SkipList<Key, TestComparator>::Iterator iter(&list);
    iter.Seek(i);

    // Compare against model iterator
    std::set<Key>::iterator model_iter = keys.lower_bound(i);
    for (int j = 0; j < 3; j++) {
      if (model_iter == keys.end()) {
        ASSERT_TRUE(!iter.Valid());
        break;
      } else {
        ASSERT_TRUE(iter.Valid());
        ASSERT_EQ(*model_iter, iter.key());
        ++model_iter;
        iter.Next();
      }
    }
  }

  // Backward iteration test
  for (int i = 0; i < R; i++) {
    SkipList<Key, TestComparator>::Iterator iter(&list);
    iter.SeekForPrev(i);

    // Compare against model iterator
    std::set<Key>::iterator model_iter = keys.upper_bound(i);
    for (int j = 0; j < 3; j++) {
      if (model_iter == keys.begin()) {
        ASSERT_TRUE(!iter.Valid());
        break;
      } else {
        ASSERT_TRUE(iter.Valid());
        ASSERT_EQ(*--model_iter, iter.key());
        iter.Prev();
      }
    }
  }
}

const int N = 2000;
const int R = 5000;
std::set<Key> keys;
RandomRng rnd(0, R-1);

int insert(SkipList<Key, TestComparator>& list) {
  for (int i = 0; i < N; i++) {
    Key key = rnd.rand();
    if (keys.insert(key).second) {
      list.Insert(key);
    }
  }
  return 1;
}

void contains(SkipList<Key, TestComparator>& list) {
  for (int i = 0; i < R; i++) {
    if (list.Contains(i)) {
      ASSERT_EQ(keys.count(i), 1U);
    } else {
      ASSERT_EQ(keys.count(i), 0U);
    }
  }
}

void simple_iter(SkipList<Key, TestComparator>& list) {
  SkipList<Key, TestComparator>::Iterator iter(&list);
  ASSERT_TRUE(!iter.Valid());

  iter.Seek(0);
  ASSERT_TRUE(iter.Valid());
  ASSERT_EQ(*(keys.begin()), iter.key());

  iter.SeekForPrev(R - 1);
  ASSERT_TRUE(iter.Valid());
  ASSERT_EQ(*(keys.rbegin()), iter.key());

  iter.SeekToFirst();
  ASSERT_TRUE(iter.Valid());
  ASSERT_EQ(*(keys.begin()), iter.key());

  iter.SeekToLast();
  ASSERT_TRUE(iter.Valid());
  ASSERT_EQ(*(keys.rbegin()), iter.key());
}

void forward_iter(SkipList<Key, TestComparator>& list) {
  for (int i = 0; i < R; i++) {
    SkipList<Key, TestComparator>::Iterator iter(&list);
    iter.Seek(i);

    // Compare against model iterator
    std::set<Key>::iterator model_iter = keys.lower_bound(i);
    for (int j = 0; j < 3; j++) {
      if (model_iter == keys.end()) {
        ASSERT_TRUE(!iter.Valid());
        break;
      } else {
        ASSERT_TRUE(iter.Valid());
        ASSERT_EQ(*model_iter, iter.key());
        ++model_iter;
        iter.Next();
      }
    }
  }
}

// We want to make sure that with a single writer and multiple
// concurrent readers (with no synchronization other than when a
// reader's iterator is created)
TEST(SkipTest, Concurent) {
  lockfree::ThreadPool tp(4, 1024);
  TestComparator cmp;
  SkipList<Key, TestComparator> list(cmp);
  std::future<int> done;
  done = tp.enqueue(
    [&](){return insert(list);}
  );
  done.get();
  tp.enqueue(
    [&]{contains(list);}
  );
  tp.enqueue(
    [&]{simple_iter(list);}
  );
  tp.enqueue(
    [&]{forward_iter(list);}
  );
}