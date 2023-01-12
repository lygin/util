#include <gtest/gtest.h>
#include <cstdio>
#include "threadpool.h"
#include "threadpool_lockfree.h"
#include "timer.h"

const int testnum = 10'0000;
int func(int i)
{
  return i * i;
}

TEST(threadpool, basic)
{
  ThreadPool pool(8);
  std::vector<std::future<int>> results;
  Timer tm;

  for (int i = 0; i < testnum; ++i)
  {
    results.emplace_back(
        pool.enqueue(func, i)
    );
  }
  int i = 0;
  for (auto &result : results)
  {
    int x = result.get();
    ASSERT_EQ(x, i * i);
    i++;
  }
  printf("IOPS %fMops\n", testnum/tm.GetDurationUs());
}

TEST(threadpool_lockfree, basic)
{
  lockfree::ThreadPool pool(8);
  std::vector<std::future<int>> results;
  Timer tm;
  for (int i = 0; i < testnum; ++i)
  {
    results.emplace_back(
        pool.enqueue(func, i)
    );
  }
  int i = 0;
  for (auto &result : results)
  {
    int x = result.get();
    ASSERT_EQ(x, i * i);
    i++;
  }
  printf("IOPS %fMops\n", testnum/tm.GetDurationUs());
}