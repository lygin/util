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
} cmp;

TEST(SkipTest, Empty) {
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
static constexpr int N = 2000;
static constexpr int R = 5000;
class SkipListTest : public ::testing::Test {
  protected:
  void SetUp() override {
    keys.clear();
    delete list;
    list = new SkipList<Key, TestComparator>(cmp);
  }
  public:
  SkipListTest(): rnd(0, R-1),list(new SkipList<Key, TestComparator>(cmp)), tp(4, 1024) {}
  std::set<Key> keys;
  RandomRng rnd;
  SkipList<Key, TestComparator> *list;
  lockfree::ThreadPool tp;

  void insert() {
    for (int i = 0; i < N; i++) {
      Key key = rnd.rand();
      if (keys.insert(key).second) {
        list->Insert(key);
      }
    }
  }
  void contains() {
    for (int i = 0; i < R; i++) {
      if (list->Contains(i)) {
        ASSERT_EQ(keys.count(i), 1U);
      } else {
        ASSERT_EQ(keys.count(i), 0U);
      }
    }
  }
  void simple_iter() {
    SkipList<Key, TestComparator>::Iterator iter(list);
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
  void forward_iter() {
    for (int i = 0; i < R; i++) {
      SkipList<Key, TestComparator>::Iterator iter(list);
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
};

TEST_F(SkipListTest, Concurent) {
  auto done = std::async(
    [&](){ insert(); }
  );
  done.wait();
  tp.enqueue(
    [&]{contains();}
  );
  tp.enqueue(
    [&]{simple_iter();}
  );
  tp.enqueue(
    [&]{forward_iter();}
  );
}